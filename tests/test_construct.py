import random
from pathlib import Path

from src.construct import build_initial
from src.evaluator_ref import resolve_occurrences
from src.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def test_build_initial_assigns_a_time_and_resource_to_every_unassigned_slot():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    solution = build_initial(instance, random.Random(0))
    occurrences = resolve_occurrences(instance, solution)

    assert len(occurrences) == 16
    for o in occurrences:
        assert o.time_ref is not None
        for role, resource_ref in o.resource_assignments:
            assert resource_ref is not None, f"role {role!r} left unassigned"


def test_build_initial_only_assigns_resources_of_the_correct_type():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    room_ids = {r.id for r in instance.resources if r.resource_type_ref == "Room"}

    solution = build_initial(instance, random.Random(0))
    occurrences = resolve_occurrences(instance, solution)

    for o in occurrences:
        for role, resource_ref in o.resource_assignments:
            if role.startswith("Room"):
                assert resource_ref in room_ids


def test_build_initial_is_deterministic_given_the_same_seed():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    solution_a = build_initial(instance, random.Random(42))
    solution_b = build_initial(instance, random.Random(42))

    assert solution_a.events == solution_b.events


def test_build_initial_skips_events_that_are_already_fully_preassigned():
    archive = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="Day_1"><Name>Day_1</Name></Time></Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Time Reference="Day_1"/>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    instance = parse_archive(archive)[0]

    solution = build_initial(instance, random.Random(0))

    assert solution.events == []


def test_build_initial_does_not_crash_on_a_real_world_instance():
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]

    solution = build_initial(instance, random.Random(0))
    occurrences = resolve_occurrences(instance, solution)

    for o in occurrences:
        assert o.time_ref is not None


def _split_events_archive(duration: int, min_duration: int, max_duration: int) -> str:
    # 5 times on one Day group -- mirrors the real-world shape (a week of
    # single-day blocks) that exposed the original bug on BrazilInstance1.
    times_xml = "".join(
        f'<Time Id="T{i}"><Name>T{i}</Name><TimeGroups><TimeGroup Reference="gr_Day"/></TimeGroups></Time>'
        for i in range(1, 6)
    )
    return f"""<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups><TimeGroup Id="gr_Day"><Name>Day</Name></TimeGroup></TimeGroups>
        {times_xml}
      </Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups><EventGroup Id="gr_All"><Name>All</Name></EventGroup></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>{duration}</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_All"/></EventGroups>
        </Event>
      </Events>
      <Constraints>
        <SplitEventsConstraint Id="SE1">
          <Name>Split</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_All"/></EventGroups></AppliesTo>
          <MinimumDuration>{min_duration}</MinimumDuration>
          <MaximumDuration>{max_duration}</MaximumDuration>
          <MinimumAmount>1</MinimumAmount>
          <MaximumAmount>999</MaximumAmount>
        </SplitEventsConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_build_initial_splits_duration_according_to_split_events_constraint():
    # Mirrors BrazilInstance1's real T1-S3 event exactly: Duration=3,
    # MinimumDuration=1, MaximumDuration=2 -- must become multiple
    # sub-events, each within [1, 2], summing back to 3.
    instance = parse_archive(
        _split_events_archive(duration=3, min_duration=1, max_duration=2)
    )[0]

    solution = build_initial(instance, random.Random(0))

    sub_events = [se for se in solution.events if se.event_ref == "E1"]
    assert len(sub_events) >= 2
    for se in sub_events:
        assert 1 <= se.duration <= 2
    assert sum(se.duration for se in sub_events) == 3


def test_build_initial_never_places_a_sub_event_past_the_last_time():
    # Regression test for the exact real-world HSEval rejection this fix
    # addresses: "<Time> 'Fr_4' not assignable to <Event> 'T1-S3'" -- a
    # duration-3 event naively placed 2 slots before the end of the week
    # overflowed past the last defined Time. Every generated sub-event's
    # [start, start+duration-1] span must stay within instance.times.
    instance = parse_archive(
        _split_events_archive(duration=3, min_duration=1, max_duration=2)
    )[0]
    all_time_ids = [t.id for t in instance.times]

    for seed in range(20):
        solution = build_initial(instance, random.Random(seed))
        for se in solution.events:
            start = all_time_ids.index(se.time_ref)
            assert start + se.duration <= len(all_time_ids), (
                f"seed={seed}: {se.event_ref} at {se.time_ref} dur={se.duration} overflows"
            )


def test_build_initial_keeps_a_single_piece_when_duration_already_fits():
    # No splitting needed when the event's duration is already within
    # MaximumDuration -- must not fragment it unnecessarily.
    instance = parse_archive(
        _split_events_archive(duration=2, min_duration=1, max_duration=2)
    )[0]

    solution = build_initial(instance, random.Random(0))

    sub_events = [se for se in solution.events if se.event_ref == "E1"]
    assert len(sub_events) == 1
    assert sub_events[0].duration == 2


def test_build_initial_splits_brazil_instance_without_overflowing_any_time():
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    all_time_ids = [t.id for t in instance.times]

    solution = build_initial(instance, random.Random(0))

    for se in solution.events:
        start = all_time_ids.index(se.time_ref)
        assert start + se.duration <= len(all_time_ids)
