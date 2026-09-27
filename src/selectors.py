import math
import random
from typing import Protocol

from src.heuristics import Heuristic


class HeuristicSelector(Protocol):
    def select(self, heuristics: list[Heuristic], rng: random.Random) -> Heuristic:
        """Selects a heuristic to apply in the current iteration."""
        ...

    def update(self, heuristic: Heuristic, reward: float) -> None:
        """Informs the selector about the outcome of the chosen heuristic."""
        ...


class RandomSelector:
    """Classic random selection. Ignores the reward signal."""

    def select(self, heuristics: list[Heuristic], rng: random.Random) -> Heuristic:
        return rng.choice(heuristics)

    def update(self, heuristic: Heuristic, reward: float) -> None:
        pass


class EpsilonGreedySelector:
    """Selects the best heuristic with probability 1-epsilon, otherwise selects randomly.
    Uses an Exponential Moving Average (EMA) to react to non-stationarity."""

    def __init__(self, epsilon: float = 0.1, alpha: float = 0.05):
        self.epsilon = epsilon
        self.alpha = alpha
        self._q_values: dict[str, float] = {}

    def select(self, heuristics: list[Heuristic], rng: random.Random) -> Heuristic:
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

    def update(self, heuristic: Heuristic, reward: float) -> None:
        old_q = self._q_values.get(heuristic.id, 0.0)
        self._q_values[heuristic.id] = (1.0 - self.alpha) * old_q + self.alpha * reward


class UCBSelector:
    """Discounted Upper Confidence Bound (D-UCB).
    By discounting history (EMA), the selector adapts to the non-stationarity of the problem,
    i.e., changes in heuristic effectiveness during different phases of the search."""

    def __init__(self, c: float = 0.5, alpha: float = 0.05):
        self.c = c
        self.alpha = alpha
        self._q_values: dict[str, float] = {}
        self._n_values: dict[str, float] = {}

    def select(self, heuristics: list[Heuristic], rng: random.Random) -> Heuristic:
        if not heuristics:
            raise ValueError("Heuristic pool cannot be empty.")

        # Step 1: Forced exploration of unpulled heuristics
        unpulled = [h for h in heuristics if self._n_values.get(h.id, 0.0) == 0.0]
        if unpulled:
            return rng.choice(unpulled)

        # Step 2: Selection based on Upper Confidence Bound
        t = sum(self._n_values.values())
        ln_t = math.log(t) if t > 1.0 else 0.0

        scores = {}
        for h in heuristics:
            q = self._q_values[h.id]
            n = self._n_values[h.id]
            # UCB1 formula with discounted N and Q values
            scores[h.id] = q + self.c * math.sqrt(ln_t / n)

        max_score = max(scores.values())
        best_heuristics = [h for h in heuristics if scores[h.id] == max_score]
        return rng.choice(best_heuristics)

    def update(self, heuristic: Heuristic, reward: float) -> None:
        # Discount history for all heuristics
        for h_id in self._n_values:
            self._n_values[h_id] *= (1.0 - self.alpha)
            
        # Update values for the selected heuristic
        self._n_values[heuristic.id] = self._n_values.get(heuristic.id, 0.0) + 1.0
        
        old_q = self._q_values.get(heuristic.id, 0.0)
        self._q_values[heuristic.id] = (1.0 - self.alpha) * old_q + self.alpha * reward
