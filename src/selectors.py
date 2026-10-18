import math
import random
from typing import Protocol

from src.heuristics import Heuristic

# Discrete solver state for the contextual UCB selector.
# (hard_violations_bucket, stagnation_bucket)
#   hard_violations_bucket: 1 = feasible (no hard violations), 0 = infeasible
#   stagnation_bucket:      0 = low, 1 = medium, 2 = high
# Produces 2 x 3 = 6 distinct states.
SolverState = tuple[int, int]

_ALL_STATES_6: tuple[SolverState, ...] = tuple(
    (h, s) for h in range(2) for s in range(3)
)
_ALL_STATES_2: tuple[SolverState, ...] = ((0, 0), (1, 0))

_POOL_LIMIT = 20
_MIN_PULLS_BEFORE_REMOVAL = 100
_OPTIMISTIC_Q = 0.5


class HeuristicSelector(Protocol):
    def select(
        self,
        heuristics: list[Heuristic],
        rng: random.Random,
        state: SolverState | None = None,
    ) -> Heuristic:
        """Select a heuristic to apply in the current iteration."""
        ...

    def update(
        self,
        heuristic: Heuristic,
        reward: float,
        state: SolverState | None = None,
    ) -> None:
        """Inform the selector about the outcome of the chosen heuristic."""
        ...


class RandomSelector:
    """Classic random selection. Ignores the reward signal."""

    def select(
        self,
        heuristics: list[Heuristic],
        rng: random.Random,
        state: SolverState | None = None,
    ) -> Heuristic:
        return rng.choice(heuristics)

    def update(
        self,
        heuristic: Heuristic,
        reward: float,
        state: SolverState | None = None,
    ) -> None:
        pass


class EpsilonGreedySelector:
    """Selects the best heuristic with probability 1-epsilon, else randomly.
    Uses Exponential Moving Average (EMA) to react to non-stationarity."""

    def __init__(self, epsilon: float = 0.1, alpha: float = 0.05) -> None:
        self.epsilon = epsilon
        self.alpha = alpha
        self._q_values: dict[str, float] = {}

    def select(
        self,
        heuristics: list[Heuristic],
        rng: random.Random,
        state: SolverState | None = None,
    ) -> Heuristic:
        if not heuristics:
            raise ValueError("Heuristic pool cannot be empty.")

        for h in heuristics:
            if h.id not in self._q_values:
                self._q_values[h.id] = 0.0

        if rng.random() < self.epsilon:
            return rng.choice(heuristics)

        max_q = max(self._q_values[h.id] for h in heuristics)
        best_heuristics = [h for h in heuristics if self._q_values[h.id] == max_q]
        return rng.choice(best_heuristics)

    def update(
        self,
        heuristic: Heuristic,
        reward: float,
        state: SolverState | None = None,
    ) -> None:
        old_q = self._q_values.get(heuristic.id, 0.0)
        self._q_values[heuristic.id] = (1.0 - self.alpha) * old_q + self.alpha * reward


class UCBSelector:
    """Contextual UCB selector with per-state statistics.

    Maintains a (SolverState, heuristic_id) → (Q, n) table, where Q is an
    Exponential Moving Average of observed rewards and n is a raw pull count.
    The exploration bonus uses the standard UCB1 formula: Q + c*sqrt(ln N / n),
    where N is the total number of pulls in the current state.

    When `collapse_stagnation=True` the 6-state context (2 x 3) is reduced to
    2 states (feasible / infeasible), cutting the number of (state, heuristic)
    cells from 54 to 18 and speeding up convergence on instances with a small
    iteration budget.

    New heuristics are initialised with an optimistic Q (`initial_q`) in all
    states, ensuring they are explored before any established heuristic is
    selected again (the protection period). When the pool exceeds `pool_limit`,
    `try_remove_weakest` identifies the lowest-average-Q non-protected
    heuristic that has been pulled at least `min_pulls_before_removal` times.

    The default `c` was calibrated for the binary reward (0, 1, 1 + bonus) on
    BR-SA-00 and ZA-WD-09 (c in 0.02..1.0, 5 seeds, 60 s): 0.02 was best or
    near-best on both, though differences between c values were within
    seed-to-seed noise.
    """

    def __init__(
        self,
        c: float = 0.02,
        alpha: float = 0.08,
        pool_limit: int = _POOL_LIMIT,
        min_pulls_before_removal: int = _MIN_PULLS_BEFORE_REMOVAL,
        initial_q: float = _OPTIMISTIC_Q,
        collapse_stagnation: bool = False,
    ) -> None:
        self.c = c
        self.alpha = alpha
        self.pool_limit = pool_limit
        self.min_pulls_before_removal = min_pulls_before_removal
        self.initial_q = initial_q
        self.collapse_stagnation = collapse_stagnation
        # 2 states when collapse_stagnation=True, 6 otherwise
        self._all_states: tuple[SolverState, ...] = (
            _ALL_STATES_2 if collapse_stagnation else _ALL_STATES_6
        )
        # (state, h_id) -> (Q, n)
        self._stats: dict[tuple[SolverState, str], tuple[float, float]] = {}
        # state -> total pulls in that state (sum of n across all heuristics)
        self._total_n: dict[SolverState, float] = {}

    def _resolve_state(self, state: SolverState) -> SolverState:
        """Map incoming state to internal state (strips stagnation if collapsed)."""
        if self.collapse_stagnation:
            return (state[0], 0)
        return state

    def _init_heuristic(self, h_id: str) -> None:
        """Lazily create state slots for a heuristic with optimistic Q."""
        for s in self._all_states:
            key = (s, h_id)
            if key not in self._stats:
                self._stats[key] = (self.initial_q, 0.0)

    def add_heuristic(
        self, heuristic: Heuristic, initial_q: float | None = None
    ) -> None:
        """Explicitly register a heuristic with optimistic initialisation.

        Called by the LLM pipeline (Etap 7) when a new heuristic enters the
        pool at a synchronisation point. The explicit `initial_q` overrides the
        instance default when a caller wants a specific value."""
        q = initial_q if initial_q is not None else self.initial_q
        for s in self._all_states:
            key = (s, heuristic.id)
            if key not in self._stats:
                self._stats[key] = (q, 0.0)

    def select(
        self,
        heuristics: list[Heuristic],
        rng: random.Random,
        state: SolverState | None = None,
    ) -> Heuristic:
        if not heuristics:
            raise ValueError("Heuristic pool cannot be empty.")

        s: SolverState = self._resolve_state(
            state if state is not None else (0, 0)
        )

        for h in heuristics:
            self._init_heuristic(h.id)

        # Forced exploration: any heuristic never tried in this state goes first.
        unpulled = [h for h in heuristics if self._stats[(s, h.id)][1] == 0.0]
        if unpulled:
            return rng.choice(unpulled)

        total_n = self._total_n.get(s, 0.0)
        ln_n = math.log(total_n) if total_n > 1.0 else 0.0

        scores: dict[str, float] = {}
        for h in heuristics:
            q, n = self._stats[(s, h.id)]
            scores[h.id] = q + self.c * math.sqrt(ln_n / n)

        max_score = max(scores.values())
        best = [h for h in heuristics if scores[h.id] == max_score]
        return rng.choice(best)

    def update(
        self,
        heuristic: Heuristic,
        reward: float,
        state: SolverState | None = None,
    ) -> None:
        s: SolverState = self._resolve_state(
            state if state is not None else (0, 0)
        )
        self._init_heuristic(heuristic.id)
        key = (s, heuristic.id)
        old_q, old_n = self._stats[key]
        self._stats[key] = (
            (1.0 - self.alpha) * old_q + self.alpha * reward,
            old_n + 1.0,
        )
        self._total_n[s] = self._total_n.get(s, 0.0) + 1.0

    def total_pulls(self, heuristic: Heuristic) -> float:
        """Total pull count for a heuristic summed across all states."""
        return sum(
            self._stats.get((s, heuristic.id), (0.0, 0.0))[1]
            for s in self._all_states
        )

    def mean_q(self, heuristic: Heuristic) -> float:
        """Average Q-value across all active states for a heuristic."""
        qs = [
            self._stats[(s, heuristic.id)][0]
            for s in self._all_states
            if (s, heuristic.id) in self._stats
        ]
        return sum(qs) / len(qs) if qs else 0.0

    def try_remove_weakest(self, heuristics: list[Heuristic]) -> Heuristic | None:
        """Remove the weakest heuristic from the pool if it exceeds the size limit.

        Conditions for a heuristic to be eligible for removal:
        - pool is larger than `pool_limit`
        - heuristic is not `protected`
        - heuristic has been pulled at least `min_pulls_before_removal` times

        Returns the heuristic to remove (caller must update their pool), or
        None if no removal is warranted."""
        if len(heuristics) <= self.pool_limit:
            return None

        removable = [
            h
            for h in heuristics
            if not h.protected and self.total_pulls(h) > self.min_pulls_before_removal
        ]
        if not removable:
            return None

        return min(removable, key=self.mean_q)
