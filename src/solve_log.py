"""Structured JSON Lines logging for solver runs.

Emits one JSON object per line to a ``.jsonl`` file.  Event types:

- ``new_best`` — whenever the best-known cost improves.
- ``selector_window`` — periodic aggregate of heuristic usage/rewards over
  the last *window_size* iterations (emitted every *window_size* steps).
- ``perturbation`` — when the stagnation-triggered perturbation fires,
  with the temperature the acceptance criterion was re-heated to.
- ``perturbation_outcome`` — at the next perturbation or the end of the
  run: whether the current cost got back to the best cost the perturbation
  started from (and after how many steps) and whether it led to a new best.
- ``run_environment`` — once at the start (run_solver.py): git commit,
  command line, Python version, CPU count -- what reproducing the run needs.
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
        self._window_last_step: int = 0
        # ── solver context (latest value seen in this window) ──
        self._window_start_elapsed: float | None = None
        self._window_last_elapsed: float | None = None
        self._ctx_current_inf: int | None = None
        self._ctx_current_obj: int | None = None
        self._ctx_best_inf: int | None = None
        self._ctx_best_obj: int | None = None
        self._ctx_temperature: float | None = None
        self._ctx_stagnation: int | None = None

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
        temperature: float | None = None,
    ) -> None:
        """Record a stagnation-triggered perturbation; *temperature* is the
        acceptance temperature the criterion was re-heated to (None for LAHC)."""
        self._emit(
            "perturbation",
            step=step,
            tier=tier,
            heuristic_id=heuristic_id,
            cost_before=cost_before,
            cost_after=cost_after,
            temperature=temperature,
        )

    def log_llm_pool_add(
        self,
        *,
        step: int,
        h_id: str,
        source: str,
        trigger_idx: int,
    ) -> None:
        """Record a heuristic being added to the pool from LLM (fresh or replay)."""
        self._emit(
            "llm_pool_add",
            step=step,
            h_id=h_id,
            source=source,
            trigger_idx=trigger_idx,
        )

    def log_perturbation_outcome(self, **fields: object) -> None:
        """Record what a perturbation led to (logged at the next perturbation
        or at the end of the run)."""
        self._emit("perturbation_outcome", **fields)

    def log_run_environment(self, **fields: object) -> None:
        """Record what is needed to reproduce the run: code version,
        command line, interpreter, machine (called once at the start)."""
        self._emit("run_environment", **fields)

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
        current_inf: int | None = None,
        current_obj: int | None = None,
        best_inf: int | None = None,
        best_obj: int | None = None,
        temperature: float | None = None,
        stagnation: int | None = None,
        elapsed_s: float | None = None,
    ) -> None:
        """Accumulate per-heuristic stats for the current window.

        *cost_change* is the candidate's cost minus the current solution's
        cost. Accepted candidates are counted separately as better (< 0),
        equal (== 0) and worse (> 0): an accepted candidate with equal cost
        moves the search sideways (or not at all), so a high acceptance rate
        alone does not show that the search is still making progress.

        The optional solver-context fields (current_inf/obj, best_inf/obj,
        temperature, stagnation, elapsed_s) are stored as the latest snapshot
        seen in this window and emitted with the ``selector_window`` record,
        enabling cost-trajectory and SA-temperature plots from the logs.

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

        # update solver context snapshot
        if current_inf is not None:
            self._ctx_current_inf = current_inf
        if current_obj is not None:
            self._ctx_current_obj = current_obj
        if best_inf is not None:
            self._ctx_best_inf = best_inf
        if best_obj is not None:
            self._ctx_best_obj = best_obj
        if temperature is not None:
            self._ctx_temperature = temperature
        if stagnation is not None:
            self._ctx_stagnation = stagnation
        if elapsed_s is not None:
            if self._window_start_elapsed is None:
                self._window_start_elapsed = elapsed_s
            self._window_last_elapsed = elapsed_s

        self._window_last_step = step
        if (step + 1) % self._window_size == 0:
            self._flush_window(step)

    def flush(self) -> None:
        """Flush any partial window (call at end of run)."""
        if self._window_calls:
            self._flush_window(self._window_last_step)

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

        iters_per_s: float | None = None
        if (
            self._window_start_elapsed is not None
            and self._window_last_elapsed is not None
            and self._window_last_elapsed > self._window_start_elapsed
        ):
            iters_per_s = round(
                self._window_size / (self._window_last_elapsed - self._window_start_elapsed),
                1,
            )

        self._emit(
            "selector_window",
            window_start=self._window_start_step,
            window_end=last_step,
            heuristics=heuristics,
            current_inf=self._ctx_current_inf,
            current_obj=self._ctx_current_obj,
            best_inf=self._ctx_best_inf,
            best_obj=self._ctx_best_obj,
            temperature=self._ctx_temperature,
            stagnation=self._ctx_stagnation,
            elapsed_s=round(self._window_last_elapsed, 3)
            if self._window_last_elapsed is not None
            else None,
            iters_per_s=iters_per_s,
        )

        self._window_calls.clear()
        self._window_reward_sum.clear()
        self._window_accepted.clear()
        self._window_accepted_by_change.clear()
        self._window_start_step = last_step + 1
        self._window_last_step = last_step + 1
        self._window_start_elapsed = None
        self._ctx_current_inf = None
        self._ctx_current_obj = None
        self._ctx_best_inf = None
        self._ctx_best_obj = None
        self._ctx_temperature = None
        self._ctx_stagnation = None

    def _emit(self, event: str, **data: Any) -> None:
        record = {"event": event, "t": time.time(), **data}
        self._out.write(json.dumps(record, separators=(",", ":")) + "\n")
