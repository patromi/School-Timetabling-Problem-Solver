import random
from dataclasses import dataclass
from typing import Callable

from xhstt_core.model import Instance, Solution
from xhstt_core.moves import time_reassign_move


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


MANUAL_HEURISTICS: list[Heuristic] = [
    Heuristic(
        id="move_random",
        name="Random time reassignment",
        protected=True,
        apply=move_random,
    ),
]
