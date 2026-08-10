import random
from pathlib import Path

import pytest
from xhstt_core.construct import build_initial
from xhstt_core.model import Solution, SolutionEvent
from xhstt_core.moves import (
    kempe_chain_move,
    large_perturbation_move,
    resource_reassign_move,
    time_reassign_move,
    time_swap_move,
)
from xhstt_core.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _two_day_multi_period_archive() -> str:
    # Mirrors the exact shape that produced a real HSEval rejection
    # ("'Fr_5' not assignable to Event 'T10-S1'") after a longer LAHC run:
    # a duration>1 event whose *current* placement is mid-instance, with
    # too few remaining slots in some days/near the end of the array for
    # every time to be a legal reassignment target.
    return """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <Day Id="gr_Mon"><Name>Mon</Name></Day>
          <Day Id="gr_Tue"><Name>Tue</Name></Day>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Mon_3"><Name>Mon_3</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><Day Reference="gr_Tue"/></Time>
        <Time Id="Tue_2"><Name>Tue_2</Name><Day Reference="gr_Tue"/></Time>
      </Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>2</Duration><Resources></Resources></Event>
        <Event Id="E2"><Name>E2</Name><Duration>1</Duration><Resources></Resources></Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def _sudoku_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    return instance, solution


def test_time_reassign_move_changes_exactly_one_events_time():
    instance, solution = _sudoku_solution()

    new_solution = time_reassign_move(instance, solution, random.Random(1))

    assert len(new_solution.events) == len(solution.events)
    diffs = [
        (a, b)
        for a, b in zip(solution.events, new_solution.events)
        if a.time_ref != b.time_ref
    ]
    assert len(diffs) == 1
    old, new = diffs[0]
    assert old.event_ref == new.event_ref
    assert new.resources == old.resources
    # Everything else (event_ref, resources) must be byte-for-byte identical
    # for all OTHER events -- only the chosen event's time changed.
    for a, b in zip(solution.events, new_solution.events):
        if a is not old:
            assert a == b


def test_time_reassign_move_is_deterministic_given_same_seed():
    instance, solution = _sudoku_solution()

    a = time_reassign_move(instance, solution, random.Random(7))
    b = time_reassign_move(instance, solution, random.Random(7))

    assert a.events == b.events


def test_time_reassign_move_does_not_mutate_the_input_solution():
    instance, solution = _sudoku_solution()
    original_times = [e.time_ref for e in solution.events]

    time_reassign_move(instance, solution, random.Random(1))

    assert [e.time_ref for e in solution.events] == original_times


def _resources_only_and_movable_archive():
    # E1's Time is fixed on the Instance and its only resource role is
    # unassigned -- construct.build_initial gives it a resources-only
    # SolutionEvent entry (time_ref=None, duration=None). E2 is a normal
    # movable event. Regression fixture for the real AU-BG-98 crash:
    # time_reassign_move used to pick uniformly over ALL solution events,
    # including resources-only ones, and call valid_start_time_ids with
    # duration=None.
    return """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
      </Times>
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
        <Event Id="E2"><Name>E2</Name><Duration>1</Duration><Resources></Resources></Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_time_reassign_move_skips_resources_only_solution_events():
    instance = parse_archive(_resources_only_and_movable_archive())[0]
    solution = build_initial(instance, random.Random(0))
    resources_only = next(se for se in solution.events if se.event_ref == "E1")
    assert resources_only.time_ref is None and resources_only.duration is None, (
        "fixture must produce a resources-only entry for E1"
    )

    for seed in range(20):
        new_solution = time_reassign_move(instance, solution, random.Random(seed))
        new_e1 = next(se for se in new_solution.events if se.event_ref == "E1")
        assert new_e1.time_ref is None and new_e1.duration is None


def test_time_reassign_move_raises_when_only_resources_only_events_exist():
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
        time_reassign_move(instance, solution, random.Random(0))


def test_time_swap_move_swaps_two_events_times():
    # Uses an explicit solution (not build_initial's random assignment) so
    # the two chosen events are guaranteed to start with *different* times
    # -- otherwise a swap between two coincidentally-equal times is a
    # legitimate no-op, not evidence of a bug, and the test would be flaky.
    instance, solution = _sudoku_solution()
    all_times = sorted({e.time_ref for e in solution.events})
    assert len(all_times) >= 2, "fixture must offer at least 2 distinct times"
    for i, event in enumerate(solution.events):
        event.time_ref = all_times[i % len(all_times)]

    new_solution = time_swap_move(instance, solution, random.Random(2))

    diffs = [
        i
        for i, (a, b) in enumerate(zip(solution.events, new_solution.events))
        if a.time_ref != b.time_ref
    ]
    assert len(diffs) == 2
    i, j = diffs
    assert new_solution.events[i].time_ref == solution.events[j].time_ref
    assert new_solution.events[j].time_ref == solution.events[i].time_ref


def test_resource_reassign_move_changes_one_resource_to_a_same_type_alternative():
    instance, solution = _sudoku_solution()
    room_ids = {r.id for r in instance.resources if r.resource_type_ref == "Room"}

    new_solution = resource_reassign_move(instance, solution, random.Random(3))

    old_flat = [
        (se.event_ref, role, ref)
        for se in solution.events
        for role, ref in [(r.role, r.resource_ref) for r in se.resources]
    ]
    new_flat = [
        (se.event_ref, role, ref)
        for se in new_solution.events
        for role, ref in [(r.role, r.resource_ref) for r in se.resources]
    ]
    assert len(old_flat) == len(new_flat)
    diffs = [(o, n) for o, n in zip(old_flat, new_flat) if o != n]
    assert len(diffs) == 1
    (event_ref, role, old_ref), (_, _, new_ref) = diffs[0]
    assert new_ref != old_ref
    assert new_ref in room_ids


def test_time_reassign_move_never_overflows_a_day_boundary_or_the_time_array():
    # Regression test for the exact real-world HSEval rejection this fix
    # addresses: "'Fr_5' not assignable to Event 'T10-S1'" -- reassigning
    # a duration>1 event's time with no regard for whether it still fits
    # (same day, doesn't run past the last defined Time) produced
    # structurally invalid solutions after enough LAHC moves.
    instance = parse_archive(_two_day_multi_period_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1", duration=2),
            SolutionEvent(event_ref="E2", time_ref="Tue_1", duration=1),
        ],
    )
    # Duration-2 valid starts here are Mon_1, Mon_2, Tue_1 -- NOT Mon_3
    # (would spill into Tuesday) or Tue_2 (would run past the last Time).
    invalid_for_e1 = {"Mon_3", "Tue_2"}

    for seed in range(50):
        new_solution = time_reassign_move(instance, solution, random.Random(seed))
        e1 = next(se for se in new_solution.events if se.event_ref == "E1")
        assert e1.time_ref not in invalid_for_e1, (
            f"seed={seed}: E1 landed on {e1.time_ref}"
        )


def test_time_swap_move_never_overflows_a_day_boundary_or_the_time_array():
    instance = parse_archive(_two_day_multi_period_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1", duration=2),
            SolutionEvent(event_ref="E2", time_ref="Tue_1", duration=1),
        ],
    )

    for seed in range(50):
        new_solution = time_swap_move(instance, solution, random.Random(seed))
        for se in new_solution.events:
            if se.event_ref == "E1":
                assert se.time_ref not in {"Mon_3", "Tue_2"}, (
                    f"seed={seed}: E1 landed on {se.time_ref}"
                )


def test_time_swap_move_raises_when_the_only_possible_swap_would_overflow():
    instance = parse_archive(_two_day_multi_period_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1", duration=2),
            SolutionEvent(event_ref="E2", time_ref="Tue_2", duration=1),
        ],
    )
    # The only pair is (E1, E2); swapping would place E1 (duration 2) at
    # Tue_2, which overflows past the last Time -- no valid swap exists.
    with pytest.raises(ValueError):
        time_swap_move(instance, solution, random.Random(0))


def _two_time_conflict_archive():
    return """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Room"><Name>Room</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="R1"><Name>R1</Name><ResourceType Reference="Room"/></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="A">
          <Name>A</Name><Duration>1</Duration>
          <Resources><Resource Reference="R1"><Role>Room</Role></Resource></Resources>
        </Event>
        <Event Id="B">
          <Name>B</Name><Duration>1</Duration>
          <Resources><Resource Reference="R1"><Role>Room</Role></Resource></Resources>
        </Event>
        <Event Id="C"><Name>C</Name><Duration>1</Duration><Resources></Resources></Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_kempe_chain_move_swaps_the_whole_connected_component():
    # A and B share resource R1 (an edge); C shares nothing with either, so
    # it must never move even though it's parked at one of the two chosen
    # times too.
    instance = parse_archive(_two_time_conflict_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="A", time_ref="T1", duration=1),
            SolutionEvent(event_ref="B", time_ref="T2", duration=1),
            SolutionEvent(event_ref="C", time_ref="T1", duration=1),
        ],
    )

    for seed in range(20):
        new_solution = kempe_chain_move(instance, solution, random.Random(seed))
        by_ref = {se.event_ref: se.time_ref for se in new_solution.events}
        assert by_ref["A"] == "T2"
        assert by_ref["B"] == "T1"
        assert by_ref["C"] == "T1", "C shares no resource with A/B and must stay put"


def test_kempe_chain_move_raises_with_fewer_than_two_times():
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
        kempe_chain_move(instance, solution, random.Random(0))


def test_kempe_chain_move_raises_when_no_shared_resource_at_the_chosen_times():
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
        <Event Id="E1"><Name>E1</Name><Duration>1</Duration><Resources></Resources></Event>
        <Event Id="E2"><Name>E2</Name><Duration>1</Duration><Resources></Resources></Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E2", time_ref="T2", duration=1),
        ],
    )

    with pytest.raises(ValueError):
        kempe_chain_move(instance, solution, random.Random(0))


def test_resource_reassign_move_raises_when_no_event_has_a_reassignable_resource():
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
        resource_reassign_move(instance, solution, random.Random(0))


def test_large_perturbation_move_changes_a_large_fraction_of_movable_events() -> None:
    instance, solution = _sudoku_solution()
    movable = [
        se
        for se in solution.events
        if se.time_ref is not None and se.duration is not None
    ]
    # Mirrors moves.py's _LARGE_PERTURBATION_FRACTION=0.3 /
    # _LARGE_PERTURBATION_MIN_EVENTS=4 exactly -- if those constants ever
    # change, update this expectation to match.
    expected_k = min(len(movable), max(4, round(len(movable) * 0.3)))

    # A selected event can coincidentally land back on its own current
    # time_ref by design (see large_perturbation_move's docstring) -- a
    # harmless no-op, not a bug -- so "k events selected" and "k events
    # changed" aren't the same thing. Swept across seeds instead of
    # pinning one: len(diffs) must never exceed expected_k, and across
    # enough seeds it must reach expected_k at least once (proving the
    # operator does select a full k, not fewer).
    observed_diff_counts = []
    for seed in range(50):
        new_solution = large_perturbation_move(instance, solution, random.Random(seed))
        assert len(new_solution.events) == len(solution.events)
        diffs = [
            (a, b)
            for a, b in zip(solution.events, new_solution.events, strict=True)
            if a.time_ref != b.time_ref
        ]
        assert len(diffs) <= expected_k, f"seed={seed}: {len(diffs)} > {expected_k}"
        for old, new in diffs:
            assert old.event_ref == new.event_ref
            assert new.resources == old.resources
        for a, b in zip(solution.events, new_solution.events, strict=True):
            if a.time_ref == b.time_ref:
                assert a == b
        observed_diff_counts.append(len(diffs))

    assert max(observed_diff_counts) == expected_k, (
        f"no seed in range(50) selected the full {expected_k}: {observed_diff_counts}"
    )


def test_large_perturbation_move_is_deterministic_given_same_seed() -> None:
    instance, solution = _sudoku_solution()

    a = large_perturbation_move(instance, solution, random.Random(9))
    b = large_perturbation_move(instance, solution, random.Random(9))

    assert a.events == b.events


def test_large_perturbation_move_does_not_mutate_the_input_solution() -> None:
    instance, solution = _sudoku_solution()
    original_times = [e.time_ref for e in solution.events]

    large_perturbation_move(instance, solution, random.Random(5))

    assert [e.time_ref for e in solution.events] == original_times


def test_large_perturbation_move_raises_when_no_movable_event_exists() -> None:
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


def test_large_perturbation_move_never_overflows_a_day_boundary_or_the_time_array() -> None:
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
