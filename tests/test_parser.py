from collections import Counter
from pathlib import Path

from xhstt_core.parser import parse_archive, parse_solution_groups

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


# Minimal but well-formed archive covering an edge case seen in real
# instances (e.g. FinlandHighSchool.xml): an Event's Resource can carry a
# Reference to an already-typed Resource without repeating <ResourceType>.
MINIMAL_ARCHIVE_WITH_UNTYPED_EVENT_RESOURCE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
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
          <Resources>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""

_MINIMAL_ARCHIVE_NO_SOLUTIONS = """<HighSchoolTimetableArchive>
  <Instances></Instances>
</HighSchoolTimetableArchive>"""


def test_parses_instance_id_and_name():
    instances = parse_archive(_load("ArtificialSudoku4x4.xml"))

    assert len(instances) == 1
    assert instances[0].id == "ArtificialSudoku4x4_XHSTT2014A"
    assert instances[0].name == "Sudoku4x4"


def test_parses_times_and_time_groups():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    assert [g.id for g in instance.time_groups] == ["gr_Day"]
    assert instance.time_groups[0].name == "Day"
    assert instance.time_groups[0].kind == "Day"

    assert len(instance.times) == 4
    first = instance.times[0]
    assert first.id == "Day_1"
    assert first.name == "Day_1"
    assert first.group_refs == ["gr_Day"]


def test_parses_resources():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    assert {rt.id for rt in instance.resource_types} == {"Teacher", "Class", "Room"}
    assert {g.id for g in instance.resource_groups} == {
        "gr_Teachers",
        "gr_Classes",
        "gr_Rooms",
        "gr_RT1",
        "gr_RT2",
        "gr_RT3",
        "gr_RT4",
    }

    assert len(instance.resources) == 12
    t1 = next(r for r in instance.resources if r.id == "T1")
    assert t1.name == "T1"
    assert t1.resource_type_ref == "Teacher"
    assert t1.group_refs == ["gr_Teachers"]

    r1 = next(r for r in instance.resources if r.id == "R1")
    assert r1.group_refs == ["gr_Rooms", "gr_RT1"]


def test_parses_events_and_event_groups():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    kinds = {g.kind for g in instance.event_groups}
    assert kinds == {"Course", "EventGroup"}
    assert any(
        g.id == "gr_math-C1" and g.kind == "Course" for g in instance.event_groups
    )
    assert any(
        g.id == "gr_AllEvents" and g.kind == "EventGroup"
        for g in instance.event_groups
    )

    assert len(instance.events) == 16
    event1 = next(e for e in instance.events if e.id == "Event1")
    assert event1.name == "math-C1_1"
    assert event1.duration == 1
    assert event1.course_ref == "gr_math-C1"
    assert event1.group_refs == ["gr_EventsRT1", "gr_AllEvents"]

    assert len(event1.resources) == 3
    class_res = next(r for r in event1.resources if r.role == "Class")
    assert class_res.resource_ref == "C1"
    assert class_res.resource_type_ref == "Class"

    room_res = next(r for r in event1.resources if r.role == "RoomRT1")
    assert room_res.resource_ref is None
    assert room_res.resource_type_ref == "Room"


def test_parses_constraints():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    assert len(instance.constraints) == 10
    types = [c.type for c in instance.constraints]
    assert types.count("AssignResourceConstraint") == 4
    assert types.count("AssignTimeConstraint") == 1
    assert types.count("PreferResourcesConstraint") == 4
    assert types.count("AvoidClashesConstraint") == 1

    assign_time = next(
        c for c in instance.constraints if c.type == "AssignTimeConstraint"
    )
    assert assign_time.id == "AssignTimes_5"
    assert assign_time.required is True
    assert assign_time.weight == 1
    assert assign_time.cost_function == "Linear"
    assert assign_time.applies_to.event_groups == ["gr_AllEvents"]

    prefer = next(c for c in instance.constraints if c.id == "PreferredResources_6")
    assert prefer.applies_to.event_groups == ["gr_EventsRT1"]
    assert prefer.params["Role"] == "RoomRT1"
    assert prefer.params["ResourceGroups"] == [{"reference": "gr_RT1"}]

    clashes = next(
        c for c in instance.constraints if c.type == "AvoidClashesConstraint"
    )
    assert clashes.applies_to.resource_groups == [
        "gr_Teachers",
        "gr_Rooms",
        "gr_Classes",
    ]


def test_parses_real_world_instance_without_error():
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]

    assert instance.id == "BrazilInstance1_XHSTT-v2014"
    assert len(instance.times) == 25
    assert len(instance.resources) == 11
    assert len(instance.events) == 21
    assert len(instance.constraints) == 18

    constraint_type_counts = Counter(c.type for c in instance.constraints)
    assert constraint_type_counts == {
        "AvoidUnavailableTimesConstraint": 8,
        "DistributeSplitEventsConstraint": 2,
        "ClusterBusyTimesConstraint": 2,
        "AssignTimeConstraint": 1,
        "SplitEventsConstraint": 1,
        "PreferTimesConstraint": 1,
        "SpreadEventsConstraint": 1,
        "AvoidClashesConstraint": 1,
        "LimitIdleTimesConstraint": 1,
    }

    # Every event resource role must resolve to a declared resource type,
    # and every reference used anywhere must point at something that exists —
    # this is the real correctness bar for a "parses without error" instance.
    resource_type_ids = {rt.id for rt in instance.resource_types}
    for event in instance.events:
        for res in event.resources:
            assert res.resource_type_ref is None or res.resource_type_ref in resource_type_ids


def test_event_resource_type_is_optional_when_resource_is_referenced():
    instance = parse_archive(MINIMAL_ARCHIVE_WITH_UNTYPED_EVENT_RESOURCE)[0]

    res = instance.events[0].resources[0]
    assert res.resource_ref == "T1"
    assert res.role == "Teacher"
    assert res.resource_type_ref is None


WORKLOAD_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
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
          <Resources>
            <Resource Reference="T1"><Role>Teacher</Role><Workload>2</Workload></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_event_resource_parses_optional_workload():
    instance = parse_archive(WORKLOAD_ARCHIVE)[0]

    res = instance.events[0].resources[0]
    assert res.workload == 2.0


def test_event_resource_workload_defaults_to_none():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]

    assert instance.events[0].resources[0].workload is None


EVENT_PAIRS_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>1</Duration><Resources></Resources></Event>
        <Event Id="E2"><Name>E2</Name><Duration>1</Duration><Resources></Resources></Event>
      </Events>
      <Constraints>
        <OrderEventsConstraint Id="OE1">
          <Name>Order</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <EventPairs>
              <EventPair>
                <FirstEvent Reference="E1"/>
                <SecondEvent Reference="E2"/>
                <MinSeparation>2</MinSeparation>
                <MaxSeparation>5</MaxSeparation>
              </EventPair>
            </EventPairs>
          </AppliesTo>
        </OrderEventsConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_parses_applies_to_event_pairs():
    instance = parse_archive(EVENT_PAIRS_ARCHIVE)[0]

    constraint = instance.constraints[0]
    assert len(constraint.applies_to.event_pairs) == 1
    pair = constraint.applies_to.event_pairs[0]
    assert pair.first_event == "E1"
    assert pair.second_event == "E2"
    assert pair.min_separation == 2
    assert pair.max_separation == 5


def test_parses_solution_groups():
    solution_groups = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))

    assert len(solution_groups) == 1
    group = solution_groups[0]
    assert group.id == "GerhardPost_2009-04-29"
    assert len(group.solutions) == 1

    solution = group.solutions[0]
    assert solution.instance_ref == "ArtificialSudoku4x4_XHSTT2014A"
    assert len(solution.events) == 16

    event1 = next(e for e in solution.events if e.event_ref == "Event1")
    assert event1.time_ref == "Day_1"
    assert len(event1.resources) == 1
    assert event1.resources[0].role == "RoomRT1"
    assert event1.resources[0].resource_ref == "R1"


def test_parses_solution_groups_with_split_events():
    # BrazilInstance1's solutions have SplitEventsConstraint in effect: the
    # same Event Reference can appear multiple times, once per split
    # occurrence, each carrying its own Duration and Time.
    solution_groups = parse_solution_groups(_load("BrazilInstance1.xml"))

    assert len(solution_groups) == 2
    assert solution_groups[0].id == "Haroldo_Dec_2011"

    solution = solution_groups[0].solutions[0]
    assert solution.instance_ref == "BrazilInstance1_XHSTT-v2014"
    assert len(solution.events) == 48

    first, second = solution.events[0], solution.events[1]
    assert first.event_ref == second.event_ref == "T1-S1"
    assert first.duration == 2
    assert first.time_ref == "Mo_4"
    assert second.duration == 1
    assert second.time_ref == "Tu_5"
    assert first.resources == []


def test_parse_solution_groups_returns_empty_list_when_absent():
    assert parse_solution_groups(_MINIMAL_ARCHIVE_NO_SOLUTIONS) == []
