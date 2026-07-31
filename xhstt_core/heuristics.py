import random
from dataclasses import dataclass, replace
from typing import Callable

from xhstt_core.evaluator_ref import total_cost, valid_start_time_ids
from xhstt_core.model import Instance, Solution
from xhstt_core.moves import time_reassign_move, time_swap_move


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
    apply(solution, instance, rng) argument order."""
    return time_reassign_move(instance, solution, rng)


def _best_time_for_event(instance: Instance, solution: Solution, index: int) -> Solution:
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
    """Picks one solution event at random, then moves it to the valid
    start time that minimizes total_cost (see _best_time_for_event)."""
    if not solution.events:
        raise ValueError("cannot apply a move to a solution with no solution events")
    index = rng.randrange(len(solution.events))
    return _best_time_for_event(instance, solution, index)


def swap(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.time_swap_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Two events that happen
    to already share a time_ref produce a harmless structurally-valid
    no-op, not an error -- no special-casing needed."""
    return time_swap_move(instance, solution, rng)


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
]
