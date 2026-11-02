import itertools
import os
import random
import subprocess
import sys
from dataclasses import replace
from pathlib import Path

import pytest
from src.construct import build_initial
from src.cost import Cost, evaluate_cost
from src.evaluator_ref import (
    evaluate_constraint,
    resolve_occurrences,
    total_cost,
)
from src.heuristics import (
    MANUAL_HEURISTICS,
    RESOURCE_HEURISTICS,
    Heuristic,
    move_best,
    reassign_resource_times,
    repair_hard_violation,
    ruin_and_recreate,
)
from src.incremental import IncrementalEvaluator, StructuralChangeError
from src.model import (
    AppliesTo,
    Constraint,
    Event,
    EventResource,
    Group,
    Instance,
    Resource,
    Solution,
    SolutionEvent,
    Time,
)
from src.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _sudoku_solution() -> tuple:
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    return instance, solution


# Sudoku allows one room per event, so these need a fixture with real choices.
_RESOURCE_OPERATORS = {"resource_reassign"} | {h.id for h in RESOURCE_HEURISTICS}


def _resource_solution() -> tuple:
    instance = parse_archive(_load("ResourceAssignment.xml"))[0]
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


@pytest.mark.parametrize(
    "heuristic",
    [h for h in MANUAL_HEURISTICS + RESOURCE_HEURISTICS if h.incremental_safe],
    ids=lambda h: h.id,
)
def test_heuristic_preserves_events_and_does_not_mutate_input(
    heuristic: Heuristic,
) -> None:
    """Covers structural-safe heuristics only. split_resize is structural
    (incremental_safe=False) and has its own test below."""
    if heuristic.id in _RESOURCE_OPERATORS:
        instance, solution = _resource_solution()
    else:
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
    assert new_solution is not solution, (
        f"{heuristic.id} returned the exact same Solution object (no deep copy/immutability)"
    )
    assert new_solution.events is not solution.events, (
        f"{heuristic.id} did not copy the events list/tuple"
    )
    assert isinstance(new_solution, Solution), (
        f"{heuristic.id} did not return a Solution"
    )
    assert sorted(se.event_ref for se in new_solution.events) == sorted(
        se.event_ref for se in solution.events
    ), f"{heuristic.id} lost or duplicated an event"

    for old_se, new_se in zip(
        sorted(solution.events, key=lambda e: e.event_ref),
        sorted(new_solution.events, key=lambda e: e.event_ref),
    ):
        assert old_se.duration == new_se.duration, (
            f"{heuristic.id} changed duration of event {old_se.event_ref}"
        )


def test_split_resize_returns_valid_solution_with_correct_total_duration() -> None:
    """split_resize is structural (incremental_safe=False): changes piece count.
    Uses BrazilInstance1 which has multi-duration split events."""
    from src.heuristics import MANUAL_HEURISTICS

    split_h = next(h for h in MANUAL_HEURISTICS if h.id == "split_resize")

    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    before_snapshot = _snapshot(solution)

    # Total duration per event_ref before
    total_dur_before: dict[str, int] = {}
    for se in solution.events:
        if se.duration is not None:
            total_dur_before[se.event_ref] = (
                total_dur_before.get(se.event_ref, 0) + se.duration
            )

    new_solution = None
    for seed in range(20):
        try:
            new_solution = split_h.apply(solution, instance, random.Random(seed))
            break
        except ValueError:
            continue
    assert new_solution is not None, (
        "split_resize: no seed produced applicable move on BrazilInstance1"
    )

    # Input must not be mutated
    assert _snapshot(solution) == before_snapshot, "split_resize mutated input"
    assert isinstance(new_solution, Solution)

    # Total duration per event_ref must be preserved
    total_dur_after: dict[str, int] = {}
    for se in new_solution.events:
        if se.duration is not None:
            total_dur_after[se.event_ref] = (
                total_dur_after.get(se.event_ref, 0) + se.duration
            )
    assert total_dur_after == total_dur_before, (
        "split_resize changed total event duration"
    )

    # Every sub-event must have a valid time_ref and duration > 0
    for se in new_solution.events:
        assert se.time_ref is not None
        assert se.duration is not None and se.duration > 0


def test_move_best_never_increases_total_cost() -> None:
    instance, solution = _sudoku_solution()
    before = total_cost(instance, solution)

    ran_at_least_once = False
    for seed in range(20):
        new_solution = move_best(solution, instance, random.Random(seed))
        ran_at_least_once = True
        after = total_cost(instance, new_solution)
        assert after <= before, (
            f"seed={seed}: move_best increased total_cost ({before} -> {after})"
        )
    assert ran_at_least_once


def _infeasibility(instance: Instance, solution: Solution) -> int:
    occurrences = resolve_occurrences(instance, solution)
    return sum(
        evaluate_constraint(instance, occurrences, c)
        for c in instance.constraints
        if c.required
    )


def test_repair_hard_violation_never_increases_infeasibility() -> None:
    instance, solution = _sudoku_solution()
    before = _infeasibility(instance, solution)
    assert before > 0, (
        "fixture/seed must start with a hard violation for this test to matter"
    )

    ran_at_least_once = False
    for seed in range(20):
        try:
            new_solution = repair_hard_violation(
                solution, instance, random.Random(seed)
            )
        except ValueError:
            continue
        ran_at_least_once = True
        after = _infeasibility(instance, new_solution)
        assert after <= before, (
            f"seed={seed}: repair_hard_violation increased infeasibility "
            f"({before} -> {after})"
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
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources></Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = build_initial(instance, random.Random(0))

    with pytest.raises(ValueError):
        repair_hard_violation(solution, instance, random.Random(0))


def test_repair_hard_violation_targets_the_violating_event_scoped_via_events() -> None:
    # PreferTimesConstraint's AppliesTo targets Events directly -- E2 only.
    # E1/E3 aren't covered by the constraint at all, so repair must never
    # touch them.
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
      </Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name><Duration>1</Duration><Resources></Resources>
        </Event>
        <Event Id="E2">
          <Name>E2</Name><Duration>1</Duration><Resources></Resources>
        </Event>
        <Event Id="E3">
          <Name>E3</Name><Duration>1</Duration><Resources></Resources>
        </Event>
      </Events>
      <Constraints>
        <PreferTimesConstraint Id="C1">
          <Name>PreferT2ForE2</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <Events><Event Reference="E2"/></Events>
          </AppliesTo>
          <Times><Time Reference="T2"/></Times>
        </PreferTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E2", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E3", time_ref="T1", duration=1),
        ],
    )
    assert _infeasibility(instance, solution) > 0

    for seed in range(20):
        new_solution = repair_hard_violation(solution, instance, random.Random(seed))
        diffs = [
            se.event_ref
            for old, se in zip(solution.events, new_solution.events, strict=True)
            if old.time_ref != se.time_ref
        ]
        assert diffs == ["E2"], f"seed={seed}: repair moved {diffs}, expected only E2"


def test_repair_hard_violation_targets_the_violating_event_scoped_via_resources() -> (
    None
):
    # AvoidClashesConstraint's AppliesTo targets Resources, not Events --
    # this is the case that was invisible to repair_hard_violation before
    # the fix (final-review Finding 1). E1/E2 share resource R1 at the
    # same time (a clash); E3 has no resource assignment and is unrelated.
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes>
          <ResourceType Id="Room"><Name>Room</Name></ResourceType>
        </ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="R1"><Name>R1</Name><ResourceType Reference="Room"/></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="R1"><Role>Room</Role></Resource></Resources>
        </Event>
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="R1"><Role>Room</Role></Resource></Resources>
        </Event>
        <Event Id="E3">
          <Name>E3</Name><Duration>1</Duration><Resources></Resources>
        </Event>
      </Events>
      <Constraints>
        <AvoidClashesConstraint Id="C1">
          <Name>NoClashes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <Resources><Resource Reference="R1"/></Resources>
          </AppliesTo>
        </AvoidClashesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E2", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E3", time_ref="T2", duration=1),
        ],
    )
    assert _infeasibility(instance, solution) > 0

    for seed in range(20):
        new_solution = repair_hard_violation(solution, instance, random.Random(seed))
        diffs = [
            se.event_ref
            for old, se in zip(solution.events, new_solution.events, strict=True)
            if old.time_ref != se.time_ref
        ]
        assert diffs != ["E3"], (
            f"seed={seed}: repair moved E3, which isn't involved in the clash"
        )
        assert diffs and diffs[0] in {"E1", "E2"}, (
            f"seed={seed}: repair moved {diffs}, expected E1 or E2"
        )


def test_ruin_and_recreate_raises_without_a_movable_event() -> None:
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="Day_1"><Name>Day_1</Name></Time></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Time Reference="Day_1"/>
          <Resources></Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = build_initial(instance, random.Random(0))
    assert solution.events == [], (
        "E1 is fully preassigned, so build_initial must skip it"
    )

    with pytest.raises(ValueError):
        ruin_and_recreate(solution, instance, random.Random(0))


def test_ruin_and_recreate_touches_at_most_the_ruin_budget() -> None:
    instance, solution = _sudoku_solution()

    for seed in range(20):
        new_solution = ruin_and_recreate(solution, instance, random.Random(seed))
        diffs = [
            old.event_ref
            for old, new in zip(solution.events, new_solution.events, strict=True)
            if old.time_ref != new.time_ref
        ]
        assert len(diffs) <= 6, (
            f"seed={seed}: touched {len(diffs)} events, expected at most 6"
        )




@pytest.mark.parametrize(
    "heuristic",
    [h for h in MANUAL_HEURISTICS + RESOURCE_HEURISTICS if h.incremental_safe],
    ids=lambda h: h.id,
)
def test_heuristic_incremental_safety(heuristic: Heuristic) -> None:
    if heuristic.id in _RESOURCE_OPERATORS:
        instance, solution = _resource_solution()
    else:
        instance, solution = _sudoku_solution()

    changed = False
    for seed in range(50):
        try:
            new_solution = heuristic.apply(solution, instance, random.Random(seed))
        except ValueError:
            continue

        if _snapshot(new_solution) != _snapshot(solution):
            changed = True

            evaluator = IncrementalEvaluator(instance, solution)
            try:
                transaction = evaluator.evaluate(new_solution)
            except StructuralChangeError:
                pytest.fail(
                    f"{heuristic.id} is marked incremental_safe but caused StructuralChangeError"
                )

            incremental_cost = transaction.cost.as_scalar()
            full_cost = evaluate_cost(instance, new_solution).as_scalar()

            assert incremental_cost == full_cost, (
                f"{heuristic.id} failed incremental safety check! "
                f"Incremental cost: {incremental_cost}, Full cost: {full_cost}"
            )

    assert changed, (
        f"{heuristic.id} did not make any structural change to the solution across 50 seeds"
    )


_CLASH_INSTANCE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
        <Time Id="T3"><Name>T3</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes>
          <ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType>
        </ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="R1"><Name>R1</Name><ResourceType Reference="Teacher"/></Resource>
        <Resource Id="R2"><Name>R2</Name><ResourceType Reference="Teacher"/></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name><Duration>1</Duration>
          <Resources><Resource Reference="R1"><Role>T</Role></Resource></Resources>
        </Event>
        <Event Id="E2">
          <Name>E2</Name><Duration>1</Duration>
          <Resources><Resource Reference="R1"><Role>T</Role></Resource></Resources>
        </Event>
        <Event Id="E3">
          <Name>E3</Name><Duration>1</Duration>
          <Resources><Resource Reference="R2"><Role>T</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <AvoidClashesConstraint Id="C1">
          <Name>NoClashes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <Resources><Resource Reference="R1"/><Resource Reference="R2"/></Resources>
          </AppliesTo>
        </AvoidClashesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_repair_clash_moves_a_clashing_event_to_a_free_time() -> None:
    # E1 and E2 share teacher R1 at T1; T2 and T3 are free for R1. Every
    # seed must leave the plan without a clash and touch only E1 or E2.
    from src.heuristics import repair_clash

    instance = parse_archive(_CLASH_INSTANCE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E2", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E3", time_ref="T2", duration=1),
        ],
    )
    assert _infeasibility(instance, solution) > 0

    for seed in range(20):
        new_solution = repair_clash(solution, instance, random.Random(seed))
        assert _infeasibility(instance, new_solution) == 0, f"seed={seed}"
        moved = [
            new.event_ref
            for old, new in zip(solution.events, new_solution.events, strict=True)
            if old.time_ref != new.time_ref
        ]
        assert len(moved) == 1 and moved[0] in {"E1", "E2"}, f"seed={seed}: {moved}"


def test_repair_clash_raises_without_a_clash() -> None:
    from src.heuristics import repair_clash

    instance = parse_archive(_CLASH_INSTANCE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E2", time_ref="T2", duration=1),
            SolutionEvent(event_ref="E3", time_ref="T1", duration=1),
        ],
    )

    with pytest.raises(ValueError):
        repair_clash(solution, instance, random.Random(0))


def test_repair_clash_never_increases_clashes_on_a_real_instance() -> None:
    from src.heuristics import repair_clash

    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    clash_ids = {
        c.id for c in instance.constraints if c.type == "AvoidClashesConstraint"
    }

    def clashes(sol: Solution) -> int:
        occurrences = resolve_occurrences(instance, sol)
        return sum(
            evaluate_constraint(instance, occurrences, c)
            for c in instance.constraints
            if c.id in clash_ids
        )

    before = clashes(solution)
    assert before > 0, "fixture must start with clashes for this test to matter"
    for seed in range(10):
        after = clashes(repair_clash(solution, instance, random.Random(seed)))
        assert after < before, f"seed={seed}: clashes {before} -> {after}"


@pytest.mark.parametrize(
    "heuristic",
    [h for h in MANUAL_HEURISTICS if h.apply_with_evaluator is not None],
    ids=lambda h: h.id,
)
def test_evaluator_path_matches_apply_and_leaves_the_evaluator_unchanged(
    heuristic: Heuristic,
) -> None:
    # The solver uses apply_with_evaluator whenever it has an incremental
    # evaluator; it must return what apply() returns for the same rng state,
    # or the incremental and full evaluation modes would diverge.
    from src.incremental import IncrementalEvaluator

    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    evaluator = IncrementalEvaluator(instance, solution)
    cost_before = evaluator.cost

    checked = 0
    for seed in range(10):
        try:
            expected = heuristic.apply(solution, instance, random.Random(seed))
        except ValueError:
            continue
        assert heuristic.apply_with_evaluator is not None
        got = heuristic.apply_with_evaluator(
            solution, instance, random.Random(seed), evaluator
        )
        assert _snapshot(got) == _snapshot(expected), f"seed={seed}"
        assert evaluator.cost == cost_before, f"seed={seed}: evaluator moved"
        checked += 1
    assert checked > 0


def _rrt_instance() -> tuple[Instance, Solution]:
    instance = Instance(
        id="RRT",
        name="RRT",
        time_groups=[Group(id="D", name="Day", kind="Day")],
        times=[Time(id=f"T{i}", name=f"T{i}", group_refs=["D"]) for i in range(6)],
        resources=[Resource(id="R", name="Teacher", resource_type_ref="Teacher")],
        events=[
            Event(
                id=f"E{i}",
                name=f"E{i}",
                duration=1,
                resources=[EventResource(role="Teacher", resource_ref="R")],
            )
            for i in range(3)
        ],
    )
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref=f"E{i}", duration=1, time_ref=f"T{2 * i}")
            for i in range(3)
        ],
    )
    return instance, solution


def test_goal_neighbourhoods_are_in_the_selector_pool() -> None:
    ids = {h.id for h in MANUAL_HEURISTICS}
    assert {"event_block_swap", "reassign_resource_times"} <= ids
    assert len(ids) == len(MANUAL_HEURISTICS)


def test_rrt_returns_best_alternative_even_when_all_are_worse() -> None:
    instance, solution = _rrt_instance()
    instance.constraints = [
        Constraint(
            id=f"C{i}",
            name="Prefer original time",
            type="PreferTimesConstraint",
            required=False,
            weight=i + 1,
            cost_function="Linear",
            applies_to=AppliesTo(events=[f"E{i}"]),
            params={"Times": [{"reference": f"T{2 * i}"}]},
        )
        for i in range(3)
    ]
    before = _snapshot(solution)
    alternatives = [
        replace(
            solution,
            events=[
                replace(event, time_ref=time)
                for event, time in zip(solution.events, permutation, strict=True)
            ],
        )
        for permutation in itertools.permutations(["T0", "T2", "T4"])
        if permutation != ("T0", "T2", "T4")
    ]
    expected = min(evaluate_cost(instance, candidate) for candidate in alternatives)
    assert expected > evaluate_cost(instance, solution)
    for seed in range(5):
        candidate = reassign_resource_times(solution, instance, random.Random(seed))
        assert evaluate_cost(instance, candidate) == expected
        assert _snapshot(solution) == before
        assert sorted(e.time_ref for e in candidate.events) == ["T0", "T2", "T4"]


def test_rrt_rejects_permutations_that_overflow_event_spans() -> None:
    instance, solution = _rrt_instance()
    solution.events[0] = replace(solution.events[0], duration=3)
    solution.events[2] = replace(solution.events[2], time_ref="T5")
    for seed in range(10):
        candidate = reassign_resource_times(solution, instance, random.Random(seed))
        assert candidate.events[0].time_ref != "T5"
        evaluator = IncrementalEvaluator(instance, solution)
        assert evaluator.probe(candidate) == evaluate_cost(instance, candidate)


def test_rrt_raises_if_no_alternative_permutation_exists() -> None:
    instance, solution = _rrt_instance()
    solution.events = [replace(e, time_ref="T0") for e in solution.events]
    with pytest.raises(ValueError, match="no valid alternative permutation"):
        reassign_resource_times(solution, instance, random.Random(0))


def test_rrt_is_reproducible_across_python_hash_seeds() -> None:
    code = """
import random
from pathlib import Path
from src.construct import build_initial
from src.heuristics import reassign_resource_times
from src.parser import parse_archive
instance = parse_archive(
    Path("tests/fixtures/ArtificialSudoku4x4.xml").read_text()
)[0]
solution = build_initial(instance, random.Random(0))
for seed in range(4):
    candidate = reassign_resource_times(solution, instance, random.Random(seed))
    print([event.time_ref for event in candidate.events])
"""
    results = [
        subprocess.run(
            [sys.executable, "-c", code],
            cwd=Path(__file__).resolve().parents[1],
            env={**os.environ, "PYTHONHASHSEED": str(hash_seed)},
            capture_output=True,
            text=True,
            check=True,
        ).stdout
        for hash_seed in (1, 2, 3)
    ]
    assert len(set(results)) == 1


def test_rrt_limits_work_to_five_events_and_119_alternative_probes(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    instance, solution = _rrt_instance()
    instance.times = [
        Time(id=f"T{i}", name=f"T{i}", group_refs=["D"]) for i in range(7)
    ]
    instance.events = [replace(instance.events[0], id=f"E{i}") for i in range(7)]
    solution.events = [
        SolutionEvent(event_ref=f"E{i}", duration=1, time_ref=f"T{i}") for i in range(7)
    ]
    probes = 0
    original_probe = IncrementalEvaluator.probe

    def count_probe(self: IncrementalEvaluator, candidate: Solution) -> Cost:
        nonlocal probes
        probes += 1
        return original_probe(self, candidate)

    monkeypatch.setattr(IncrementalEvaluator, "probe", count_probe)
    candidate = reassign_resource_times(solution, instance, random.Random(0))
    assert probes == 119
    assert (
        sum(
            old.time_ref != new.time_ref
            for old, new in zip(solution.events, candidate.events, strict=True)
        )
        <= 5
    )
    assert sorted(e.time_ref for e in candidate.events) == [f"T{i}" for i in range(7)]


def test_move_best_preserves_lexicographic_cost_even_with_large_soft_penalty() -> None:
    instance, solution = _rrt_instance()
    instance.events = instance.events[:1]
    solution.events = solution.events[:1]
    instance.constraints = [
        Constraint(
            id="hard",
            name="Hard",
            type="PreferTimesConstraint",
            required=True,
            weight=1,
            cost_function="Linear",
            applies_to=AppliesTo(events=["E0"]),
            params={"Times": [{"reference": "T0"}]},
        ),
        Constraint(
            id="soft",
            name="Soft",
            type="PreferTimesConstraint",
            required=False,
            weight=2_000_000,
            cost_function="Linear",
            applies_to=AppliesTo(events=["E0"]),
            params={"Times": [{"reference": "T2"}]},
        ),
    ]
    candidate = move_best(solution, instance, random.Random(0))
    assert candidate.events[0].time_ref == "T0"
    assert evaluate_cost(instance, candidate).infeasibility == 0


def test_move_best_with_probe_sample_never_worsens_the_solution() -> None:
    instance, solution = _rrt_instance()
    instance.events = instance.events[:1]
    solution.events = solution.events[:1]
    instance.constraints = [
        Constraint(
            id="prefer",
            name="Prefer T0",
            type="PreferTimesConstraint",
            required=True,
            weight=1,
            cost_function="Linear",
            applies_to=AppliesTo(events=["E0"]),
            params={"Times": [{"reference": "T0"}]},
        )
    ]
    before = evaluate_cost(instance, solution)
    assert before.infeasibility == 0
    for seed in range(20):
        candidate = move_best(solution, instance, random.Random(seed), max_probe=1)
        assert evaluate_cost(instance, candidate) <= before, f"seed={seed}"
