"""Acceptance criteria for local search: LAHC and Simulated Annealing.

Each criterion exposes three methods called by the solver loop:
  accepts(current_cost, candidate_cost, rng) -> bool   -- make accept/reject decision
  after_step(current_cost)                             -- update internal state; current_cost
                                                          is already the post-accept/reject value
  reset(current_cost)                                  -- reinitialise after a perturbation restart
"""

import math
import random
from typing import Protocol, runtime_checkable


@runtime_checkable
class AcceptanceCriterion(Protocol):
    """Protocol for pluggable acceptance criteria."""

    @property
    def name(self) -> str: ...

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random
    ) -> bool: ...

    def after_step(self, current_cost: int) -> None: ...

    def reset(self, current_cost: int) -> None: ...


class LAHCAcceptance:
    """Late Acceptance Hill Climbing (Burke & Bykov 2012).

    A candidate is accepted when it is no worse than the current solution OR
    no worse than the solution accepted L steps ago.
    """

    def __init__(self, history_length: int = 30) -> None:
        self._L = history_length
        self._history: list[int] = [0] * history_length
        self._step: int = 0

    @property
    def name(self) -> str:
        return f"lahc(L={self._L})"

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random
    ) -> bool:
        v = self._step % self._L
        return candidate_cost <= current_cost or candidate_cost <= self._history[v]

    def after_step(self, current_cost: int) -> None:
        v = self._step % self._L
        self._history[v] = current_cost
        self._step += 1

    def reset(self, current_cost: int) -> None:
        self._history = [current_cost] * self._L
        self._step = 0


class SimulatedAnnealingAcceptance:
    """Simulated Annealing with geometric cooling.

    Improvements are always accepted.  Worsenings are accepted with probability
    exp(-delta / T), where delta = candidate_cost - current_cost and T decreases
    geometrically: T <- max(t_end, T * cooling_rate) after every step.
    """

    def __init__(
        self,
        t_start: float = 10_000.0,
        t_end: float = 1.0,
        cooling_rate: float = 0.9999,
    ) -> None:
        if not (0.0 < cooling_rate < 1.0):
            raise ValueError(f"cooling_rate must be in (0, 1), got {cooling_rate}")
        if t_start <= 0 or t_end <= 0:
            raise ValueError("t_start and t_end must be positive")
        self._t_start = t_start
        self._t_end = t_end
        self._cooling_rate = cooling_rate
        self._T: float = t_start

    @property
    def name(self) -> str:
        return f"sa(T0={self._t_start:.0f},Tend={self._t_end:.1f},alpha={self._cooling_rate})"

    @property
    def temperature(self) -> float:
        return self._T

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random
    ) -> bool:
        if candidate_cost <= current_cost:
            return True
        delta = candidate_cost - current_cost
        return rng.random() < math.exp(-delta / self._T)

    def after_step(self, current_cost: int) -> None:
        self._T = max(self._t_end, self._T * self._cooling_rate)

    def reset(self, current_cost: int) -> None:
        self._T = self._t_start
