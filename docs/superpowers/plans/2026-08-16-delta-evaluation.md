# Ewaluacja przyrostowa (delta evaluation) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `xhstt_core/delta.py::delta_cost`, a function that computes the cost of a new
solution incrementally from a known old cost, instead of recomputing every constraint from
scratch — numerically identical to `xhstt_core.cost.evaluate_cost`, but skipping constraints
whose `AppliesTo` scope provably doesn't touch anything that changed.

**Architecture:** One new, self-contained module. `delta_cost(instance, old_solution, old_cost,
new_solution)` resolves `Occurrence` lists for both solutions (`evaluator_ref.resolve_occurrences`,
already exists), diffs them positionally to find which events/resources changed, then re-runs the
existing `evaluate_constraint` (unchanged, same formulas as full evaluation) only for constraints
whose scope overlaps the changed set. Zero changes to `evaluator_ref.py`, `moves.py`,
`heuristics.py`, or `lahc.py` — this plan builds and tests the module in isolation; wiring it into
the LAHC hot path is an explicitly separate, later step (user decision from brainstorming).

**Tech Stack:** Python 3.12, stdlib only (`random`, `time`, `pathlib`) — no new dependency.

**Spec:** `docs/superpowers/specs/2026-08-16-delta-evaluation-design.md`

## Global Constraints

- Python 3.12+; `xhstt_core/` (repo root, not `src/`) is where this code lives.
- `mypy --strict` compatible: full type hints on every new function/parameter/return.
- `ruff` line-length 88, target `py312` (`pyproject.toml`).
- No new dependencies — benchmark script uses stdlib `time.perf_counter`, not `pytest-benchmark`
  (CLAUDE.md: "Zależności minimalne").
- Every RNG use is an explicit `random.Random(seed)` instance — never the global `random` module.
- Run tests with `uv run python -m pytest tests/test_delta.py -v` (bare `pytest` breaks imports —
  see CLAUDE.md's Commands section).
- Zero changes to `xhstt_core/evaluator_ref.py`, `xhstt_core/moves.py`, `xhstt_core/heuristics.py`,
  `xhstt_core/lahc.py` in this plan.
- Docstrings in English, matching the existing convention in every other `xhstt_core/*.py` file
  (`evaluator_ref.py`, `cost.py`, `heuristics.py`, `moves.py` are all English-docstring, even
  though CLI-facing text in `run_solver.py` is Polish).
- Work happens directly on the current branch (`main`) — consistent with how the design spec for
  this same feature was already committed directly to `main` earlier in this session; no new
  branch needed (repo's `check_branch_name.py` pre-commit hook exempts `main`).

> **Amendment (post-execution):** work actually happened on branch `issue#23-delta-evaluation`,
> not `main` as stated above — an isolated git worktree was created for this plan during the
> `superpowers:subagent-driven-development` setup step, and the branch was named to match the
> repo's own `issue#<number>[-description]` convention (see `scripts/check_branch_name.py`)
> instead of relying on `main`'s pre-commit exemption. Better isolation, no downside.

---

### Task 1: `xhstt_core/delta.py` core implementation + unit tests

**Files:**
- Create: `xhstt_core/delta.py`
- Create: `tests/test_delta.py`

**Interfaces:**
- Consumes (all pre-existing, unchanged):
  - `xhstt_core.cost.Cost` — `@dataclass(frozen=True, order=True)` with fields
    `infeasibility: int`, `objective: int`, method `as_scalar(hard_multiplier: int = ...) -> int`.
  - `xhstt_core.cost.evaluate_cost(instance: Instance, solution: Solution) -> Cost`.
  - `xhstt_core.evaluator_ref.resolve_occurrences(instance: Instance, solution: Solution) -> list[Occurrence]`.
  - `xhstt_core.evaluator_ref.evaluate_constraint(instance: Instance, occurrences: list[Occurrence], constraint: Constraint) -> int`.
  - `xhstt_core.evaluator_ref._events_in_applies_to(instance: Instance, applies_to: AppliesTo) -> frozenset[str]`.
  - `xhstt_core.evaluator_ref._resources_in_applies_to(instance: Instance, applies_to: AppliesTo) -> frozenset[str]`.
  - `xhstt_core.evaluator_ref._assigned_resource_ids(occurrence: Occurrence) -> list[str | None]`.
  - `xhstt_core.evaluator_ref._build_occupancy_index(instance: Instance, occurrences: list[Occurrence]) -> dict[str, Counter[str]]`.
  - `xhstt_core.evaluator_ref._current_occupancy_index` — module-level `dict[str, Counter[str]] | None`, read by `evaluate_constraint`'s callees when set.
  - `Occurrence` dataclass (not frozen, plain `@dataclass`, value-comparable via `==`): fields
    `event_ref: str`, `duration: int`, `time_ref: str | None`, `resource_assignments: list[tuple[str, str | None]]`.
- Produces:
  - `xhstt_core.delta.delta_cost(instance: Instance, old_solution: Solution, old_cost: Cost, new_solution: Solution) -> Cost`
    — raises `ValueError` if `old_solution`/`new_solution` resolve to a different number of
    occurrences.
  - `xhstt_core.delta._constraint_touches(instance: Instance, constraint: Constraint, touched_events: frozenset[str], touched_resources: frozenset[str]) -> bool` (internal, used by Task 1's own tests via monkeypatch of `evaluate_constraint`, not consumed elsewhere).

- [ ] **Step 1: Write the failing tests**

Create `tests/test_delta.py`:

```python
import random
from pathlib import Path

import pytest

import xhstt_core.delta as delta
from xhstt_core.construct import build_initial
from xhstt_core.cost import Cost, evaluate_cost
from xhstt_core.delta import delta_cost
from xhstt_core.evaluator_ref import evaluate_constraint
from xhstt_core.heuristics import MANUAL_HEURISTICS
from xhstt_core.model import (
    AppliesTo,
    Constraint,
    Event,
    Group,
    Instance,
    Solution,
    SolutionEvent,
    Time,
)
from xhstt_core.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _independent_prefer_times_instance(n: int) -> Instance:
    """n events, each duration 1, each with its OWN PreferTimesConstraint
    scoped to just that event (applies_to.events=[event_id]) preferring P1
    over P2. The constraints are fully independent of each other -- moving
    one event must never change another constraint's contribution, which is
    exactly what these tests check via the touched-constraint call count."""
    return Instance(
        id="I1",
        name="Test",
        time_groups=[Group(id="gr_Preferred", name="Preferred", kind="TimeGroup")],
        times=[
            Time(id="P1", name="P1", group_refs=["gr_Preferred"]),
            Time(id="P2", name="P2"),
        ],
        events=[Event(id=f"E{i}", name=f"E{i}", duration=1) for i in range(n)],
        constraints=[
            Constraint(
                type="PreferTimesConstraint",
                id=f"PT{i}",
                name=f"PreferTimes{i}",
                required=False,
                weight=10,
                cost_function="Linear",
                applies_to=AppliesTo(events=[f"E{i}"]),
                params={"TimeGroups": [{"reference": "gr_Preferred"}]},
            )
            for i in range(n)
        ],
    )


def _all_at_p1(n: int) -> Solution:
    return Solution(
        instance_ref="I1",
        events=[SolutionEvent(event_ref=f"E{i}", time_ref="P1") for i in range(n)],
    )


def test_delta_cost_returns_old_cost_unchanged_when_nothing_changed(monkeypatch):
    instance = _independent_prefer_times_instance(3)
    solution = _all_at_p1(3)
    cost = evaluate_cost(instance, solution)
    calls: list[int] = []
    real = evaluate_constraint
    monkeypatch.setattr(
        delta, "evaluate_constraint", lambda *a, **kw: (calls.append(1), real(*a, **kw))[1]
    )

    result = delta_cost(instance, solution, cost, solution)

    assert result == cost
    assert calls == []


def test_delta_cost_matches_full_evaluation_and_skips_unrelated_constraints(monkeypatch):
    n = 5
    instance = _independent_prefer_times_instance(n)
    old_solution = _all_at_p1(n)
    old_cost = evaluate_cost(instance, old_solution)
    new_events = list(old_solution.events)
    new_events[0] = SolutionEvent(event_ref="E0", time_ref="P2")
    new_solution = Solution(instance_ref="I1", events=new_events)

    calls: list[int] = []
    real = evaluate_constraint
    monkeypatch.setattr(
        delta, "evaluate_constraint", lambda *a, **kw: (calls.append(1), real(*a, **kw))[1]
    )

    result = delta_cost(instance, old_solution, old_cost, new_solution)

    assert result == evaluate_cost(instance, new_solution)
    assert result == Cost(infeasibility=0, objective=10)
    # Only PT0 (the constraint scoped to the moved event E0) is touched --
    # one call for its old contribution, one for its new contribution.
    # PT1..PT4 (scoped to untouched E1..E4) must never be re-evaluated.
    assert len(calls) == 2


def test_delta_cost_raises_when_solutions_have_a_different_occurrence_count():
    instance = Instance(
        id="I1",
        name="Test",
        times=[Time(id="P1", name="P1")],
        events=[Event(id="E1", name="E1", duration=1)],
    )
    old_solution = Solution(
        instance_ref="I1",
        events=[SolutionEvent(event_ref="E1", time_ref="P1", duration=1)],
    )
    new_solution = Solution(
        instance_ref="I1",
        events=[
            SolutionEvent(event_ref="E1", time_ref="P1", duration=1),
            SolutionEvent(event_ref="E1", time_ref="P1", duration=1),
        ],
    )
    old_cost = evaluate_cost(instance, old_solution)

    with pytest.raises(ValueError, match="different number of occurrences"):
        delta_cost(instance, old_solution, old_cost, new_solution)


def test_delta_cost_matches_full_evaluation_on_a_real_instance_after_one_lahc_style_move():
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    rng = random.Random(0)
    old_solution = build_initial(instance, rng)
    old_cost = evaluate_cost(instance, old_solution)
    heuristic = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    new_solution = heuristic.apply(old_solution, instance, rng)

    result = delta_cost(instance, old_solution, old_cost, new_solution)

    assert result == evaluate_cost(instance, new_solution)
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `uv run python -m pytest tests/test_delta.py -v`
Expected: `ModuleNotFoundError: No module named 'xhstt_core.delta'` (all 4 tests error out at
collection/import time, since `xhstt_core/delta.py` doesn't exist yet).

- [ ] **Step 3: Implement `xhstt_core/delta.py`**

```python
"""Incremental cost evaluation (Etap 3). delta_cost recomputes only the
constraints whose AppliesTo scope overlaps events/resources that actually
changed between old_solution and new_solution, instead of re-running every
constraint from scratch like xhstt_core.cost.evaluate_cost does. See
docs/superpowers/specs/2026-08-16-delta-evaluation-design.md for the design
rationale and the correctness argument for why skipping untouched
constraints is sound, not just fast."""

from xhstt_core import evaluator_ref
from xhstt_core.cost import Cost
from xhstt_core.evaluator_ref import (
    _assigned_resource_ids,
    _build_occupancy_index,
    _events_in_applies_to,
    _resources_in_applies_to,
    evaluate_constraint,
    resolve_occurrences,
)
from xhstt_core.model import Constraint, Instance, Solution


def _constraint_touches(
    instance: Instance,
    constraint: Constraint,
    touched_events: frozenset[str],
    touched_resources: frozenset[str],
) -> bool:
    """True if constraint's resolved AppliesTo scope (events/event groups,
    resources/resource groups, or event pairs) overlaps anything that
    changed -- i.e. its contribution to the total cost might differ
    between old_solution and new_solution. False means it's PROVEN
    identical (the constraint would see byte-for-byte the same Occurrence
    data on every position it looks at), not just probably unaffected."""
    if touched_events & _events_in_applies_to(instance, constraint.applies_to):
        return True
    if touched_resources & _resources_in_applies_to(instance, constraint.applies_to):
        return True
    return any(
        pair.first_event in touched_events or pair.second_event in touched_events
        for pair in constraint.applies_to.event_pairs
    )


def delta_cost(
    instance: Instance,
    old_solution: Solution,
    old_cost: Cost,
    new_solution: Solution,
) -> Cost:
    """Cost of new_solution, computed incrementally from old_solution's
    already-known old_cost -- numerically identical to
    evaluate_cost(instance, new_solution), but only re-evaluates
    constraints whose scope touches something that changed (see
    _constraint_touches). Assumes new_solution was produced from
    old_solution by a move that preserves event_ref and SolutionEvent
    count at every position (every heuristic in
    xhstt_core.heuristics.MANUAL_HEURISTICS satisfies this) -- raises
    ValueError if the resolved occurrence counts differ, which means that
    assumption was violated."""
    old_occurrences = resolve_occurrences(instance, old_solution)
    new_occurrences = resolve_occurrences(instance, new_solution)
    if len(old_occurrences) != len(new_occurrences):
        raise ValueError(
            "old_solution and new_solution resolve to a different number of "
            "occurrences -- delta_cost only supports moves that preserve "
            "event/split structure (see xhstt_core.heuristics.MANUAL_HEURISTICS)"
        )

    changed = [
        k for k in range(len(old_occurrences)) if old_occurrences[k] != new_occurrences[k]
    ]
    if not changed:
        return old_cost

    touched_events = frozenset(old_occurrences[k].event_ref for k in changed)
    touched_resources = frozenset(
        r
        for k in changed
        for r in _assigned_resource_ids(old_occurrences[k])
        + _assigned_resource_ids(new_occurrences[k])
        if r is not None
    )

    old_index = _build_occupancy_index(instance, old_occurrences)
    new_index = _build_occupancy_index(instance, new_occurrences)

    infeasibility, objective = old_cost.infeasibility, old_cost.objective
    for constraint in instance.constraints:
        if not _constraint_touches(instance, constraint, touched_events, touched_resources):
            continue

        evaluator_ref._current_occupancy_index = old_index
        try:
            old_contribution = evaluate_constraint(instance, old_occurrences, constraint)
        finally:
            evaluator_ref._current_occupancy_index = None

        evaluator_ref._current_occupancy_index = new_index
        try:
            new_contribution = evaluate_constraint(instance, new_occurrences, constraint)
        finally:
            evaluator_ref._current_occupancy_index = None

        change = new_contribution - old_contribution
        if change == 0:
            continue
        if constraint.required:
            infeasibility += change
        else:
            objective += change

    return Cost(infeasibility, objective)
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `uv run python -m pytest tests/test_delta.py -v`
Expected: PASS (4 passed).

- [ ] **Step 5: Type-check and lint**

Run: `uv run mypy xhstt_core/delta.py --follow-imports=silent`
Expected: `Success: no issues found`.

Run: `uv run ruff check xhstt_core/delta.py tests/test_delta.py`
Expected: no findings (fix any line-length/import-order issues if they appear).

Run: `uv run ruff format xhstt_core/delta.py tests/test_delta.py`

- [ ] **Step 6: Commit**

```bash
git add xhstt_core/delta.py tests/test_delta.py
git commit -m "$(cat <<'EOF'
feat: add delta_cost, incremental cost evaluation (Etap 3)

Diffs old_solution/new_solution's resolved Occurrences and only
re-evaluates constraints whose AppliesTo scope overlaps what changed --
reuses evaluate_constraint unchanged, so the delta and full evaluation
paths share the exact same per-constraint formulas.

Not wired into lahc.py/heuristics.py yet -- deliberately isolated per
docs/superpowers/specs/2026-08-16-delta-evaluation-design.md.
EOF
)"
```

---

### Task 2: 10,000-move chained invariant test (Etap 3 DoD)

**Files:**
- Modify: `tests/test_delta.py` (append one test)

**Interfaces:**
- Consumes: `delta_cost` and `_constraint_touches` from Task 1 (unchanged), plus existing
  `xhstt_core.construct.build_initial(instance: Instance, rng: random.Random) -> Solution`,
  `xhstt_core.heuristics.MANUAL_HEURISTICS: list[Heuristic]` where `Heuristic.apply(solution:
  Solution, instance: Instance, rng: random.Random) -> Solution` (raises `ValueError` when it
  can't find a valid move), `xhstt_core.parser.parse_archive(text: str) -> list[Instance]`.
- Produces: nothing consumed by later tasks — this is the CLAUDE.md Etap-3 DoD acceptance test.

- [ ] **Step 1: Write the test**

Append to `tests/test_delta.py`:

```python
def test_delta_cost_matches_full_evaluation_over_a_chain_of_10000_random_moves():
    # The Etap 3 DoD invariant from CLAUDE.md, verbatim: "po dowolnej
    # sekwencji ruchow koszt liczony przyrostowo == koszt liczony od
    # zera" -- chained (each move's output feeds the next), not 10000
    # independent single moves from the same starting point, and covers
    # all 7 heuristics (single-event and multi-event moves alike) since
    # each iteration draws uniformly from MANUAL_HEURISTICS.
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    rng = random.Random(0)
    solution = build_initial(instance, rng)
    cost = evaluate_cost(instance, solution)

    applied = 0
    while applied < 10_000:
        heuristic = rng.choice(MANUAL_HEURISTICS)
        try:
            candidate = heuristic.apply(solution, instance, rng)
        except ValueError:
            continue
        candidate_cost = delta_cost(instance, solution, cost, candidate)
        assert candidate_cost == evaluate_cost(instance, candidate)
        solution, cost = candidate, candidate_cost
        applied += 1
```

- [ ] **Step 2: Run the test**

Run: `uv run python -m pytest tests/test_delta.py::test_delta_cost_matches_full_evaluation_over_a_chain_of_10000_random_moves -v`
Expected: PASS. (This exercises the already-implemented `delta_cost` from Task 1 at scale on a
real instance rather than driving new implementation, so a RED step isn't meaningful here — the
test either confirms the invariant holds over 10,000 chained real moves, or it fails and surfaces
a correctness bug in Task 1's implementation to fix before proceeding.)

If it takes noticeably longer than ~2 minutes, that's still acceptable (the existing
`test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget` test already runs 40,000
iterations) — but if any single `assert` fails, stop and debug `delta_cost`/`_constraint_touches`
rather than loosening the test.

- [ ] **Step 3: Type-check and lint**

Run: `uv run mypy xhstt_core/delta.py --follow-imports=silent` (unchanged, re-confirm still clean)
Run: `uv run ruff check tests/test_delta.py`
Run: `uv run ruff format tests/test_delta.py`

- [ ] **Step 4: Commit**

```bash
git add tests/test_delta.py
git commit -m "$(cat <<'EOF'
test: add 10000-move chained invariant test for delta_cost (Etap 3 DoD)

Verifies delta_cost == full evaluate_cost after every move in a chained
sequence of 10000 real MANUAL_HEURISTICS moves on BrazilInstance1, per
CLAUDE.md's Etap 3 definition of done.
EOF
)"
```

---

### Task 3: Benchmark script (Etap 3 DoD)

> **Amendment (post-execution, during Task 3):** the plan originally specified `N_MOVES = 2000`.
> A clean, single-process calibration run on AU-BG-98 showed `move_best` costs ~3.7s/call and
> `ruin_and_recreate` costs ~22s/call on this instance (387 events) — with the deterministic
> `seed=0` sequence, roughly 5 of the first 17 heuristic draws were `ruin_and_recreate`,
> extrapolating to 3-4+ hours just to generate a 2000-move chain. `N_MOVES` is corrected to
> `120` below (~10-15 min total) — small enough to finish in one sitting, large enough to average
> out the per-move-type variance. The full 8-heuristic `MANUAL_HEURISTICS` pool is kept
> unchanged (user decision): the benchmark reports whatever blended speedup ratio actually comes
> out, honestly — including heuristics like `large_perturbation` (touches ~30% of all events in
> one move) for which `delta_cost` is expected to show little to no benefit, since incremental
> evaluation only pays off when a move's footprint is small relative to the instance. A ratio
> close to 1.0x is a legitimate, explainable finding (speedup is proportional to move locality),
> not a sign of a bug — Tasks 1-2 already independently proved `delta_cost`'s numeric correctness
> against full evaluation across 10,000 chained moves.

> **Second amendment (post-execution, after the first real run):** the blended, full-pool
> benchmark (N_MOVES=120, seed=0) measured **0.7x — `delta_cost` ~43% SLOWER** than full
> evaluation on AU-BG-98 (13.463s full vs 18.122s delta over 120 chained moves). This is a real,
> reproducible result, not noise from a small sample. Root cause: AU-BG-98's 172 constraints
> appear to include broadly-scoped ones (e.g. one `AvoidClashesConstraint` over a large resource
> group rather than many narrow per-resource ones) — `_constraint_touches` correctly identifies
> such a constraint as "touched" by nearly any move, but `delta_cost` then pays its full
> unrestricted-scope `evaluate_constraint` cost anyway (the "filtered full re-evaluation" design
> only saves time on constraints that are *entirely* untouched — see
> `docs/superpowers/specs/2026-08-16-delta-evaluation-design.md`'s rejected Option B/C), **plus**
> a fixed 2x overhead per call (`resolve_occurrences` and `_build_occupancy_index` are each run
> twice — once for the old solution, once for the new — versus once in full evaluation). With
> `large_perturbation`/`ruin_and_recreate` in the mix (each touching many events per move), almost
> every constraint ends up "touched," so `delta_cost` pays the 2x setup overhead with little to no
> constraint-skipping to offset it.
>
> Per user decision, the script now runs and reports **two** benchmarks instead of one: the
> existing blended one (kept, honestly, at whatever it measures — currently 0.7x) and a second one
> restricted to `LOCAL_HEURISTIC_IDS = {"move_random", "swap", "resource_reassign", "kempe_chain"}`
> — single- or few-event moves, the scenario `delta_cost` is actually designed for (CLAUDE.md's
> Etap 3 wording is literally "przy przesunięciu jednego zdarzenia"). The code block and Step 2
> below are updated to reflect this two-benchmark version — `_generate_move_chain` now takes an
> explicit `heuristics` parameter instead of closing over `MANUAL_HEURISTICS`, and a new
> `_run_benchmark(label, instance, heuristics, n, seed)` helper runs+prints one full
> chain-gen/time/report cycle so the two calls in `main()` share it instead of duplicating the
> body.

**Files:**
- Create: `scripts/benchmark_delta_evaluation.py`

**Interfaces:**
- Consumes: `delta_cost` (Task 1), plus `xhstt_core.construct.build_initial`,
  `xhstt_core.cost.Cost`, `xhstt_core.cost.evaluate_cost`, `xhstt_core.heuristics.MANUAL_HEURISTICS`,
  `xhstt_core.model.Instance`, `xhstt_core.model.Solution`, `xhstt_core.parser.parse_archive`.
- Produces: nothing consumed elsewhere — standalone diagnostic script, run manually (`python
  scripts/benchmark_delta_evaluation.py`), output goes to stdout for the thesis chapter, not to a
  file.

- [ ] **Step 1: Write the script**

Create `scripts/benchmark_delta_evaluation.py`:

```python
#!/usr/bin/env python
"""Benchmark: full evaluation (evaluate_cost from scratch) vs delta_cost
(incremental) on a real instance -- Etap 3 DoD ("benchmark pokazujacy
przyspieszenie"). Diagnostic script for the thesis chapter, not a test: no
assertions, just timings printed to stdout.

Reports two speedup ratios: one with the full MANUAL_HEURISTICS pool
(blended -- includes large_perturbation/ruin_and_recreate/move_best/
repair_hard_violation, which touch many events per move, so delta_cost has
little to skip), and one restricted to LOCAL_HEURISTIC_IDS (single- or
few-event moves -- the scenario delta_cost is designed for, matching
CLAUDE.md's Etap 3 wording "przesuniecie jednego zdarzenia"). Both numbers
are reported honestly, whatever they come out to."""

import random
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.resolve()))

from xhstt_core.construct import build_initial
from xhstt_core.cost import Cost, evaluate_cost
from xhstt_core.delta import delta_cost
from xhstt_core.heuristics import MANUAL_HEURISTICS, Heuristic
from xhstt_core.model import Instance, Solution
from xhstt_core.parser import parse_archive

ARCHIVE = Path(__file__).parent.parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
INSTANCE_ID = "AU-BG-98"
N_MOVES = 120
LOCAL_HEURISTIC_IDS = {"move_random", "swap", "resource_reassign", "kempe_chain"}


def _load_instance() -> Instance:
    instances = parse_archive(ARCHIVE.read_text(encoding="utf-8-sig"))
    instance = next((i for i in instances if i.id == INSTANCE_ID), None)
    if instance is None:
        raise SystemExit(f"Nie znaleziono instancji {INSTANCE_ID!r} w {ARCHIVE}")
    return instance


def _generate_move_chain(
    instance: Instance, heuristics: list[Heuristic], n: int, seed: int
) -> list[Solution]:
    rng = random.Random(seed)
    solutions = [build_initial(instance, rng)]
    while len(solutions) <= n:
        heuristic = rng.choice(heuristics)
        try:
            solutions.append(heuristic.apply(solutions[-1], instance, rng))
        except ValueError:
            continue
    return solutions


def _time_full_evaluation(instance: Instance, solutions: list[Solution]) -> float:
    start = time.perf_counter()
    for solution in solutions[1:]:
        evaluate_cost(instance, solution)
    return time.perf_counter() - start


def _time_delta_evaluation(
    instance: Instance, solutions: list[Solution], costs: list[Cost]
) -> float:
    start = time.perf_counter()
    for i in range(1, len(solutions)):
        delta_cost(instance, solutions[i - 1], costs[i - 1], solutions[i])
    return time.perf_counter() - start


def _run_benchmark(
    label: str, instance: Instance, heuristics: list[Heuristic], n: int, seed: int
) -> None:
    print(f"\n=== {label} ===")
    print(f"Generowanie lancucha {n} losowych ruchow...")
    solutions = _generate_move_chain(instance, heuristics, n, seed)
    costs = [evaluate_cost(instance, s) for s in solutions]

    print("Mierzenie pelnej ewaluacji (evaluate_cost od zera kazdorazowo)...")
    full_time = _time_full_evaluation(instance, solutions)

    print("Mierzenie ewaluacji przyrostowej (delta_cost)...")
    delta_time = _time_delta_evaluation(instance, solutions, costs)

    print(
        f"Pelna ewaluacja:       {full_time:.3f}s  "
        f"({len(solutions) / full_time:.0f} it/s)"
    )
    print(
        f"Ewaluacja przyrostowa: {delta_time:.3f}s  "
        f"({len(solutions) / delta_time:.0f} it/s)"
    )
    print(f"Przyspieszenie: {full_time / delta_time:.1f}x")


def main() -> None:
    print(f"Wczytywanie {INSTANCE_ID} z {ARCHIVE.name}...")
    instance = _load_instance()
    print(
        f"  Zdarzenia={len(instance.events)}  Czasy={len(instance.times)}  "
        f"Zasoby={len(instance.resources)}  Ograniczenia={len(instance.constraints)}"
    )

    local_heuristics = [h for h in MANUAL_HEURISTICS if h.id in LOCAL_HEURISTIC_IDS]

    _run_benchmark(
        "Pelna pula (8 heurystyk z MANUAL_HEURISTICS)",
        instance,
        MANUAL_HEURISTICS,
        N_MOVES,
        seed=0,
    )
    _run_benchmark(
        "Tylko ruchy lokalne (move_random, swap, resource_reassign, kempe_chain)",
        instance,
        local_heuristics,
        N_MOVES,
        seed=0,
    )


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Run the script and capture the result**

Run: `uv run python scripts/benchmark_delta_evaluation.py`
Expected: prints instance stats, then two labeled sections each with both timings and a speedup
ratio. **Actual measured result (final, reproduced twice):** blended (full 8-heuristic pool)
0.7x (14.769s full vs 21.050s delta); local-only (`move_random`/`swap`/`resource_reassign`/
`kempe_chain`) 0.9x (14.710s full vs 17.139s delta) — `delta_cost` is slower in BOTH scenarios on
AU-BG-98, not just the blended one. The original hypothesis that local-only moves would show a
clear win did not hold on this instance; the fixed per-call overhead (`resolve_occurrences` and
`_build_occupancy_index` each run twice — once for the old solution, once for the new — versus
once in full evaluation) combined with AU-BG-98's apparently broadly-scoped constraints outweighs
whatever `_constraint_touches` manages to skip, regardless of move locality. This is the honest,
final Etap 3 benchmark result — not a bug (Tasks 1-2 already proved `delta_cost` numerically
correct across 10,000 chained moves) but a real limitation of the "filtered full re-evaluation"
design on this instance's constraint structure. Note both printed speedup ratios in the task's
commit message — both are Etap 3 DoD evidence, together they show the full, honest picture.

- [ ] **Step 3: Type-check and lint**

Run: `uv run mypy scripts/benchmark_delta_evaluation.py --follow-imports=silent`
Run: `uv run ruff check scripts/benchmark_delta_evaluation.py`
Run: `uv run ruff format scripts/benchmark_delta_evaluation.py`

- [ ] **Step 4: Commit**

```bash
git add scripts/benchmark_delta_evaluation.py
git commit -m "$(cat <<'EOF'
perf: add delta_cost vs full-evaluation benchmark script (Etap 3 DoD)

Runs both cost-evaluation paths over the same chained-move sequence on
AU-BG-98 and reports two speedup ratios -- blended (full 8-heuristic
pool) and local-moves-only -- diagnostic evidence for the thesis
chapter, not a pytest test. Measured: 0.7x blended, 0.9x local-only --
delta_cost is slower in both scenarios on this instance (see plan's
Task 3 amendments for root-cause analysis: fixed 2x per-call overhead
outweighs constraint-skipping given AU-BG-98's broadly-scoped
constraints). Correctness is unaffected and separately proven (Tasks
1-2, 10000 chained moves, zero discrepancies).
EOF
)"
```

---

## Post-plan note

This plan deliberately stops at "delta.py exists, is correct, and is measurably faster" — it does
NOT wire `delta_cost` into `xhstt_core/lahc.py`'s acceptance loop or
`xhstt_core/heuristics.py`'s `move_best`/`repair_hard_violation`/`ruin_and_recreate` (all of which
still call full `evaluate_cost` per candidate today). That wiring is a separate follow-up
brainstorm + plan, same pattern as
`docs/superpowers/specs/2026-08-01-wire-heuristic-pool-into-lahc-design.md` was for the heuristic
pool.

### Follow-up experiment: lazy occupancy-index building (post-PR)

Tried the first of the final review's two cheap-optimization recommendations: build
`old_index`/`new_index` (`_build_occupancy_index`) only if at least one touched constraint's type
is actually one of the 5 (of 16) that ever reads `evaluator_ref._current_occupancy_index`
(`AvoidClashesConstraint`, `ClusterBusyTimesConstraint`, `AvoidUnavailableTimesConstraint`,
`LimitIdleTimesConstraint`, `LimitBusyTimesConstraint` — confirmed by grep). Change is
correctness-neutral (the index is purely a perf cache; every reader has a slower-but-equivalent
fallback for `None`) — all 4 fast `test_delta.py` tests still pass, mypy/ruff clean.

Single-run benchmark result on AU-BG-98: blended 0.7x → 0.8x, local-only 0.9x → 0.8x. **Net effect
is inconclusive** — full evaluation (untouched by this change) also sped up ~24% between the two
runs, indicating measurement noise on this machine is at least that large, which swamps whatever
small real effect the lazy index has. Not worth a multi-run statistical benchmark right now (user
decision) — recorded here as a documented, safe, marginal experiment rather than pursued further.
The other two recommendations from the final review (threading the occupancy index across solver
iterations instead of rebuilding it per call, and true per-point-of-application deltas) remain
unexplored and would need the Etap 5/6 solver-loop wiring to be meaningful to test anyway.
