import random
import time
from typing import Callable

from xhstt_core.evaluator_ref import total_cost
from xhstt_core.model import Instance, Solution
from xhstt_core.moves import resource_reassign_move, time_reassign_move, time_swap_move

_DEFAULT_MOVES = [time_reassign_move, time_swap_move, resource_reassign_move]


def run_lahc(
    instance: Instance,
    initial: Solution,
    rng: random.Random,
    history_length: int = 30,
    max_iterations: int = 1000,
    moves: list | None = None,
    on_progress: Callable[[int, int], None] | None = None,
    progress_every: int = 1000,
    progress_seconds: float = 2.0,
) -> tuple[Solution, int]:
    """Late Acceptance Hill Climbing (Burke & Bykov): a candidate move is
    accepted if it's no worse than the current solution OR no worse than
    the solution accepted `history_length` steps ago. Returns the best
    solution seen and its cost. A move step that cannot be applied (e.g.
    no reassignable resource in a tiny instance) is skipped.

    If `on_progress` is given, it's called as `on_progress(iteration,
    best_cost)` whenever `progress_every` completed iterations have passed
    OR at least `progress_seconds` have elapsed since the last call,
    whichever comes first -- on a large/slow instance, waiting for a fixed
    iteration count could mean minutes with no feedback at all, so a
    wall-clock heartbeat guarantees the caller hears something regularly
    regardless of instance size."""
    move_fns = moves if moves is not None else _DEFAULT_MOVES

    current = initial
    current_cost = total_cost(instance, current)
    best, best_cost = current, current_cost
    history = [current_cost] * history_length
    last_progress_time = time.monotonic()

    for step in range(max_iterations):
        move_fn = rng.choice(move_fns)
        try:
            candidate = move_fn(instance, current, rng)
        except ValueError:
            continue
        candidate_cost = total_cost(instance, candidate)

        v = step % history_length
        if candidate_cost <= current_cost or candidate_cost <= history[v]:
            current, current_cost = candidate, candidate_cost
            if current_cost < best_cost:
                best, best_cost = current, current_cost
        history[v] = current_cost

        if on_progress is not None:
            now = time.monotonic()
            if (step + 1) % progress_every == 0 or now - last_progress_time >= progress_seconds:
                on_progress(step + 1, best_cost)
                last_progress_time = now

    return best, best_cost
