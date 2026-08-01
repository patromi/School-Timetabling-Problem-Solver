import random
from pathlib import Path

import pytest
from xhstt_core.construct import build_initial
from xhstt_core.evaluator_ref import (
    evaluate_constraint,
    resolve_occurrences,
    total_cost,
)
from xhstt_core.heuristics import (
    MANUAL_HEURISTICS,
    Heuristic,
    move_best,
    repair_hard_violation,
    ruin_and_recreate,
)
from xhstt_core.model import Instance, Solution, SolutionEvent
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
def test_heuristic_preserves_events_and_does_not_mutate_input(
    heuristic: Heuristic,
) -> None:
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
    assert isinstance(new_solution, Solution), (
        f"{heuristic.id} did not return a Solution"
    )
    assert sorted(se.event_ref for se in new_solution.events) == sorted(
        se.event_ref for se in solution.events
    ), f"{heuristic.id} lost or duplicated an event"


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
