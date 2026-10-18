"""Structured JSON Lines logging for solver runs.

Emits one JSON object per line to a ``.jsonl`` file.  Event types:

- ``new_best`` — whenever the best-known cost improves.
- ``selector_window`` — periodic aggregate of heuristic usage/rewards over
  the last *window_size* iterations (emitted every *window_size* steps).
- ``perturbation`` — when the stagnation-triggered perturbation fires.
- ``run_config`` — once at the start: every solver and selector parameter.
- ``selector_state`` — periodically, at each perturbation and at the end:
  the selector's learned statistics (for UCB, Q and n per state and
  heuristic) plus the solver's state, stagnation and SA temperature.
- ``run_summary`` — single record at the end of a run (config + outcome).

Named ``solve_log`` (not ``logging``) so it never shadows the stdlib module.
"""

from __future__ import annotations

import json
import time
from pathlib import Path
from typing import Any, TextIO


class SolveLogger:
    """Appends JSON Lines records to *output*.

    Parameters
    ----------
    output:
        An open text-mode file (or any object with ``.write``).
        The caller owns the file handle — ``SolveLogger`` never opens or
        closes it itself, so it composes cleanly with ``with open(...)``
        blocks in ``run_solver.py``.
    window_size:
        How many iterations between ``selector_window`` snapshots.
    """

    def __init__(self, output: TextIO, *, window_size: int = 500) -> None:
        self._out = output
        self._window_size = window_size

        # ── per-window accumulators ──
        self._window_calls: dict[str, int] = {}
        self._window_reward_sum: dict[str, float] = {}
        self._window_accepted: dict[str, int] = {}
        # accepted candidates split by the sign of their cost change
        self._window_accepted_by_change: dict[str, list[int]] = {}
        self._window_start_step: int = 0

    # ── public API (called from lahc.py / run_solver.py) ──

    def log_new_best(
        self,
        *,
        step: int,
        heuristic_id: str,
        old_cost: int,
        new_cost: int,
        infeasibility: int,
        objective: int,
        elapsed: float,
    ) -> None:
        """Record a new global-best solution."""
        self._emit(
            "new_best",
            step=step,
            heuristic_id=heuristic_id,
            old_cost=old_cost,
            new_cost=new_cost,
            infeasibility=infeasibility,
            objective=objective,
            elapsed_s=round(elapsed, 3),
        )

    def log_perturbation(
        self,
        *,
        step: int,
        tier: int,
        heuristic_id: str,
        cost_before: int,
        cost_after: int,
    ) -> None:
        """Record a stagnation-triggered perturbation."""
        self._emit(
            "perturbation",
            step=step,
            tier=tier,
            heuristic_id=heuristic_id,
            cost_before=cost_before,
            cost_after=cost_after,
        )

    def log_run_config(self, **fields: Any) -> None:
        """Record the run's parameters (called once at the start)."""
        self._emit("run_config", **fields)

    def log_selector_state(self, **fields: Any) -> None:
        """Record a snapshot of what the selector has learned so far."""
        self._emit("selector_state", **fields)

    def log_run_summary(self, **fields: Any) -> None:
        """Record final run metadata (called once at the end)."""
        self._emit("run_summary", **fields)

    # ── per-step tracking (called from lahc loop) ──

    def record_step(
        self,
        *,
        step: int,
        heuristic_id: str,
        reward: float,
        accepted: bool,
        cost_change: int = 0,
    ) -> None:
        """Accumulate per-heuristic stats for the current window.

        *cost_change* is the candidate's cost minus the current solution's
        cost. Accepted candidates are counted separately as better (< 0),
        equal (== 0) and worse (> 0): an accepted candidate with equal cost
        moves the search sideways (or not at all), so a high acceptance rate
        alone does not show that the search is still making progress.

        When *step* crosses a window boundary, a ``selector_window``
        record is flushed automatically.
        """
        self._window_calls[heuristic_id] = self._window_calls.get(heuristic_id, 0) + 1
        self._window_reward_sum[heuristic_id] = (
            self._window_reward_sum.get(heuristic_id, 0.0) + reward
        )
        if accepted:
            self._window_accepted[heuristic_id] = (
                self._window_accepted.get(heuristic_id, 0) + 1
            )
            by_change = self._window_accepted_by_change.setdefault(
                heuristic_id, [0, 0, 0]
            )
            by_change[0 if cost_change < 0 else 1 if cost_change == 0 else 2] += 1

        if (step + 1) % self._window_size == 0:
            self._flush_window(step)

    def flush(self) -> None:
        """Flush any partial window (call at end of run)."""
        # Only flush if there is accumulated data
        if self._window_calls:
            self._flush_window(self._window_start_step + len(self._window_calls) - 1)

    # ── internals ──

    def _flush_window(self, last_step: int) -> None:
        heuristics: dict[str, dict[str, Any]] = {}
        for h_id in sorted(self._window_calls):
            n = self._window_calls[h_id]
            better, equal, worse = self._window_accepted_by_change.get(h_id, [0, 0, 0])
            heuristics[h_id] = {
                "calls": n,
                "accepted": self._window_accepted.get(h_id, 0),
                "accepted_better": better,
                "accepted_equal": equal,
                "accepted_worse": worse,
                "avg_reward": round(self._window_reward_sum.get(h_id, 0.0) / n, 6)
                if n
                else 0.0,
            }

        self._emit(
            "selector_window",
            window_start=self._window_start_step,
            window_end=last_step,
            heuristics=heuristics,
        )

        self._window_calls.clear()
        self._window_reward_sum.clear()
        self._window_accepted.clear()
        self._window_accepted_by_change.clear()
        self._window_start_step = last_step + 1

    def _emit(self, event: str, **data: Any) -> None:
        record = {"event": event, "t": time.time(), **data}
        self._out.write(json.dumps(record, separators=(",", ":")) + "\n")
