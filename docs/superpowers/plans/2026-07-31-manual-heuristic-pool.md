# Manual Heuristic Pool (move/swap/repair) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a formal `Heuristic` contract and a pool of four manual heuristics (`move_random`, `move_best`, `swap`, `repair_hard_violation`) implementing `apply(solution, instance, rng) -> Solution`, covering GitHub issues #28, #29, #32.

**Architecture:** One new module `xhstt_core/heuristics.py` wraps the two existing `moves.py` operators (`time_reassign_move`, `time_swap_move`) behind the new argument order, and adds a genuinely new best-slot search (`_best_time_for_event`) shared by `move_best` and the new `repair_hard_violation`. `moves.py` and `lahc.py` are not touched — the pool isn't wired into the solve loop yet.

**Tech Stack:** Python 3.12, stdlib `dataclasses`/`random`/`typing`, pytest.

## Global Constraints

- Spec: `docs/superpowers/specs/2026-07-31-manual-heuristic-pool-design.md` (read before starting; this plan implements it verbatim, including its "Korekta" section).
- `apply(solution, instance, rng) -> Solution` — argument order is `(solution, instance, rng)`, the reverse of `moves.py`'s `move_fn(instance, solution, rng)`. Do not "fix" this to match `moves.py`; it's the deliberate target contract.
- Never mutate the input `Solution` (or any `SolutionEvent`/list inside it). Build new objects via `dataclasses.replace` + structural sharing, exactly like `moves.py` does today.
- An unapplicable heuristic call raises `ValueError` (never returns `None`, never silently no-ops).
- `_best_time_for_event`'s candidate set for a given event **includes its current `time_ref`** — never excludes it. This is required so `move_best`/`repair_hard_violation` can never produce a solution worse than their input (see spec's "Korekta" section).
- All four `Heuristic` entries get `protected=True`.
- No delta evaluation: cost comparisons use `evaluator_ref.total_cost` (full re-evaluation) per candidate. This is intentional and already accepted in the spec — do not attempt to add incremental scoring.
- `moves.py` and `lahc.py` are out of scope: do not edit them.
- New code needs full type hints (repo's `mypy --strict` convention) even though the default `uv run mypy src/` command doesn't reach `xhstt_core/` — verify explicitly with `uv run mypy --follow-imports=silent xhstt_core/heuristics.py` instead (see Task 4; plain `--follow-imports` default would also surface ~18 pre-existing errors in other `xhstt_core` files that are not this task's to fix).
- Docstrings/comments in `xhstt_core/*.py` are in English (matches every existing file in that package); this is a library module, not user-facing CLI output, so no Polish strings are needed here.
- **Test invocation:** use `uv run python -m pytest ...`, not bare `uv run pytest ...`. Confirmed on this repo: `pyproject.toml` has no `[build-system]` table, so `uv run pytest`'s console-script entry point never gets the repo root on `sys.path` and `xhstt_core` fails to import (`ModuleNotFoundError`). Running via `python -m pytest` adds the current directory to `sys.path`, which fixes it — every existing test passes this way (118 passed, confirmed on `main` before this plan started). This is a known pre-existing repo-infra gap (see open GitHub issue #60 "dodanie uv"), not something to fix as part of this plan.

---

## File Structure

- Create: `xhstt_core/heuristics.py` — `Heuristic` dataclass, `move_random`, `_best_time_for_event`, `move_best`, `swap`, `_movable_violating_indices`, `repair_hard_violation`, `MANUAL_HEURISTICS`.
- Create: `tests/test_heuristics.py` — structural test parametrized over `MANUAL_HEURISTICS`, plus targeted tests for `move_best` and `repair_hard_violation`.

---

### Task 1: `Heuristic` contract + `move_random`

**Files:**
- Create: `xhstt_core/heuristics.py`
- Create: `tests/test_heuristics.py`

**Interfaces:**
- Consumes: `xhstt_core.moves.time_reassign_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution` (existing); `xhstt_core.construct.build_initial(instance: Instance, rng: random.Random) -> Solution` (existing, for test fixtures); `xhstt_core.parser.parse_archive(xml_text: str) -> list[Instance]` (existing).
- Produces: `Heuristic` dataclass (fields `id: str`, `name: str`, `protected: bool`, `apply: Callable[[Solution, Instance, random.Random], Solution]`); `move_random(solution: Solution, instance: Instance, rng: random.Random) -> Solution`; `MANUAL_HEURISTICS: list[Heuristic]` (will grow in later tasks — Task 1 leaves it with one entry). Later tasks and the structural test in this task both import `MANUAL_HEURISTICS` directly, so they automatically pick up every heuristic added after this task.

- [ ] **Step 1: Write the failing structural test**

Create `tests/test_heuristics.py`:

```python
import random
from pathlib import Path

import pytest

from xhstt_core.construct import build_initial
from xhstt_core.heuristics import MANUAL_HEURISTICS
from xhstt_core.model import Solution
from xhstt_core.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _sudoku_solution() -> tuple:
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    return instance, solution


def _snapshot(solution: Solution) -> list:
    return [
        (
            se.event_ref,
            se.time_ref,
            se.duration,
            tuple((r.role, r.resource_ref) for r in se.resources),
        )
        for se in solution.events
    ]


@pytest.mark.parametrize("heuristic", MANUAL_HEURISTICS, ids=lambda h: h.id)
def test_heuristic_preserves_events_and_does_not_mutate_input(heuristic) -> None:
    instance, solution = _sudoku_solution()
    before = _snapshot(solution)

    new_solution = None
    for seed in range(20):
        try:
            new_solution = heuristic.apply(solution, instance, random.Random(seed))
            break
        except ValueError:
            continue
    assert new_solution is not None, (
        f"{heuristic.id}: no seed in range(20) produced an applicable move"
    )

    assert _snapshot(solution) == before, f"{heuristic.id} mutated its input solution"
    assert isinstance(new_solution, Solution), f"{heuristic.id} did not return a Solution"
    assert sorted(se.event_ref for se in new_solution.events) == sorted(
        se.event_ref for se in solution.events
    ), f"{heuristic.id} lost or duplicated an event"
```

- [ ] **Step 2: Run test to verify it fails on import**

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: FAIL/ERROR — `ModuleNotFoundError: No module named 'xhstt_core.heuristics'` (the module doesn't exist yet).

- [ ] **Step 3: Implement `Heuristic` and `move_random`**

Create `xhstt_core/heuristics.py`:

```python
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
```

- [ ] **Step 4: Run test to verify it passes**

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: PASS (1 test, parametrized with 1 case: `move_random`).

- [ ] **Step 5: Commit**

```bash
git add xhstt_core/heuristics.py tests/test_heuristics.py
git commit -m "feat: add Heuristic contract and move_random wrapper"
```

---

### Task 2: `_best_time_for_event` + `move_best`

**Files:**
- Modify: `xhstt_core/heuristics.py`
- Modify: `tests/test_heuristics.py`

**Interfaces:**
- Consumes: `xhstt_core.evaluator_ref.valid_start_time_ids(instance: Instance, duration: int) -> tuple[str, ...]` (existing); `xhstt_core.evaluator_ref.total_cost(instance: Instance, solution: Solution) -> int` (existing).
- Produces: `_best_time_for_event(instance: Instance, solution: Solution, index: int) -> Solution` (module-private, reused by Task 4's `repair_hard_violation`); `move_best(solution: Solution, instance: Instance, rng: random.Random) -> Solution`, appended to `MANUAL_HEURISTICS`.

- [ ] **Step 1: Write the failing cost test**

Append to `tests/test_heuristics.py` (add `total_cost` to the existing `from xhstt_core.evaluator_ref import ...` — there isn't one yet, so add a new import line, and import `move_best` directly):

```python
from xhstt_core.evaluator_ref import total_cost
from xhstt_core.heuristics import move_best  # add alongside the MANUAL_HEURISTICS import


def test_move_best_never_increases_total_cost() -> None:
    instance, solution = _sudoku_solution()
    before = total_cost(instance, solution)

    ran_at_least_once = False
    for seed in range(20):
        new_solution = move_best(solution, instance, random.Random(seed))
        ran_at_least_once = True
        after = total_cost(instance, new_solution)
        assert after <= before, f"seed={seed}: move_best increased total_cost ({before} -> {after})"
    assert ran_at_least_once
```

(`move_best` has no `ValueError` path worth retrying here: `_sudoku_solution()` always has events with a non-empty `valid_start_time_ids`, so every seed succeeds.)

- [ ] **Step 2: Run test to verify it fails**

Run: `uv run python -m pytest tests/test_heuristics.py::test_move_best_never_increases_total_cost -v`
Expected: FAIL — `ImportError: cannot import name 'move_best' from 'xhstt_core.heuristics'`.

- [ ] **Step 3: Implement `_best_time_for_event` and `move_best`**

In `xhstt_core/heuristics.py`, update the imports and add the two functions (insert after `move_random`, before `MANUAL_HEURISTICS`):

```python
from dataclasses import dataclass, replace
from typing import Callable

from xhstt_core.evaluator_ref import total_cost, valid_start_time_ids
from xhstt_core.model import Instance, Solution
from xhstt_core.moves import time_reassign_move
```

```python
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
```

Add to `MANUAL_HEURISTICS`:

```python
    Heuristic(
        id="move_best",
        name="Best-slot time reassignment",
        protected=True,
        apply=move_best,
    ),
```

- [ ] **Step 4: Run tests to verify everything passes**

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: PASS — the new cost test, plus the Task 1 structural test now parametrized over 2 heuristics (`move_random`, `move_best`).

- [ ] **Step 5: Commit**

```bash
git add xhstt_core/heuristics.py tests/test_heuristics.py
git commit -m "feat: add move_best (best-slot search shared with repair)"
```

---

### Task 3: `swap`

**Files:**
- Modify: `xhstt_core/heuristics.py`
- Modify: `tests/test_heuristics.py` (no new test file changes beyond the parametrized structural test picking it up automatically)

**Interfaces:**
- Consumes: `xhstt_core.moves.time_swap_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution` (existing).
- Produces: `swap(solution: Solution, instance: Instance, rng: random.Random) -> Solution`, appended to `MANUAL_HEURISTICS`.

- [ ] **Step 1: Confirm the failing case is absent (no new test file needed)**

Task 1's structural test is parametrized directly over `MANUAL_HEURISTICS`, so it will pick up a 3rd case (`swap`) automatically the moment `swap` exists in that list — no test file edit needed this task. Confirm it's currently absent:

Run: `uv run python -m pytest tests/test_heuristics.py -v --collect-only`
Expected: only `move_random` and `move_best` cases listed for the structural test (no `swap` case yet).

- [ ] **Step 2: Implement `swap`**

In `xhstt_core/heuristics.py`, add `time_swap_move` to the `moves` import:

```python
from xhstt_core.moves import time_reassign_move, time_swap_move
```

Add the function (after `move_best`, before `MANUAL_HEURISTICS`):

```python
def swap(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.time_swap_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Two events that happen
    to already share a time_ref produce a harmless structurally-valid
    no-op, not an error -- no special-casing needed."""
    return time_swap_move(instance, solution, rng)
```

Add to `MANUAL_HEURISTICS`:

```python
    Heuristic(
        id="swap",
        name="Swap two events' times",
        protected=True,
        apply=swap,
    ),
```

- [ ] **Step 3: Run tests to verify it passes**

Run: `uv run python -m pytest tests/test_heuristics.py -v --collect-only`
Expected: structural test now has 3 parametrized cases (`move_random`, `move_best`, `swap`).

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: PASS, all tests.

- [ ] **Step 4: Commit**

```bash
git add xhstt_core/heuristics.py
git commit -m "feat: add swap heuristic"
```

---

### Task 4: `repair_hard_violation` + final verification

**Files:**
- Modify: `xhstt_core/heuristics.py`
- Modify: `tests/test_heuristics.py`

**Interfaces:**
- Consumes: `xhstt_core.evaluator_ref.resolve_occurrences(instance: Instance, solution: Solution) -> list[Occurrence]` (existing); `xhstt_core.evaluator_ref.evaluate_constraint(instance: Instance, occurrences: list[Occurrence], constraint: Constraint) -> int` (existing); `xhstt_core.evaluator_ref._events_in_applies_to(instance: Instance, applies_to: AppliesTo) -> frozenset[str]` (existing private helper, already imported across modules by `moves.py`).
- Produces: `_movable_violating_indices(instance: Instance, solution: Solution) -> list[int]` (module-private); `repair_hard_violation(solution: Solution, instance: Instance, rng: random.Random) -> Solution`, appended to `MANUAL_HEURISTICS` (final pool of 4).

- [ ] **Step 1: Write the failing tests**

Append to `tests/test_heuristics.py`:

```python
from xhstt_core.evaluator_ref import evaluate_constraint, resolve_occurrences
from xhstt_core.heuristics import repair_hard_violation  # add alongside move_best import


def _infeasibility(instance, solution: Solution) -> int:
    occurrences = resolve_occurrences(instance, solution)
    return sum(
        evaluate_constraint(instance, occurrences, c)
        for c in instance.constraints
        if c.required
    )


def test_repair_hard_violation_never_increases_infeasibility() -> None:
    instance, solution = _sudoku_solution()
    before = _infeasibility(instance, solution)
    assert before > 0, "fixture/seed must start with a hard violation for this test to matter"

    ran_at_least_once = False
    for seed in range(20):
        try:
            new_solution = repair_hard_violation(solution, instance, random.Random(seed))
        except ValueError:
            continue
        ran_at_least_once = True
        after = _infeasibility(instance, new_solution)
        assert after <= before, (
            f"seed={seed}: repair_hard_violation increased infeasibility ({before} -> {after})"
        )
    assert ran_at_least_once, "no seed in range(20) found a movable violating event"


def test_repair_hard_violation_raises_without_a_violation() -> None:
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="Day_1"><Name>Day_1</Name></Time></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>1</Duration><Resources></Resources></Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = build_initial(instance, random.Random(0))

    with pytest.raises(ValueError):
        repair_hard_violation(solution, instance, random.Random(0))
```

This mirrors the no-reassignable-resource fixture already used in `tests/test_moves.py::test_resource_reassign_move_raises_when_no_event_has_a_reassignable_resource` — a single unconstrained event has no `Required` constraints at all, so `repair_hard_violation` must raise.

- [ ] **Step 2: Run tests to verify they fail**

Run: `uv run python -m pytest tests/test_heuristics.py::test_repair_hard_violation_never_increases_infeasibility tests/test_heuristics.py::test_repair_hard_violation_raises_without_a_violation -v`
Expected: FAIL — `ImportError: cannot import name 'repair_hard_violation' from 'xhstt_core.heuristics'`.

- [ ] **Step 3: Implement `_movable_violating_indices` and `repair_hard_violation`**

In `xhstt_core/heuristics.py`, update the `evaluator_ref` import:

```python
from xhstt_core.evaluator_ref import (
    _events_in_applies_to,
    evaluate_constraint,
    resolve_occurrences,
    total_cost,
    valid_start_time_ids,
)
```

Add the functions (after `swap`, before `MANUAL_HEURISTICS`):

```python
def _movable_violating_indices(instance: Instance, solution: Solution) -> list[int]:
    """Indices into solution.events whose event participates in at least
    one violated Required constraint AND has a time_ref we can move (fully
    preassigned events never appear in solution.events at all -- see
    construct.build_initial -- so they're excluded automatically)."""
    occurrences = resolve_occurrences(instance, solution)
    violated_event_ids: set[str] = set()
    for constraint in instance.constraints:
        if not constraint.required:
            continue
        if evaluate_constraint(instance, occurrences, constraint) <= 0:
            continue
        violated_event_ids |= _events_in_applies_to(instance, constraint.applies_to)

    return [
        i
        for i, se in enumerate(solution.events)
        if se.event_ref in violated_event_ids and se.time_ref is not None
    ]


def repair_hard_violation(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Picks one (movable) event involved in a violated Required
    constraint at random and moves it to its best available time (see
    _best_time_for_event) -- never a purely random move, so the operator
    actually tends to repair rather than just perturb."""
    candidates = _movable_violating_indices(instance, solution)
    if not candidates:
        raise ValueError("no movable event participates in a hard constraint violation")
    index = rng.choice(candidates)
    return _best_time_for_event(instance, solution, index)
```

Add to `MANUAL_HEURISTICS`:

```python
    Heuristic(
        id="repair_hard_violation",
        name="Repair a hard constraint violation",
        protected=True,
        apply=repair_hard_violation,
    ),
```

- [ ] **Step 4: Run the full test file to verify everything passes**

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: PASS — all tests, including the structural test now parametrized over all 4 heuristics (`move_random`, `move_best`, `swap`, `repair_hard_violation`).

- [ ] **Step 5: Type-check and lint the new files explicitly**

The repo's default `uv run mypy src/` / `uv run ruff check src/` commands don't reach `xhstt_core/` (see `CLAUDE.md`), so point the tools at the new files directly. Plain `uv run mypy xhstt_core/heuristics.py` follows imports into `evaluator_ref.py`/`moves.py`/`model.py` and will report ~18 **pre-existing** errors in those files (confirmed present on `main` before this plan started, e.g. `xhstt_core/moves.py:42` passing `int | None` where `int` is expected) — none of that is this task's to fix. Use `--follow-imports=silent` to scope the report to the file actually being checked:

Run: `uv run mypy --follow-imports=silent xhstt_core/heuristics.py`
Expected: `Success: no issues found in 1 source file`. If it reports anything, fix `heuristics.py` (the assert in `_best_time_for_event`, Task 2, exists specifically to keep this clean) — do not touch `evaluator_ref.py`/`moves.py`/`model.py` to silence something reported there.

Run: `uv run ruff check xhstt_core/heuristics.py tests/test_heuristics.py`
Expected: `All checks passed!` (this one doesn't follow imports, so it's already scoped to just these two files).

Fix any reported issues before proceeding (common ones: missing return type on a helper, unused import).

- [ ] **Step 6: Run the full existing test suite to confirm no regressions**

Run: `uv run python -m pytest`
Expected: PASS — every existing test still passes (this task never touched `moves.py`/`lahc.py`, so this is a safety net, not expected to catch anything).

- [ ] **Step 7: Commit**

```bash
git add xhstt_core/heuristics.py tests/test_heuristics.py
git commit -m "feat: add repair_hard_violation heuristic, completing the E4-T1/T2/T5 pool"
```
