"""Acceptance criteria for local search: LAHC and Simulated Annealing.

Each criterion exposes three methods called by the solver loop:
  accepts(current_cost, candidate_cost, rng, step) -> bool
  after_step(current_cost, step)
  reset(current_cost)

`step` is the solver loop's iteration index, passed in rather than counted
by the criterion because the loop also advances on failed moves and restarts.
"""

import math
import random
import time
from typing import Protocol, runtime_checkable


@runtime_checkable
class AcceptanceCriterion(Protocol):
    """Protocol for pluggable acceptance criteria."""

    @property
    def name(self) -> str: ...

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random, step: int
    ) -> bool: ...

    def after_step(self, current_cost: int, step: int) -> None: ...

    def reset(self, current_cost: int) -> None: ...


class LAHCAcceptance:
    """Late Acceptance Hill Climbing (Burke & Bykov 2012).

    A candidate is accepted when it is no worse than the current solution OR
    no worse than the solution accepted L steps ago.
    """

    def __init__(self, history_length: int = 30) -> None:
        self._L = history_length
        self._history: list[int] = [0] * history_length

    @property
    def name(self) -> str:
        return f"lahc(L={self._L})"

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random, step: int
    ) -> bool:
        v = step % self._L
        return candidate_cost <= current_cost or candidate_cost <= self._history[v]

    def after_step(self, current_cost: int, step: int) -> None:
        v = step % self._L
        self._history[v] = current_cost

    def reset(self, current_cost: int) -> None:
        self._history = [current_cost] * self._L


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
        return (
            f"sa(T0={self._t_start:.0f},"
            f"Tend={self._t_end:.1f},alpha={self._cooling_rate})"
        )

    @property
    def temperature(self) -> float:
        return self._T

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random, step: int
    ) -> bool:
        if candidate_cost <= current_cost:
            return True
        delta = candidate_cost - current_cost
        return rng.random() < math.exp(-delta / self._T)

    def after_step(self, current_cost: int, step: int) -> None:
        self._T = max(self._t_end, self._T * self._cooling_rate)

    def reset(self, current_cost: int) -> None:
        self._T = self._t_start


class AutoSAAcceptance:
    """Simulated Annealing with time-proportional cooling and auto-calibrated T₀.

    The default t0_factor (0.01, raised from 0.001) came out best in a sweep
    over 1e-2..1e-5 on GR-H1-97, GR-PA-08, ZA-WD-09 and BR-SA-00: colder
    starts left more hard violations, monotonically.

    T₀ is set to `initial_cost * t0_factor` on the first `reset()` call, so
    it scales automatically with the problem's cost magnitude — no per-instance
    tuning required.

    Temperature follows an exponential decay keyed to wall-clock time rather
    than iteration count:

        T(t) = T₀ * (t_end / T₀) ** (elapsed / time_budget)

    This makes the cooling schedule invariant to instance speed: a fast instance
    (many iterations in 120 s) and a slow one (few iterations) both see T₀ at
    the start and t_end at the end of the budget.

    With `iteration_budget` set, the same decay is keyed to the step number
    instead of the clock -- T(step) = T₀ * (t_end / T₀) ** (steps since the
    last reset / iteration_budget) -- so a run is reproducible from its
    seed. That is the mode for iteration-bounded runs (run_solver.py
    without --time-limit); `time_budget` is then ignored.

    After perturbation restarts the criterion re-heats to T₀ and resets its
    internal clock (or step counter), matching LAHCAcceptance.reset().

    `reheat` decides how long the schedule after such a restart lasts:
    "full" (the default, and the only behaviour before 18.10.2026) cools
    over the whole budget again; "remaining" cools over what is left of the
    budget since the first reset(), so the run still ends at t_end. With
    "full" and perturbations every few thousand steps, a long run never
    cools down: on the large instances the temperature stayed in the
    millions for the whole hour and the search stopped improving.
    """

    REHEAT_MODES = ("full", "remaining")

    def __init__(
        self,
        t0_factor: float = 0.01,
        t_end: float = 1.0,
        time_budget: float = 120.0,
        iteration_budget: int | None = None,
        reheat: str = "full",
    ) -> None:
        if t0_factor <= 0:
            raise ValueError(f"t0_factor must be positive, got {t0_factor}")
        if t_end <= 0:
            raise ValueError(f"t_end must be positive, got {t_end}")
        if time_budget <= 0:
            raise ValueError(f"time_budget must be positive, got {time_budget}")
        if iteration_budget is not None and iteration_budget <= 0:
            raise ValueError(
                f"iteration_budget must be positive, got {iteration_budget}"
            )
        if reheat not in self.REHEAT_MODES:
            raise ValueError(
                f"reheat must be one of {self.REHEAT_MODES}, got {reheat!r}"
            )
        self._reheat = reheat
        # length of the current cooling schedule: the whole budget, or (after
        # a "remaining" re-heat) what was left of it
        self._schedule_time = time_budget
        self._schedule_steps = iteration_budget
        # clock time / step of the first reset(), where the budget starts
        self._run_start: float | None = None
        self._run_start_step = 0
        self._t0_factor = t0_factor
        self._t_end = t_end
        self._time_budget = time_budget
        self._t0: float = max(t_end, 1.0)
        self._start: float = time.monotonic()
        self._iteration_budget = iteration_budget
        self._last_step = -1  # last step seen by after_step()
        self._start_step = 0  # step at which the last reset() happened

    @property
    def name(self) -> str:
        budget = (
            f"{self._iteration_budget}it"
            if self._iteration_budget is not None
            else f"{self._time_budget:.0f}s"
        )
        reheat = "" if self._reheat == "full" else f",reheat={self._reheat}"
        return (
            f"sa_auto(t0_factor={self._t0_factor},t_end={self._t_end},"
            f"budget={budget}{reheat})"
        )

    def temperature_for(self, step: int) -> float:
        """Current temperature in either mode -- the step is used only in
        iteration-budget mode."""
        if self._iteration_budget is not None:
            return self.temperature_at(step)
        return self.temperature

    def temperature_at(self, step: int) -> float:
        """Temperature for `step` in iteration-budget mode."""
        assert self._schedule_steps is not None
        ratio = min(1.0, (step - self._start_step) / self._schedule_steps)
        return float(self._t0 * (self._t_end / self._t0) ** ratio)

    @property
    def temperature(self) -> float:
        elapsed = time.monotonic() - self._start
        ratio = min(1.0, elapsed / self._schedule_time)
        return float(self._t0 * (self._t_end / self._t0) ** ratio)

    def accepts(
        self, current_cost: int, candidate_cost: int, rng: random.Random, step: int
    ) -> bool:
        if candidate_cost <= current_cost:
            return True
        T = (
            self.temperature_at(step)
            if self._iteration_budget is not None
            else self.temperature
        )
        if T <= 0.0:
            return False
        delta = candidate_cost - current_cost
        return rng.random() < math.exp(-delta / T)

    def after_step(self, current_cost: int, step: int) -> None:
        # temperature comes from the clock or the step number; only the
        # step is remembered, so reset() knows where a new schedule starts
        self._last_step = step

    def reset(self, current_cost: int) -> None:
        self._t0 = max(self._t_end, current_cost * self._t0_factor)
        self._start = time.monotonic()
        self._start_step = self._last_step + 1
        if self._run_start is None:
            self._run_start = self._start
            self._run_start_step = self._start_step
        if self._reheat == "remaining":
            # what is left of the budget; never zero, so the next schedule
            # still starts at T₀ (and, past the budget, sits at t_end)
            self._schedule_time = max(
                1e-9, self._time_budget - (self._start - self._run_start)
            )
            if self._iteration_budget is not None:
                self._schedule_steps = max(
                    1,
                    self._iteration_budget - (self._start_step - self._run_start_step),
                )
