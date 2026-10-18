import random
from collections.abc import Callable
from dataclasses import dataclass, replace

from src.evaluator_ref import (
    _assigned_resource_ids,
    _events_in_applies_to,
    _resources_in_applies_to,
    evaluate_constraint,
    resolve_occurrence,
    resolve_occurrences,
    valid_start_time_ids,
)
from src.incremental import IncrementalEvaluator
from src.model import Instance, Solution, SolutionEvent
from src.moves import (
    kempe_chain_move,
    large_perturbation_move,
    resource_reassign_move,
    small_perturbation_move,
    split_resize_move,
    time_reassign_move,
    time_swap_move,
)


@dataclass(frozen=True)
class Heuristic:
    """One entry in the manual heuristic pool -- wraps a plain apply()
    function with the identity/metadata the future RL selector (Etap 6)
    needs (id for its per-heuristic stats, protected so it's never pruned).

    `incremental_safe` declares that the operator only ever re-times or
    re-resources existing solution events, never changes how many there are
    or which instance event each one belongs to -- the precondition the
    incremental evaluator needs. It defaults to False so that anything
    registered later (notably LLM-generated heuristics, Etap 7) is scored by
    full evaluation unless it has been shown to hold.
    """

    id: str
    name: str
    protected: bool
    apply: Callable[[Solution, Instance, random.Random], Solution]
    incremental_safe: bool = False
    # Optional faster path: the same operator, given the solver's
    # IncrementalEvaluator positioned on `solution`, so it can score its
    # candidate times with probe() instead of building an evaluator from
    # scratch on every call (~25% of run time in profiling). It must give
    # exactly what apply() gives for the same rng state and must leave
    # the evaluator where it found it -- probe() only, never commit().
    apply_with_evaluator: (
        Callable[[Solution, Instance, random.Random, IncrementalEvaluator], Solution]
        | None
    ) = None


def move_random(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.time_reassign_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Raises ValueError (via
    time_reassign_move) if solution has no events, or the chosen event has
    no alternative valid time to move to."""
    return time_reassign_move(instance, solution, rng)


def _best_time_for_event(
    instance: Instance,
    solution: Solution,
    index: int,
    evaluator: IncrementalEvaluator | None = None,
    rng: random.Random | None = None,
    max_probe: int | None = None,
) -> Solution:
    """Returns the solution obtained by moving solution.events[index] to
    whichever valid start time (INCLUDING its current one) yields the
    lowest cost (numerically identical to evaluator_ref.total_cost).
    Including the current time means the result is never worse than the
    input -- move_best and repair_hard_violation both depend on this to
    guarantee they never regress the solution.

    Candidate times are scored incrementally: one full evaluation to seed
    the state, then one delta per candidate, instead of one full evaluation
    per candidate. `evaluator` lets a caller that already holds state for
    `solution` (ruin_and_recreate, repair_hard_violation) skip even that
    seed; it must be positioned on `solution`.

    When `max_probe` is set and `rng` is provided, only a random sample of
    `max_probe` candidates is evaluated. This trades exhaustive search for
    speed on large instances with many valid times per event."""
    event = solution.events[index]
    # construct.build_initial only ever leaves duration=None paired with
    # time_ref=None (a resources-only SolutionEvent that never needed a
    # time) -- for any event actually worth reassigning, duration is set.
    # moves.time_reassign_move carries the same assumption today without
    # asserting it (a pre-existing gap, out of scope to fix here); this
    # assert makes the same assumption explicit and satisfies mypy
    # --strict, and turns a latent None into a clear error instead of a
    # TypeError inside valid_start_time_ids if it's ever violated.
    assert event.duration is not None, f"event {event.event_ref!r} has no duration"
    candidates = list(valid_start_time_ids(instance, event.duration))
    if not candidates:
        raise ValueError(f"event {event.event_ref!r} has no valid start time")

    if max_probe is not None and rng is not None and len(candidates) > max_probe:
        candidates = rng.sample(candidates, max_probe)

    scorer = (
        evaluator if evaluator is not None else IncrementalEvaluator(instance, solution)
    )
    best_solution: Solution | None = None
    best_cost: int | None = None
    for time_ref in candidates:
        new_events = list(solution.events)
        new_events[index] = replace(event, time_ref=time_ref)
        candidate_solution = replace(solution, events=new_events)
        cost = scorer.probe(candidate_solution).as_scalar()
        if best_cost is None or cost < best_cost:
            best_cost = cost
            best_solution = candidate_solution
    assert best_solution is not None
    return best_solution


def move_best(
    solution: Solution,
    instance: Instance,
    rng: random.Random,
    max_probe: int | None = None,
    evaluator: IncrementalEvaluator | None = None,
) -> Solution:
    """Picks one solution event with a time to reassign at random, then
    moves it to the valid start time that minimizes total_cost (see
    _best_time_for_event). Raises ValueError if no solution event has a
    time_ref to reassign (e.g. every event is a resources-only entry --
    see construct.build_initial), or if the chosen event turns out to have
    no valid start time at all (propagated from _best_time_for_event)."""
    candidates = [i for i, se in enumerate(solution.events) if se.time_ref is not None]
    if not candidates:
        raise ValueError(
            "cannot apply a move: no solution event has a time to reassign"
        )
    index = rng.choice(candidates)
    return _best_time_for_event(
        instance, solution, index, evaluator, rng=rng, max_probe=max_probe
    )


def swap(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.time_swap_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Two events that happen
    to already share a time_ref produce a harmless structurally-valid
    no-op, not an error -- no special-casing needed. Raises ValueError
    (via time_swap_move) if fewer than 2 solution events exist, or no
    valid swap is found within its attempt budget."""
    return time_swap_move(instance, solution, rng)


def resource_reassign(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    """Thin wrapper around moves.resource_reassign_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Raises ValueError (via
    resource_reassign_move) if no event resource has a same-type alternative."""
    return resource_reassign_move(instance, solution, rng)


def kempe_chain(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.kempe_chain_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Raises ValueError (via
    kempe_chain_move) if the instance has fewer than 2 times, fewer than 2
    eligible events sit at the two chosen times, or none of them share a
    resource to chain on."""
    return kempe_chain_move(instance, solution, rng)


def large_perturbation(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    """Thin wrapper around moves.large_perturbation_move, adapted to the
    pool's apply(solution, instance, rng) argument order. Raises
    ValueError (via large_perturbation_move) if the solution has no
    movable event to perturb, or a chosen event has no valid start time."""
    return large_perturbation_move(instance, solution, rng)


def small_perturbation(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    """Thin wrapper around moves.small_perturbation_move, adapted to the
    pool's apply(solution, instance, rng) argument order."""
    return small_perturbation_move(instance, solution, rng)


_RUIN_FRACTION = 0.1
_RUIN_MIN_EVENTS = 2
_RUIN_MAX_EVENTS = 6


def ruin_and_recreate(
    solution: Solution,
    instance: Instance,
    rng: random.Random,
    max_probe: int | None = None,
) -> Solution:
    """ "Ruin-and-recreate" perturbation (Schrimpf et al. 1998): "ruins" a
    small random slice of the solution -- about _RUIN_FRACTION of its
    movable events, clamped to [_RUIN_MIN_EVENTS, _RUIN_MAX_EVENTS] -- by
    reassigning each to a uniformly random valid time, then "recreates"
    them one at a time in random order, each landing on whichever valid
    time currently minimizes total_cost given everything already rebuilt
    (the same greedy step _best_time_for_event uses for move_best). Reaches
    further in one call than a single move_random/move_best step -- a
    stronger perturbation meant to help LAHC escape plateaus a one-event
    move can't -- at the cost of being far more expensive per call (O(k)
    full evaluations across every valid time, for each of k ruined
    events)."""
    movable = [
        i
        for i, se in enumerate(solution.events)
        if se.time_ref is not None and se.duration is not None
    ]
    if not movable:
        raise ValueError(
            "cannot apply ruin-and-recreate: no solution event has a time to reassign"
        )

    ruin_target = max(_RUIN_MIN_EVENTS, round(len(movable) * _RUIN_FRACTION))
    k = min(len(movable), _RUIN_MAX_EVENTS, ruin_target)
    ruined_indices = rng.sample(movable, k)

    events = list(solution.events)
    for i in ruined_indices:
        se = events[i]
        assert se.duration is not None
        candidates = valid_start_time_ids(instance, se.duration)
        if not candidates:
            raise ValueError(f"event {se.event_ref!r} has no valid start time")
        events[i] = replace(se, time_ref=rng.choice(candidates))
    current = replace(solution, events=events)

    recreate_order = list(ruined_indices)
    rng.shuffle(recreate_order)
    # One state, advanced as each ruined event is placed -- the recreate
    # phase is the single most expensive operator in the pool (k events x
    # every valid time), and it used to pay a full evaluation for each of
    # those candidates.
    evaluator = IncrementalEvaluator(instance, current)
    for i in recreate_order:
        current = _best_time_for_event(
            instance, current, i, evaluator, rng=rng, max_probe=max_probe
        )
        evaluator.commit(current)
    return current


def _movable_violating_indices(
    instance: Instance,
    solution: Solution,
    evaluator: IncrementalEvaluator | None = None,
) -> list[int]:
    """Indices into solution.events whose event participates in at least
    one violated Required constraint (via either an event-scoped or a
    resource-scoped AppliesTo -- e.g. AvoidClashesConstraint applies to
    Resources/ResourceGroups, not Events, so it needs the resource-side
    resolution too) AND is actually movable: it has a time_ref set (a
    fully preassigned event never appears in solution.events at all, and
    a resources-only entry -- see construct.build_initial -- has
    time_ref=None; both are excluded here) and a non-empty set of valid
    alternative start times.

    An `evaluator` positioned on `solution` supplies the per-constraint
    costs it already tracks, so no separate evaluation pass is needed. The
    resulting set of candidate events is the same either way."""
    if evaluator is not None:
        occurrences = evaluator.occurrences
        constraint_costs = evaluator.constraint_costs()
    else:
        occurrences = resolve_occurrences(instance, solution)
        constraint_costs = tuple(
            evaluate_constraint(instance, occurrences, c) for c in instance.constraints
        )

    violated_event_ids: set[str] = set()
    for constraint, cost in zip(instance.constraints, constraint_costs, strict=True):
        if not constraint.required or cost <= 0:
            continue
        violated_event_ids |= _events_in_applies_to(instance, constraint.applies_to)
        violated_resource_ids = _resources_in_applies_to(
            instance, constraint.applies_to
        )
        if violated_resource_ids:
            violated_event_ids |= {
                o.event_ref
                for o in occurrences
                if violated_resource_ids & set(_assigned_resource_ids(o))
            }

    return [
        i
        for i, se in enumerate(solution.events)
        if se.event_ref in violated_event_ids
        and se.time_ref is not None
        and se.duration is not None
        and valid_start_time_ids(instance, se.duration)
    ]


def repair_hard_violation(
    solution: Solution,
    instance: Instance,
    rng: random.Random,
    max_probe: int | None = None,
    evaluator: IncrementalEvaluator | None = None,
) -> Solution:
    """Picks one (movable) event involved in a violated Required
    constraint at random and moves it to its best available time (see
    _best_time_for_event) -- never a purely random move, so the operator
    actually tends to repair rather than just perturb."""
    if evaluator is None:
        evaluator = IncrementalEvaluator(instance, solution)
    candidates = _movable_violating_indices(instance, solution, evaluator)
    if not candidates:
        raise ValueError("no movable event participates in a hard constraint violation")
    index = rng.choice(candidates)
    return _best_time_for_event(
        instance, solution, index, evaluator, rng=rng, max_probe=max_probe
    )


class _ClashTable:
    """Resource-time occupancy for the resources watched by Required
    AvoidClashesConstraints: how many events use each (resource, time)
    cell. Fixed (preassigned) events are counted too, since they block a
    slot just like movable ones."""

    def __init__(self, instance: Instance, solution: Solution) -> None:
        self.watched: set[str] = set()
        for constraint in instance.constraints:
            if constraint.required and constraint.type == "AvoidClashesConstraint":
                self.watched |= _resources_in_applies_to(
                    instance, constraint.applies_to
                )
        self._position = {t.id: k for k, t in enumerate(instance.times)}
        self._event_defs = {e.id: e for e in instance.events}
        self._busy: dict[tuple[str, int], int] = {}
        for occurrence in resolve_occurrences(instance, solution):
            if occurrence.time_ref is None:
                continue
            for resource in self._watched_of(_assigned_resource_ids(occurrence)):
                for p in self.span(occurrence.time_ref, occurrence.duration):
                    self._busy[(resource, p)] = self._busy.get((resource, p), 0) + 1

    def _watched_of(self, resources: list[str | None]) -> list[str]:
        return [r for r in resources if r is not None and r in self.watched]

    def span(self, time_ref: str, duration: int) -> range:
        start = self._position[time_ref]
        return range(start, start + duration)

    def resources(self, event: SolutionEvent) -> list[str]:
        occurrence = resolve_occurrence(self._event_defs[event.event_ref], event)
        return self._watched_of(_assigned_resource_ids(occurrence))

    def clashes(self, event: SolutionEvent) -> bool:
        if event.time_ref is None or event.duration is None:
            return False
        own = self.span(event.time_ref, event.duration)
        return any(
            self._busy.get((r, p), 0) > 1 for r in self.resources(event) for p in own
        )

    def overlaps(self, event: SolutionEvent, time_ref: str, duration: int) -> int:
        """Other events sharing a watched resource with `event` if it started
        at `time_ref` -- its own current booking is discounted."""
        own = (
            set(self.span(event.time_ref, duration))
            if event.time_ref is not None
            else set()
        )
        return sum(
            self._busy.get((r, p), 0) - (1 if p in own else 0)
            for r in self.resources(event)
            for p in self.span(time_ref, duration)
        )


def _cheapest_time(
    instance: Instance,
    solution: Solution,
    index: int,
    times: list[str],
    evaluator: IncrementalEvaluator | None = None,
) -> Solution:
    """The solution with solution.events[index] moved to whichever of
    `times` gives the lowest total cost."""
    event = solution.events[index]
    scorer = (
        evaluator if evaluator is not None else IncrementalEvaluator(instance, solution)
    )
    best_solution: Solution | None = None
    best_cost: int | None = None
    for time_ref in times:
        new_events = list(solution.events)
        new_events[index] = replace(event, time_ref=time_ref)
        candidate = replace(solution, events=new_events)
        cost = scorer.probe(candidate).as_scalar()
        if best_cost is None or cost < best_cost:
            best_cost, best_solution = cost, candidate
    assert best_solution is not None
    return best_solution


def repair_clash(
    solution: Solution,
    instance: Instance,
    rng: random.Random,
    max_probe: int | None = None,
    evaluator: IncrementalEvaluator | None = None,
) -> Solution:
    """Targets the hard violation that dominates every in-scope instance:
    a resource (teacher, class, room) booked into two events at once, as
    watched by a Required AvoidClashesConstraint.

    Picks one movable event that takes part in such a clash and moves it to
    the start time where its watched resources overlap the fewest other
    events -- a free slot when one exists. Unlike repair_hard_violation,
    which samples `max_probe` random times and compares total costs, this
    counts overlaps for *every* valid start time from a resource-time
    occupancy table, so it never misses a free slot. Total cost only breaks
    ties between the least-clashing times (sampled down to `max_probe`), so
    the move also respects the other constraints where it can.

    Raises ValueError if no Required AvoidClashesConstraint exists or no
    movable event takes part in a clash."""
    table = _ClashTable(instance, solution)
    if not table.watched:
        raise ValueError("the instance has no Required AvoidClashesConstraint")
    clashing = [i for i, se in enumerate(solution.events) if table.clashes(se)]
    if not clashing:
        raise ValueError("no movable event takes part in a clash")

    index = rng.choice(clashing)
    event = solution.events[index]
    assert event.duration is not None  # table.clashes() checked it
    candidates = list(valid_start_time_ids(instance, event.duration))
    if not candidates:
        raise ValueError(f"event {event.event_ref!r} has no valid start time")
    counts = {t: table.overlaps(event, t, event.duration) for t in candidates}
    fewest = min(counts.values())
    best_times = [t for t in candidates if counts[t] == fewest]
    if max_probe is not None and len(best_times) > max_probe:
        best_times = rng.sample(best_times, max_probe)
    return _cheapest_time(instance, solution, index, best_times, evaluator)


# Probe cap used for the "fast" variants of exhaustive-search heuristics.
# Reducing the number of candidate times probed per call trades some per-call
# quality for more iterations within a fixed time budget -- particularly useful
# on large instances where valid_start_time_ids returns 50-60 candidates.
_FAST_MAX_PROBE = 20

MANUAL_HEURISTICS: list[Heuristic] = [
    Heuristic(
        id="move_random",
        name="Random time reassignment",
        protected=True,
        apply=move_random,
        incremental_safe=True,
    ),
    Heuristic(
        id="move_best",
        name=f"Best-slot time reassignment (sample {_FAST_MAX_PROBE})",
        protected=True,
        apply=lambda sol, inst, rng: move_best(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE
        ),
        incremental_safe=True,
        apply_with_evaluator=lambda sol, inst, rng, ev: move_best(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE, evaluator=ev
        ),
    ),
    Heuristic(
        id="swap",
        name="Swap two events' times",
        protected=True,
        apply=swap,
        incremental_safe=True,
    ),
    Heuristic(
        id="repair_hard_violation",
        name=f"Repair a hard constraint violation (sample {_FAST_MAX_PROBE})",
        protected=True,
        apply=lambda sol, inst, rng: repair_hard_violation(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE
        ),
        incremental_safe=True,
        apply_with_evaluator=lambda sol, inst, rng, ev: repair_hard_violation(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE, evaluator=ev
        ),
    ),
    Heuristic(
        id="repair_clash",
        name=f"Move a clashing event to a free time (sample {_FAST_MAX_PROBE})",
        protected=True,
        apply=lambda sol, inst, rng: repair_clash(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE
        ),
        incremental_safe=True,
        apply_with_evaluator=lambda sol, inst, rng, ev: repair_clash(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE, evaluator=ev
        ),
    ),
    Heuristic(
        id="resource_reassign",
        name="Reassign an event resource to a same-type alternative",
        protected=True,
        apply=resource_reassign,
        incremental_safe=True,
    ),
    Heuristic(
        id="kempe_chain",
        name="Kempe chain interchange between two times",
        protected=True,
        apply=kempe_chain,
        incremental_safe=True,
    ),
    Heuristic(
        id="ruin_and_recreate",
        name=f"Ruin-and-recreate (sample {_FAST_MAX_PROBE} per event)",
        protected=True,
        apply=lambda sol, inst, rng: ruin_and_recreate(
            sol, inst, rng, max_probe=_FAST_MAX_PROBE
        ),
        incremental_safe=True,
    ),
    Heuristic(
        id="large_perturbation",
        name="Large random perturbation across many events",
        protected=True,
        apply=large_perturbation,
        incremental_safe=True,
    ),
    Heuristic(
        id="small_perturbation",
        name="Small random perturbation across some events",
        protected=True,
        apply=small_perturbation,
        incremental_safe=True,
    ),
    Heuristic(
        id="split_resize",
        name="Re-split one event into different-sized pieces",
        protected=True,
        apply=lambda sol, inst, rng: split_resize_move(inst, sol, rng),
        incremental_safe=False,
    ),
]
