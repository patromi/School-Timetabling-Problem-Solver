import random
from collections.abc import Callable
from dataclasses import dataclass, replace

from xhstt_core.evaluator_ref import (
    _assigned_resource_ids,
    _events_in_applies_to,
    _resources_in_applies_to,
    evaluate_constraint,
    resolve_occurrences,
    total_cost,
    valid_start_time_ids,
)
from xhstt_core.model import Instance, Solution
from xhstt_core.moves import (
    kempe_chain_move,
    resource_reassign_move,
    time_reassign_move,
    time_swap_move,
)


@dataclass(frozen=True)
class Heuristic:
    """One entry in the manual heuristic pool -- wraps a plain apply()
    function with the identity/metadata the future RL selector (Etap 6)
    needs (id for its per-heuristic stats, protected so it's never pruned).
    """

    id: str
    name: str
    protected: bool
    apply: Callable[[Solution, Instance, random.Random], Solution]


def move_random(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.time_reassign_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Raises ValueError (via
    time_reassign_move) if solution has no events, or the chosen event has
    no alternative valid time to move to."""
    return time_reassign_move(instance, solution, rng)


def _best_time_for_event(
    instance: Instance, solution: Solution, index: int
) -> Solution:
    """Returns the solution obtained by moving solution.events[index] to
    whichever valid start time (INCLUDING its current one) yields the
    lowest total_cost. Including the current time means the result is
    never worse than the input -- move_best and repair_hard_violation both
    depend on this to guarantee they never regress the solution."""
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
    candidates = valid_start_time_ids(instance, event.duration)
    if not candidates:
        raise ValueError(f"event {event.event_ref!r} has no valid start time")

    best_solution: Solution | None = None
    best_cost: int | None = None
    for time_ref in candidates:
        new_events = list(solution.events)
        new_events[index] = replace(event, time_ref=time_ref)
        candidate_solution = replace(solution, events=new_events)
        cost = total_cost(instance, candidate_solution)
        if best_cost is None or cost < best_cost:
            best_cost = cost
            best_solution = candidate_solution
    assert best_solution is not None
    return best_solution


def move_best(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
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
    return _best_time_for_event(instance, solution, index)


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


_RUIN_FRACTION = 0.1
_RUIN_MIN_EVENTS = 2
_RUIN_MAX_EVENTS = 6


def ruin_and_recreate(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    """"Ruin-and-recreate" perturbation (Schrimpf et al. 1998): "ruins" a
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
    for i in recreate_order:
        current = _best_time_for_event(instance, current, i)
    return current


def _movable_violating_indices(instance: Instance, solution: Solution) -> list[int]:
    """Indices into solution.events whose event participates in at least
    one violated Required constraint (via either an event-scoped or a
    resource-scoped AppliesTo -- e.g. AvoidClashesConstraint applies to
    Resources/ResourceGroups, not Events, so it needs the resource-side
    resolution too) AND is actually movable: it has a time_ref set (a
    fully preassigned event never appears in solution.events at all, and
    a resources-only entry -- see construct.build_initial -- has
    time_ref=None; both are excluded here) and a non-empty set of valid
    alternative start times."""
    occurrences = resolve_occurrences(instance, solution)
    violated_event_ids: set[str] = set()
    for constraint in instance.constraints:
        if not constraint.required:
            continue
        if evaluate_constraint(instance, occurrences, constraint) <= 0:
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
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    """Picks one (movable) event involved in a violated Required
    constraint at random and moves it to its best available time (see
    _best_time_for_event) -- never a purely random move, so the operator
    actually tends to repair rather than just perturb."""
    candidates = _movable_violating_indices(instance, solution)
    if not candidates:
        raise ValueError("no movable event participates in a hard constraint violation")
    index = rng.choice(candidates)
    return _best_time_for_event(instance, solution, index)


MANUAL_HEURISTICS: list[Heuristic] = [
    Heuristic(
        id="move_random",
        name="Random time reassignment",
        protected=True,
        apply=move_random,
    ),
    Heuristic(
        id="move_best",
        name="Best-slot time reassignment",
        protected=True,
        apply=move_best,
    ),
    Heuristic(
        id="swap",
        name="Swap two events' times",
        protected=True,
        apply=swap,
    ),
    Heuristic(
        id="repair_hard_violation",
        name="Repair a hard constraint violation",
        protected=True,
        apply=repair_hard_violation,
    ),
    Heuristic(
        id="resource_reassign",
        name="Reassign an event resource to a same-type alternative",
        protected=True,
        apply=resource_reassign,
    ),
    Heuristic(
        id="kempe_chain",
        name="Kempe chain interchange between two times",
        protected=True,
        apply=kempe_chain,
    ),
    Heuristic(
        id="ruin_and_recreate",
        name="Ruin a small portion of the solution and greedily rebuild it",
        protected=True,
        apply=ruin_and_recreate,
    ),
]
