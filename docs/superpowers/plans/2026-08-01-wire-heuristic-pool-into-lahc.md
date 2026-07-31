# Wire Heuristic Pool Into LAHC Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `xhstt_core.lahc.run_lahc` (and therefore `run_solver.py`, the real CLI) draw its candidate moves at random from `xhstt_core.heuristics.MANUAL_HEURISTICS` instead of the hardcoded 3-function list in `moves.py`, with a 5th pool entry (`resource_reassign`) added so no move capability is lost.

**Architecture:** `xhstt_core/heuristics.py` gains one more thin wrapper (`resource_reassign`, wrapping `moves.resource_reassign_move`) so `MANUAL_HEURISTICS` becomes a complete 5-entry replacement for `moves.py`'s move set. `xhstt_core/lahc.py`'s `run_lahc` then imports `MANUAL_HEURISTICS`/`Heuristic` instead of the three `moves.py` functions directly, and its inner loop calls `heuristic.apply(current, instance, rng)` instead of `move_fn(instance, current, rng)`. No selection weighting/learning — `rng.choice` over the pool, exactly as today.

**Tech Stack:** Python 3.12, stdlib `dataclasses`/`random`, pytest.

## Global Constraints

- Spec: `docs/superpowers/specs/2026-08-01-wire-heuristic-pool-into-lahc-design.md` (read before starting).
- Selection stays pure `rng.choice(pool)` — no weights, no UCB, no learning. That's Etap 6, explicitly out of scope.
- `moves.py` is not modified — its three functions stay as the underlying implementations that `heuristics.py`'s wrappers delegate to.
- `run_solver.py` is not modified — it calls `run_lahc(...)` without `moves=`/`heuristics=`, so it picks up the new default automatically.
- `run_lahc`'s `moves` parameter is renamed to `heuristics` (type `list[Heuristic] | None`). Verified via repo-wide grep: no caller (production code or tests) currently passes `moves=` explicitly, so this rename has no other call sites to update.
- `tests/test_heuristics.py` needs no edits — its structural test is parametrized directly over `MANUAL_HEURISTICS` and will pick up the new 5th entry automatically (same mechanism already relied on for `swap`/`move_best`/`repair_hard_violation` in the prior plan).
- `tests/test_lahc.py` needs no edits either — **verified empirically before writing this plan**: `test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget` (`random.Random(0)` for `build_initial`, `random.Random(2)` for `run_lahc`, `history_length=30`, `max_iterations=40000`) still reaches `best_cost == 0` with the new 5-heuristic pool. It just gets slower: ~7.7s today, ~21.5s with the new pool (the two cost-aware heuristics evaluate `total_cost` per candidate time). This is the accepted tradeoff from the design spec — do not "fix" it by reducing `max_iterations` or touching the test.
- Test invocation: `uv run python -m pytest ...` (not bare `uv run pytest ...` — pre-existing repo quirk, see `CLAUDE.md`).
- New/changed code needs full type hints (repo's `mypy --strict` convention); verify with `uv run mypy --follow-imports=silent <file>` to avoid pre-existing unrelated errors elsewhere in the package.

---

## File Structure

- Modify: `xhstt_core/heuristics.py` — add `resource_reassign` wrapper + 5th `MANUAL_HEURISTICS` entry.
- Modify: `xhstt_core/lahc.py` — swap the default move source from `moves.py`'s 3 functions to `heuristics.MANUAL_HEURISTICS`.

---

### Task 1: Add `resource_reassign` to the heuristic pool

**Files:**
- Modify: `xhstt_core/heuristics.py`

**Interfaces:**
- Consumes: `xhstt_core.moves.resource_reassign_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution` (existing).
- Produces: `resource_reassign(solution: Solution, instance: Instance, rng: random.Random) -> Solution`, appended to `MANUAL_HEURISTICS` (final pool of 5: `move_random`, `move_best`, `swap`, `repair_hard_violation`, `resource_reassign`).

- [ ] **Step 1: Confirm the failing case is absent**

`tests/test_heuristics.py::test_heuristic_preserves_events_and_does_not_mutate_input` is parametrized directly over `MANUAL_HEURISTICS`, so it will pick up a 5th case (`resource_reassign`) automatically once it exists in that list — no test file edit needed this task.

Run: `uv run python -m pytest tests/test_heuristics.py -v --collect-only`
Expected: exactly 4 cases for the structural test (`move_random`, `move_best`, `swap`, `repair_hard_violation`) — no `resource_reassign` case yet.

- [ ] **Step 2: Implement `resource_reassign`**

In `xhstt_core/heuristics.py`, change the `moves` import (currently `from xhstt_core.moves import time_reassign_move, time_swap_move`) to:

```python
from xhstt_core.moves import resource_reassign_move, time_reassign_move, time_swap_move
```

Add this function immediately after `swap` (before `_movable_violating_indices`):

```python
def resource_reassign(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.resource_reassign_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Raises ValueError (via
    resource_reassign_move) if no event resource has a same-type alternative."""
    return resource_reassign_move(instance, solution, rng)
```

Add this entry at the end of `MANUAL_HEURISTICS` (after the `repair_hard_violation` entry):

```python
    Heuristic(
        id="resource_reassign",
        name="Reassign an event resource to a same-type alternative",
        protected=True,
        apply=resource_reassign,
    ),
```

- [ ] **Step 3: Run tests to verify it passes**

Run: `uv run python -m pytest tests/test_heuristics.py -v --collect-only`
Expected: structural test now has 5 parametrized cases, including `resource_reassign`.

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: PASS, all tests (10 total: 5 structural + `move_best` cost test + 4 `repair_hard_violation` tests).

- [ ] **Step 4: Commit**

```bash
git add xhstt_core/heuristics.py
git commit -m "feat: add resource_reassign to the manual heuristic pool"
```

---

### Task 2: Wire `MANUAL_HEURISTICS` into `run_lahc`

**Files:**
- Modify: `xhstt_core/lahc.py`

**Interfaces:**
- Consumes: `xhstt_core.heuristics.MANUAL_HEURISTICS: list[Heuristic]` (now 5 entries after Task 1), `xhstt_core.heuristics.Heuristic` (dataclass with `.apply(solution, instance, rng) -> Solution`).
- Produces: `run_lahc(..., heuristics: list[Heuristic] | None = None, ...)` — same return type and every other parameter unchanged; only the `moves` parameter is renamed/retyped and the pool source changes.

- [ ] **Step 1: Confirm current behavior before changing it**

Run: `uv run python -m pytest tests/test_lahc.py -v --no-cov --durations=0`
Expected: PASS, all 5 tests. Note the duration of `test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget` (should be a few seconds) — you'll compare against this after the change.

- [ ] **Step 2: Replace `xhstt_core/lahc.py` in full**

This file is small (66 lines) and nearly every part changes (imports, signature, two lines inside the loop, plus one pre-existing `ruff` line-length violation on the untouched `on_progress` block that gets caught because this task's final `ruff check` is file-scoped, not line-scoped — see Step 5). Replace the entire file content with:

```python
import random
import time
from collections.abc import Callable

from xhstt_core.evaluator_ref import total_cost
from xhstt_core.heuristics import MANUAL_HEURISTICS, Heuristic
from xhstt_core.model import Instance, Solution


def run_lahc(
    instance: Instance,
    initial: Solution,
    rng: random.Random,
    history_length: int = 30,
    max_iterations: int = 1000,
    heuristics: list[Heuristic] | None = None,
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
    pool = heuristics if heuristics is not None else MANUAL_HEURISTICS

    current = initial
    current_cost = total_cost(instance, current)
    best, best_cost = current, current_cost
    history = [current_cost] * history_length
    last_progress_time = time.monotonic()

    for step in range(max_iterations):
        heuristic = rng.choice(pool)
        try:
            candidate = heuristic.apply(current, instance, rng)
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
            if (
                (step + 1) % progress_every == 0
                or now - last_progress_time >= progress_seconds
            ):
                on_progress(step + 1, best_cost)
                last_progress_time = now

    return best, best_cost
```

Changes from the original, precisely: (1) the `moves.py` import and `_DEFAULT_MOVES` constant are gone, replaced by importing `MANUAL_HEURISTICS`/`Heuristic` from `xhstt_core.heuristics`; (2) `from typing import Callable` becomes `from collections.abc import Callable` (matches the convention already established in `xhstt_core/heuristics.py`, and fixes a pre-existing `ruff` `UP035` finding on this exact line); (3) the `moves: list | None = None` parameter becomes `heuristics: list[Heuristic] | None = None` (also fixes a pre-existing `mypy --strict` finding — bare `list` needs a type argument); (4) `move_fns = moves if moves is not None else _DEFAULT_MOVES` becomes `pool = heuristics if heuristics is not None else MANUAL_HEURISTICS`; (5) inside the loop, `move_fn = rng.choice(move_fns)` / `candidate = move_fn(instance, current, rng)` become `heuristic = rng.choice(pool)` / `candidate = heuristic.apply(current, instance, rng)`; (6) the `on_progress` trigger `if` is reformatted across multiple lines (behaviorally identical) to fix a pre-existing `ruff` `E501` line-too-long finding. The LAHC acceptance logic itself (the `history`/`current_cost`/`best_cost` block) is untouched.

- [ ] **Step 3: Run the affected tests**

Run: `uv run python -m pytest tests/test_lahc.py -v --no-cov --durations=0`
Expected: PASS, all 5 tests. `test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget` should still assert `best_cost == 0` (verified before this plan was written: it does, with the same seeds and the same 40000-iteration budget) but will take roughly 15-25s instead of a few seconds — this is expected, not a regression to chase. If it fails outright (not just slow), stop and report BLOCKED — the empirical verification behind this plan assumed no other changes; don't loosen the assertion or bump `max_iterations` on your own judgment.

- [ ] **Step 4: Run the rest of the suite for regressions**

Run: `uv run python -m pytest tests/test_heuristics.py tests/test_moves.py -v`
Expected: PASS. `moves.py` itself wasn't touched, so `test_moves.py` should be unaffected; `test_heuristics.py` should be unaffected by this task (it only imports from `heuristics.py`, not `lahc.py`).

Run: `uv run python -m pytest`
Expected: PASS, full suite, no regressions (the suite will take noticeably longer than before because of the full-feasibility test's new duration — that's expected, not a failure).

- [ ] **Step 5: Type-check and lint**

Run: `uv run mypy --follow-imports=silent xhstt_core/lahc.py`
Expected: `Success: no issues found in 1 source file`.

Run: `uv run ruff check xhstt_core/lahc.py`
Expected: `All checks passed!` (the two pre-existing findings on this file — `UP035` and the `E501` on the `on_progress` trigger — are already fixed by the Step 2 rewrite; this just confirms it).

- [ ] **Step 6: Manually verify `run_solver.py` still works end-to-end**

Run: `uv run python run_solver.py BR-SA-00 --iterations 500 --seed 0`
Expected: runs to completion without error, prints a cost breakdown and writes `output/BR-SA-00_solution.xml` + `output/BR-SA-00_timetable.html`. This is a real (if small) exercise of the new default pool through the actual CLI — no `moves=`/`heuristics=` override, so it must be using `MANUAL_HEURISTICS`.

- [ ] **Step 7: Commit**

```bash
git add xhstt_core/lahc.py
git commit -m "feat: wire MANUAL_HEURISTICS into run_lahc as the default move pool"
```
