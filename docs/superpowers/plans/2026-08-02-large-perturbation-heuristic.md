# Large Perturbation Heuristic (E4-T6) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `large_perturbation_move` (a large, uncapped random reassignment of a big fraction of movable events, no greedy recreate step) to `moves.py`, wrap it as `large_perturbation` in the manual heuristic pool (`heuristics.py`), and close out GitHub issue #33 (`[E4-T6] Perturbacja duża`) — the last unimplemented item of Epic 4 (#27, "Pula heurystyk ręcznych").

**Architecture:** Mirrors the existing `moves.py`/`heuristics.py` split: `large_perturbation_move(instance, solution, rng) -> Solution` is a pure mechanical move (no `total_cost` evaluation) living in `moves.py` next to `time_reassign_move`/`time_swap_move`/`resource_reassign_move`/`kempe_chain_move`; `heuristics.py` adds only a thin 3-argument wrapper (`large_perturbation`) and a `MANUAL_HEURISTICS` entry, exactly like `kempe_chain`/`resource_reassign` already do for their `moves.py` counterparts. Unlike `ruin_and_recreate` (small, capped portion, greedily rebuilt event-by-event with `total_cost`), this operator reassigns a large, uncapped fraction of movable events, each independently to a uniformly random valid time, with **no** cost-minimizing step — that's what makes it cheap enough to be "large" and what gives it a genuinely different character in the pool (blind, aggressive escape move vs. `ruin_and_recreate`'s small, guided one).

**Tech Stack:** Python 3.12, stdlib `dataclasses`/`random`, pytest.

## Global Constraints

- Issue spec (verbatim, from `gh issue view 33`): "Duża losowa perturbacja wyrywająca z lokalnego minimum." Subtasks: (1) losowe przesunięcie P% zdarzeń, P konfigurowalny; (2) możliwość użycia jako restart z zachowaniem najlepszego; (3) test poprawnej struktury + immutability.
- `apply(solution, instance, rng) -> Solution` is the pool contract (reverse argument order from `moves.py`'s `move_fn(instance, solution, rng)`). The new pool entry follows this exactly, same as every existing entry in `MANUAL_HEURISTICS`.
- Never mutate the input `Solution` or any `SolutionEvent`/list inside it — build new objects via `dataclasses.replace` + structural sharing (list copy, only touched indices rebuilt), exactly like every function already in `moves.py`.
- An unapplicable heuristic call raises `ValueError` (never returns `None`, never silently no-ops) — matches every existing move/heuristic.
- "P konfigurowalny" is satisfied the same way `ruin_and_recreate`'s "K konfigurowalny" (issue #31) already was: module-level constants (`_LARGE_PERTURBATION_FRACTION`, `_LARGE_PERTURBATION_MIN_EVENTS`) that are trivially tunable in one place, not a runtime function parameter — this keeps the pool's fixed 3-argument `apply` signature intact and matches the accepted precedent already in this codebase. Do not add a `fraction` parameter to `large_perturbation`/`large_perturbation_move`.
- "Możliwość użycia jako restart z zachowaniem najlepszego" needs **no new code**: because `large_perturbation_move`/`large_perturbation` take whatever `Solution` they're handed (like every other heuristic), a future stagnation-triggered restart (Etap 5/6, not yet built — no `Selector`, no stagnation signal exists in `lahc.py` today) can call this same function directly on the solver's best-known solution instead of its current one. Document this in the docstring; do not build restart/stagnation orchestration as part of this plan — that's out of scope (Etap 5/6 architecture, not decided yet).
- Do not exclude an event's current `time_ref` when picking its new one — same precedent as `ruin_and_recreate`'s ruin step: landing back on the same time by chance is a harmless structurally-valid no-op, not a bug to special-case.
- A "movable" solution event is one with `time_ref is not None and duration is not None` — the same filter `time_reassign_move` and `ruin_and_recreate` already use (excludes resources-only entries from `construct.build_initial`).
- No delta evaluation, no `total_cost` calls anywhere in `large_perturbation_move` — this operator is intentionally blind/cheap, unlike `ruin_and_recreate`'s greedy recreate step.
- Docstrings/comments in `xhstt_core/*.py` are English (matches every existing file in that package).
- **Test invocation:** use `uv run python -m pytest ...`, not bare `uv run pytest ...` (no `[build-system]` table in `pyproject.toml`, so the bare console-script entry point doesn't get the repo root on `sys.path` and `xhstt_core` fails to import — see `CLAUDE.md`).
- **Type checking:** the default `uv run mypy src/` doesn't reach `xhstt_core/`. Use `uv run mypy --follow-imports=silent xhstt_core/moves.py` and `uv run mypy --follow-imports=silent xhstt_core/heuristics.py` to scope the report to just the file being checked (plain `--follow-imports` surfaces ~18 pre-existing errors elsewhere in `xhstt_core` that are not this task's to fix).
- **Linting:** `uv run ruff check xhstt_core/moves.py xhstt_core/heuristics.py tests/test_moves.py tests/test_heuristics.py` (doesn't follow imports, already scoped).

---

## File Structure

- Modify: `xhstt_core/moves.py` — add `_LARGE_PERTURBATION_FRACTION`, `_LARGE_PERTURBATION_MIN_EVENTS`, `large_perturbation_move(instance, solution, rng) -> Solution`, inserted after `kempe_chain_move` (currently ends at line 228).
- Modify: `tests/test_moves.py` — add structural/determinism/immutability/overflow/raise tests for `large_perturbation_move`.
- Modify: `xhstt_core/heuristics.py` — add `large_perturbation(solution, instance, rng) -> Solution` wrapper and a `Heuristic(id="large_perturbation", ...)` entry appended to `MANUAL_HEURISTICS` (currently ends at line 270).
- `tests/test_heuristics.py` is **not** modified: its structural test (`test_heuristic_preserves_events_and_does_not_mutate_input`, line 46) is already parametrized directly over `MANUAL_HEURISTICS`, so it picks up the new `large_perturbation` case automatically the moment it's appended to the list — this is exactly how `swap`/`repair_hard_violation`/`resource_reassign`/`kempe_chain`/`ruin_and_recreate` were each already picked up without touching that test file.

---

### Task 1: `large_perturbation_move` in `moves.py`

**Files:**
- Modify: `xhstt_core/moves.py` (append after `kempe_chain_move`, line 228)
- Modify: `tests/test_moves.py`

**Interfaces:**
- Consumes: `xhstt_core.evaluator_ref.valid_start_time_ids(instance: Instance, duration: int) -> tuple[str, ...]` (existing, already imported in `moves.py`); `dataclasses.replace` (existing import).
- Produces: `large_perturbation_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution` (module-level, public — consumed by Task 2's `heuristics.py` wrapper).

- [ ] **Step 1: Write the failing tests**

Append to `tests/test_moves.py` (add `large_perturbation_move` to the existing `from xhstt_core.moves import (...)` block):

```python
from xhstt_core.moves import (
    kempe_chain_move,
    large_perturbation_move,
    resource_reassign_move,
    time_reassign_move,
    time_swap_move,
)
```

Then append these test functions at the end of the file:

```python
def test_large_perturbation_move_changes_a_large_fraction_of_movable_events():
    instance, solution = _sudoku_solution()
    movable = [
        se for se in solution.events if se.time_ref is not None and se.duration is not None
    ]
    # Mirrors moves.py's _LARGE_PERTURBATION_FRACTION=0.3 /
    # _LARGE_PERTURBATION_MIN_EVENTS=4 exactly -- if those constants ever
    # change, update this expectation to match.
    expected_k = min(len(movable), max(4, round(len(movable) * 0.3)))

    new_solution = large_perturbation_move(instance, solution, random.Random(5))

    assert len(new_solution.events) == len(solution.events)
    diffs = [
        (a, b)
        for a, b in zip(solution.events, new_solution.events)
        if a.time_ref != b.time_ref
    ]
    assert len(diffs) == expected_k
    for old, new in diffs:
        assert old.event_ref == new.event_ref
        assert new.resources == old.resources
    for a, b in zip(solution.events, new_solution.events):
        if a.time_ref == b.time_ref:
            assert a == b


def test_large_perturbation_move_is_deterministic_given_same_seed():
    instance, solution = _sudoku_solution()

    a = large_perturbation_move(instance, solution, random.Random(9))
    b = large_perturbation_move(instance, solution, random.Random(9))

    assert a.events == b.events


def test_large_perturbation_move_does_not_mutate_the_input_solution():
    instance, solution = _sudoku_solution()
    original_times = [e.time_ref for e in solution.events]

    large_perturbation_move(instance, solution, random.Random(5))

    assert [e.time_ref for e in solution.events] == original_times


def test_large_perturbation_move_raises_when_no_movable_event_exists():
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="T1"><Name>T1</Name></Time></Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Room"><Name>Room</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="R1"><Name>R1</Name><ResourceType Reference="Room"/></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Time Reference="T1"/>
          <Resources>
            <Resource><Role>Room</Role><ResourceType Reference="Room"/></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = build_initial(instance, random.Random(0))

    with pytest.raises(ValueError):
        large_perturbation_move(instance, solution, random.Random(0))


def test_large_perturbation_move_never_overflows_a_day_boundary_or_the_time_array():
    instance = parse_archive(_two_day_multi_period_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1", duration=2),
            SolutionEvent(event_ref="E2", time_ref="Tue_1", duration=1),
        ],
    )
    # Both events are movable, and _LARGE_PERTURBATION_MIN_EVENTS=4 exceeds
    # the 2 available, so both get perturbed on every seed -- this is a
    # deliberately harder overflow check than the single-event moves get.
    invalid_for_e1 = {"Mon_3", "Tue_2"}

    for seed in range(50):
        new_solution = large_perturbation_move(instance, solution, random.Random(seed))
        e1 = next(se for se in new_solution.events if se.event_ref == "E1")
        assert e1.time_ref not in invalid_for_e1, f"seed={seed}: E1 landed on {e1.time_ref}"
```

- [ ] **Step 2: Run the tests to verify they fail**

Run: `uv run python -m pytest tests/test_moves.py -k large_perturbation -v`
Expected: FAIL/ERROR — `ImportError: cannot import name 'large_perturbation_move' from 'xhstt_core.moves'`.

- [ ] **Step 3: Implement `large_perturbation_move`**

In `xhstt_core/moves.py`, append after `kempe_chain_move` (after line 228; no new imports needed — `replace` and `valid_start_time_ids` are already imported):

```python
_LARGE_PERTURBATION_FRACTION = 0.3
_LARGE_PERTURBATION_MIN_EVENTS = 4


def large_perturbation_move(
    instance: Instance, solution: Solution, rng: random.Random
) -> Solution:
    """Large random perturbation meant to kick LAHC out of a local minimum,
    as opposed to ruin_and_recreate's small, greedily-rebuilt portion:
    picks about _LARGE_PERTURBATION_FRACTION of the solution's movable
    events (floored at _LARGE_PERTURBATION_MIN_EVENTS, capped at however
    many movable events actually exist -- unlike ruin_and_recreate there
    is no upper cap otherwise, which is what makes this "large") and
    reassigns each, independently, to a uniformly random valid start time.
    No cost-minimizing recreate step -- O(1) evaluator-free work per
    reassigned event, instead of ruin_and_recreate's O(valid times)
    total_cost calls -- which is what makes this operator cheap enough to
    touch a large fraction of the solution in one call. Returns a new
    Solution; the input is untouched.

    Because it operates on whatever Solution it's given, the exact same
    function can later be pointed at the solver's best-known solution
    instead of its current one, to implement a stagnation-triggered
    "restart from best" step once the solver loop grows one (Etap 5/6) --
    no separate code path needed here, just a different caller.

    Raises ValueError if the solution has no movable event (a time_ref and
    a duration set -- see time_reassign_move) to perturb, or if one of the
    chosen movable events has no valid start time at all."""
    movable = [
        i
        for i, se in enumerate(solution.events)
        if se.time_ref is not None and se.duration is not None
    ]
    if not movable:
        raise ValueError(
            "cannot apply a move: no solution event has a time to reassign"
        )

    k = min(
        len(movable),
        max(
            _LARGE_PERTURBATION_MIN_EVENTS,
            round(len(movable) * _LARGE_PERTURBATION_FRACTION),
        ),
    )
    chosen = rng.sample(movable, k)

    new_events = list(solution.events)
    for i in chosen:
        se = new_events[i]
        assert se.duration is not None
        candidates = valid_start_time_ids(instance, se.duration)
        if not candidates:
            raise ValueError(f"event {se.event_ref!r} has no valid start time")
        new_events[i] = replace(se, time_ref=rng.choice(candidates))
    return replace(solution, events=new_events)
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `uv run python -m pytest tests/test_moves.py -k large_perturbation -v`
Expected: PASS (5 new tests).

- [ ] **Step 5: Type-check and lint**

Run: `uv run mypy --follow-imports=silent xhstt_core/moves.py`
Expected: `Success: no issues found in 1 source file`.

Run: `uv run ruff check xhstt_core/moves.py tests/test_moves.py`
Expected: `All checks passed!`.

- [ ] **Step 6: Run the full existing test suite to confirm no regressions**

Run: `uv run python -m pytest`
Expected: PASS — every existing test still passes.

- [ ] **Step 7: Commit**

```bash
git add xhstt_core/moves.py tests/test_moves.py
git commit -m "feat: add large_perturbation_move, a large uncapped random reassignment"
```

---

### Task 2: Wire `large_perturbation` into the manual heuristic pool

**Files:**
- Modify: `xhstt_core/heuristics.py`

**Interfaces:**
- Consumes: `xhstt_core.moves.large_perturbation_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution` (Task 1).
- Produces: `large_perturbation(solution: Solution, instance: Instance, rng: random.Random) -> Solution`, appended to `MANUAL_HEURISTICS` (final pool of 8).

- [ ] **Step 1: Confirm the failing case is absent (no test file edit needed)**

Run: `uv run python -m pytest tests/test_heuristics.py -v --collect-only`
Expected: `test_heuristic_preserves_events_and_does_not_mutate_input` lists exactly the current 7 pool ids (`move_random`, `move_best`, `swap`, `repair_hard_violation`, `resource_reassign`, `kempe_chain`, `ruin_and_recreate`) — no `large_perturbation` case yet.

- [ ] **Step 2: Add `large_perturbation_move` to the `moves` import**

In `xhstt_core/heuristics.py`, update the import block:

```python
from xhstt_core.moves import (
    kempe_chain_move,
    large_perturbation_move,
    resource_reassign_move,
    time_reassign_move,
    time_swap_move,
)
```

- [ ] **Step 3: Add the wrapper**

In `xhstt_core/heuristics.py`, add this function right after `kempe_chain` (after line 121, before the `_RUIN_FRACTION` block):

```python
def large_perturbation(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    """Thin wrapper around moves.large_perturbation_move, adapted to the
    pool's apply(solution, instance, rng) argument order. Raises
    ValueError (via large_perturbation_move) if the solution has no
    movable event to perturb, or a chosen event has no valid start time."""
    return large_perturbation_move(instance, solution, rng)
```

- [ ] **Step 4: Append to `MANUAL_HEURISTICS`**

In `xhstt_core/heuristics.py`, add as the last entry of `MANUAL_HEURISTICS` (after the `ruin_and_recreate` entry, before the closing `]`):

```python
    Heuristic(
        id="large_perturbation",
        name="Large random perturbation across many events",
        protected=True,
        apply=large_perturbation,
    ),
```

- [ ] **Step 5: Run the structural test to verify it passes with 8 cases**

Run: `uv run python -m pytest tests/test_heuristics.py -v --collect-only`
Expected: `test_heuristic_preserves_events_and_does_not_mutate_input` now lists 8 parametrized cases, including `large_perturbation`.

Run: `uv run python -m pytest tests/test_heuristics.py -v`
Expected: PASS, all tests.

- [ ] **Step 6: Type-check and lint**

Run: `uv run mypy --follow-imports=silent xhstt_core/heuristics.py`
Expected: `Success: no issues found in 1 source file`.

Run: `uv run ruff check xhstt_core/heuristics.py`
Expected: `All checks passed!`.

- [ ] **Step 7: Run the full test suite to confirm no regressions**

Run: `uv run python -m pytest`
Expected: PASS — every existing test still passes, including `tests/test_lahc.py`'s `test_lahc_defaults_to_manual_heuristics_when_no_pool_is_given` (the pool now has 8 entries instead of 7, but that test doesn't hardcode a count).

- [ ] **Step 8: Commit**

```bash
git add xhstt_core/heuristics.py
git commit -m "feat: add large_perturbation to the manual heuristic pool, completing E4-T6"
```

---

## Self-Review

**Spec coverage** (issue #33):
- "Losowe przesunięcie P% zdarzeń (P konfigurowalny)" → Task 1, `_LARGE_PERTURBATION_FRACTION`/`_LARGE_PERTURBATION_MIN_EVENTS` module constants, same configurability precedent as `ruin_and_recreate`.
- "Możliwość użycia jako restart z zachowaniem najlepszego" → covered by design (function takes any `Solution`), documented in the docstring in Task 1 Step 3 and called out explicitly in Global Constraints as needing no new orchestration code (that's Etap 5/6, not yet built).
- "Test: poprawna struktura, immutability" → Task 1's 5 tests (structure, determinism, no-mutation, overflow safety, raise-when-inapplicable) plus Task 2's automatic pickup by `test_heuristics.py`'s pool-wide structural test.

**Placeholder scan:** no TBD/TODO, every step has literal runnable code and exact commands.

**Type consistency:** `large_perturbation_move(instance, solution, rng)` (Task 1) and its caller `large_perturbation(solution, instance, rng)` (Task 2) match in argument types and return type (`Solution`) throughout; `Heuristic.apply`'s declared type is `Callable[[Solution, Instance, random.Random], Solution]`, which `large_perturbation`'s signature satisfies exactly, same as every other pool entry.
