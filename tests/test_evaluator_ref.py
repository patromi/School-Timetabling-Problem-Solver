from pathlib import Path

import pytest

from xhstt_core.evaluator_ref import (
    apply_cost_function,
    evaluate_constraint,
    resolve_occurrences,
    total_cost,
)
from xhstt_core.model import (
    AppliesTo,
    Constraint,
    EventPair,
    Solution,
    SolutionEvent,
    SolutionEventResource,
)
from xhstt_core.parser import parse_archive, parse_solution_groups

DURATION_3_EVENT_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources>
        <ResourceTypes></ResourceTypes>
        <ResourceGroups></ResourceGroups>
      </Resources>
      <Events>
        <EventGroups>
          <EventGroup Id="gr_All"><Name>All</Name></EventGroup>
        </EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>3</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_All"/></EventGroups>
        </Event>
      </Events>
      <Constraints>
        <AssignTimeConstraint Id="AT1">
          <Name>AssignTimes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <EventGroups><EventGroup Reference="gr_All"/></EventGroups>
          </AppliesTo>
        </AssignTimeConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def test_resolve_occurrences_merges_fixed_and_solution_resources():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]

    occurrences = resolve_occurrences(instance, solution)

    assert len(occurrences) == 16
    event1 = next(o for o in occurrences if o.event_ref == "Event1")
    assert event1.duration == 1
    assert event1.time_ref == "Day_1"
    assert dict(event1.resource_assignments) == {
        "Class": "C1",
        "Teacher": "T1",
        "RoomRT1": "R1",
    }


FULLY_PREASSIGNED_EVENT_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="Tue_8"><Name>Tue_8</Name></Time></Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="StaffMeeting_1">
          <Name>StaffMeeting_1</Name>
          <Duration>1</Duration>
          <Time Reference="Tue_8"/>
          <Resources>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


MANY_ATTENDEES_SHARED_EMPTY_ROLE_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="Tue_8"><Name>Tue_8</Name></Time></Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="T2"><Name>T2</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="T3"><Name>T3</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="StaffMeeting_1">
          <Name>StaffMeeting_1</Name>
          <Duration>1</Duration>
          <Time Reference="Tue_8"/>
          <Resources>
            <Resource Reference="T1"/>
            <Resource Reference="T2"/>
            <Resource Reference="T3"/>
          </Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_resolve_occurrences_keeps_all_preassigned_resources_sharing_an_omitted_role():
    # Real-world pattern confirmed in AustraliaBGHS98.xml (StaffMeeting_1):
    # a fully preassigned event listing many attendees, ALL with Role
    # omitted (spec allows this when Reference is present) -- so they all
    # share the same "" role. A role-keyed dict would silently collapse
    # all but the last attendee.
    instance = parse_archive(MANY_ATTENDEES_SHARED_EMPTY_ROLE_ARCHIVE)[0]
    solution = Solution(instance_ref=instance.id, events=[])

    occurrences = resolve_occurrences(instance, solution)

    assert len(occurrences) == 1
    assigned_resources = [r for _, r in occurrences[0].resource_assignments]
    assert sorted(assigned_resources) == ["T1", "T2", "T3"]


def test_resolve_occurrences_synthesizes_occurrence_for_fully_preassigned_event_absent_from_solution():
    # Real-world pattern confirmed in AustraliaBGHS98.xml: a fully
    # preassigned event (time + all resources fixed) never appears in
    # <Solution><Events> at all -- it still "happens" and must be visible
    # to the evaluator (e.g. for clash detection).
    instance = parse_archive(FULLY_PREASSIGNED_EVENT_ARCHIVE)[0]
    solution = Solution(instance_ref=instance.id, events=[])

    occurrences = resolve_occurrences(instance, solution)

    assert len(occurrences) == 1
    occ = occurrences[0]
    assert occ.event_ref == "StaffMeeting_1"
    assert occ.duration == 1
    assert occ.time_ref == "Tue_8"
    assert dict(occ.resource_assignments) == {"Teacher": "T1"}


def test_resolve_occurrences_falls_back_to_preassigned_time_when_solution_event_omits_it():
    # Real-world pattern confirmed for a partially-preassigned event
    # (Sport_1 in AustraliaBGHS98.xml): it DOES appear in the solution
    # (because some other role still needs solving) but the solution's
    # <Event> entry omits <Time> since it's already fixed at instance level.
    instance = parse_archive(FULLY_PREASSIGNED_EVENT_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="StaffMeeting_1", time_ref=None)],
    )

    occurrences = resolve_occurrences(instance, solution)

    assert len(occurrences) == 1
    assert occurrences[0].time_ref == "Tue_8"


@pytest.mark.parametrize(
    "name,deviation,expected",
    [
        ("Linear", 0, 0),
        ("Linear", 3, 3),
        ("Quadratic", 3, 9),
        ("Step", 0, 0),
        ("Step", 1, 1),
        ("Step", 5, 1),
    ],
)
def test_apply_cost_function(name, deviation, expected):
    assert apply_cost_function(name, deviation) == expected


COURSE_AS_EVENT_GROUP_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups>
          <Course Id="gr_MyCourse"><Name>MyCourse</Name></Course>
        </EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Course Reference="gr_MyCourse"/>
          <Resources></Resources>
        </Event>
      </Events>
      <Constraints>
        <AssignTimeConstraint Id="AT1">
          <Name>AssignTimes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <EventGroups><EventGroup Reference="gr_MyCourse"/></EventGroups>
          </AppliesTo>
        </AssignTimeConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_course_reference_counts_as_event_group_membership():
    # Spec: "Course is an alternative form for EventGroup." An event's
    # <Course Reference="X"/> must count as membership in event group X for
    # AppliesTo.EventGroups matching, exactly like an explicit
    # <EventGroups><EventGroup Reference="X"/></EventGroups> entry would.
    # Confirmed as a real bug via direct HSEval comparison on
    # BrazilInstance1 (DistributeSplitEventsConstraint referenced
    # Course-only event groups and silently matched zero events).
    instance = parse_archive(COURSE_AS_EVENT_GROUP_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref=None)],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # If Course membership weren't counted, this event would fall outside
    # AppliesTo and the constraint would have zero points of application,
    # so cost would come out 0 -- deviation must be 1 (E1's duration, unassigned).
    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_assign_time_constraint_is_zero_on_a_fully_assigned_reference_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    constraint = next(
        c for c in instance.constraints if c.type == "AssignTimeConstraint"
    )

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_assign_time_constraint_counts_events_missing_a_time():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    occurrences[0].time_ref = None
    constraint = next(
        c for c in instance.constraints if c.type == "AssignTimeConstraint"
    )

    # Linear cost function, Weight=1 in the fixture -> deviation count as-is.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_assign_time_constraint_deviation_is_duration_weighted():
    # Per Kristiansen et al. 2015 (see memory), the deviation is the summed
    # *duration* of unassigned sub-events, not a flat count of 1 per event.
    instance = parse_archive(DURATION_3_EVENT_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref=None)],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 3


def test_avoid_clashes_constraint_is_zero_on_a_feasible_reference_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    constraint = next(
        c for c in instance.constraints if c.type == "AvoidClashesConstraint"
    )

    assert evaluate_constraint(instance, occurrences, constraint) == 0


TWO_EVENTS_SHARED_TEACHER_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups><Time Id="Day_1"><Name>Day_1</Name></Time></Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups>
          <ResourceGroup Id="gr_Teachers"><Name>Teachers</Name></ResourceGroup>
        </ResourceGroups>
        <Resource Id="T1">
          <Name>T1</Name>
          <ResourceType Reference="Teacher"/>
          <ResourceGroups><ResourceGroup Reference="gr_Teachers"/></ResourceGroups>
        </Resource>
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
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints>
        <AvoidClashesConstraint Id="AC1">
          <Name>NoClashes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo>
            <ResourceGroups><ResourceGroup Reference="gr_Teachers"/></ResourceGroups>
          </AppliesTo>
        </AvoidClashesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_avoid_clashes_constraint_detects_clash_from_overlapping_duration_not_shared_start():
    # Confirmed real bug (via direct HSEval comparison on BrazilInstance1):
    # "busy" must include ALL time slots a duration>1 occurrence spans, not
    # just its start. E1 (dur 2, starts T1) occupies T1,T2. E2 (dur 1,
    # starts T2) occupies T2 only. They don't share a START time but DO
    # clash at T2 -- a start-time-only Counter would miss this entirely.
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups><ResourceGroup Id="gr_Teachers"><Name>Teachers</Name></ResourceGroup></ResourceGroups>
        <Resource Id="T1r"><Name>T1r</Name><ResourceType Reference="Teacher"/>
          <ResourceGroups><ResourceGroup Reference="gr_Teachers"/></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>2</Duration>
          <Resources><Resource Reference="T1r"><Role>Teacher</Role></Resource></Resources></Event>
        <Event Id="E2"><Name>E2</Name><Duration>1</Duration>
          <Resources><Resource Reference="T1r"><Role>Teacher</Role></Resource></Resources></Event>
      </Events>
      <Constraints>
        <AvoidClashesConstraint Id="AC1">
          <Name>NoClashes</Name><Required>true</Required><Weight>1</Weight><CostFunction>Linear</CostFunction>
          <AppliesTo><ResourceGroups><ResourceGroup Reference="gr_Teachers"/></ResourceGroups></AppliesTo>
        </AvoidClashesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1"),
            SolutionEvent(event_ref="E2", time_ref="T2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_avoid_clashes_constraint_counts_double_bookings():
    instance = parse_archive(TWO_EVENTS_SHARED_TEACHER_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1"),
            SolutionEvent(event_ref="E2", time_ref="Day_1"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # T1 is double-booked at Day_1 by E1 and E2 -> deviation = 2 - 1 = 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_assign_resource_constraint_is_zero_on_a_fully_assigned_reference_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    constraint = next(
        c for c in instance.constraints if c.id == "AssignResources_1"
    )

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_assign_resource_constraint_counts_unassigned_role_duration():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    constraint = next(
        c for c in instance.constraints if c.id == "AssignResources_1"
    )

    event1 = next(o for o in occurrences if o.event_ref == "Event1")
    assert event1.duration == 1
    event1.resource_assignments = [
        (role, None if role == "RoomRT1" else ref)
        for role, ref in event1.resource_assignments
    ]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_prefer_resources_constraint_is_zero_on_a_fully_assigned_reference_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    constraint = next(
        c for c in instance.constraints if c.id == "PreferredResources_6"
    )

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_prefer_resources_constraint_counts_non_preferred_assignment():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)
    constraint = next(
        c for c in instance.constraints if c.id == "PreferredResources_6"
    )

    # PreferredResources_6 prefers gr_RT1 (R1) for RoomRT1 on gr_EventsRT1
    # events. Assign R2 (a gr_RT2 room) instead -> not preferred.
    event1 = next(o for o in occurrences if o.event_ref == "Event1")
    assert event1.duration == 1
    event1.resource_assignments = [
        (role, "R2" if role == "RoomRT1" else ref)
        for role, ref in event1.resource_assignments
    ]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_total_cost_is_zero_on_a_fully_feasible_reference_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]

    assert total_cost(instance, solution) == 0


INFEASIBILITY_VS_OBJECTIVE_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups>
          <TimeGroup Id="gr_Preferred"><Name>Preferred</Name></TimeGroup>
        </TimeGroups>
        <Time Id="P1"><Name>P1</Name><TimeGroups><TimeGroup Reference="gr_Preferred"/></TimeGroups></Time>
        <Time Id="P2"><Name>P2</Name></Time>
      </Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events>
        <EventGroups><EventGroup Id="gr_All"><Name>All</Name></EventGroup></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_All"/></EventGroups>
        </Event>
      </Events>
      <Constraints>
        <AssignTimeConstraint Id="AT1">
          <Name>AssignTimes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_All"/></EventGroups></AppliesTo>
        </AssignTimeConstraint>
        <PreferTimesConstraint Id="PT1">
          <Name>PreferTimes</Name>
          <Required>false</Required>
          <Weight>100</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_All"/></EventGroups></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_Preferred"/></TimeGroups>
        </PreferTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_total_cost_weighs_infeasibility_lexicographically_above_objective():
    # A single point of infeasibility must outweigh any amount of
    # objective-only cost, matching the plan's Env sketch
    # (infeas * 1_000_000 + obj).
    instance = parse_archive(INFEASIBILITY_VS_OBJECTIVE_ARCHIVE)[0]
    # Feasible (time assigned) but costly: lands on the non-preferred time,
    # incurring a large (weight=100) soft PreferTimesConstraint cost.
    feasible_but_costly = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="P2")],
    )
    # Infeasible (no time assigned) but cheap: violates the hard
    # AssignTimeConstraint by 1, but PreferTimesConstraint ignores
    # unassigned solution events entirely (spec), so its cost is 0.
    infeasible_but_cheap = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref=None)],
    )

    assert total_cost(instance, infeasible_but_cheap) > total_cost(
        instance, feasible_but_costly
    )


def test_full_reference_solution_has_zero_total_cost():
    # Sudoku4x4 only uses these 4 constraint types -> every constraint the
    # evaluator understands is exercised, closing the loop toward gate G1.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]
    occurrences = resolve_occurrences(instance, solution)

    total = sum(
        evaluate_constraint(instance, occurrences, c) for c in instance.constraints
    )

    assert total == 0


TWO_DAYS_ONE_TEACHER_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <TimeGroup Id="gr_Mon"><Name>Mon</Name></TimeGroup>
          <TimeGroup Id="gr_Tue"><Name>Tue</Name></TimeGroup>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><TimeGroups><TimeGroup Reference="gr_Tue"/></TimeGroups></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1">
          <Name>T1</Name>
          <ResourceType Reference="Teacher"/>
          <ResourceGroups></ResourceGroups>
        </Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <ClusterBusyTimesConstraint Id="CB1">
          <Name>MaxOneDay</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <TimeGroups>
            <TimeGroup Reference="gr_Mon"/>
            <TimeGroup Reference="gr_Tue"/>
          </TimeGroups>
          <Minimum>0</Minimum>
          <Maximum>1</Maximum>
        </ClusterBusyTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_cluster_busy_times_constraint_counts_group_active_via_spanned_time_not_just_start():
    # T1 group has only P2 (a single-time group). An event starting at P1
    # (outside the group) with duration 2 spans P1,P2 -- P2 IS in the
    # group, so the resource is "active" there even though the start
    # isn't. A start-only check would miss this.
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups><TimeGroup Id="gr_G"><Name>G</Name></TimeGroup></TimeGroups>
        <Time Id="P1"><Name>P1</Name></Time>
        <Time Id="P2"><Name>P2</Name><TimeGroups><TimeGroup Reference="gr_G"/></TimeGroups></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>2</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
      </Events>
      <Constraints>
        <ClusterBusyTimesConstraint Id="CB1">
          <Name>MustBeActive</Name><Required>false</Required><Weight>1</Weight><CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_G"/></TimeGroups>
          <Minimum>1</Minimum><Maximum>1</Maximum>
        </ClusterBusyTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="P1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_cluster_busy_times_constraint_is_zero_when_within_maximum():
    instance = parse_archive(TWO_DAYS_ONE_TEACHER_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Mon_1"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Both events land on Monday -> 1 active day, within Maximum=1.
    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_cluster_busy_times_constraint_counts_excess_active_time_groups():
    instance = parse_archive(TWO_DAYS_ONE_TEACHER_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Tue_1"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # T1 is now busy on both Monday and Tuesday -> 2 active days, exceeding
    # Maximum=1 by 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


ONE_TEACHER_UNAVAILABLE_MONDAY_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1">
          <Name>T1</Name>
          <ResourceType Reference="Teacher"/>
          <ResourceGroups></ResourceGroups>
        </Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <AvoidUnavailableTimesConstraint Id="AU1">
          <Name>T1Unavailable</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <Times><Time Reference="Mon_1"/></Times>
        </AvoidUnavailableTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_avoid_unavailable_times_constraint_detects_violation_via_spanned_time():
    # T1 unavailable at Tue_1. Event starts at Mon_1 with duration 2,
    # spanning Mon_1,Tue_1 -- Tue_1 IS occupied even though it isn't the
    # start. A start-only check would miss this violation entirely.
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>2</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
      </Events>
      <Constraints>
        <AvoidUnavailableTimesConstraint Id="AU1">
          <Name>T1Unavailable</Name><Required>true</Required><Weight>1</Weight><CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <Times><Time Reference="Tue_1"/></Times>
        </AvoidUnavailableTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Mon_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_avoid_unavailable_times_constraint_is_zero_when_avoided():
    instance = parse_archive(ONE_TEACHER_UNAVAILABLE_MONDAY_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Tue_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_avoid_unavailable_times_constraint_counts_use_of_unavailable_time():
    instance = parse_archive(ONE_TEACHER_UNAVAILABLE_MONDAY_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Mon_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


FOUR_PERIODS_ONE_TEACHER_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <TimeGroup Id="gr_Day"><Name>Day</Name></TimeGroup>
        </TimeGroups>
        <Time Id="P1"><Name>P1</Name><TimeGroups><TimeGroup Reference="gr_Day"/></TimeGroups></Time>
        <Time Id="P2"><Name>P2</Name><TimeGroups><TimeGroup Reference="gr_Day"/></TimeGroups></Time>
        <Time Id="P3"><Name>P3</Name><TimeGroups><TimeGroup Reference="gr_Day"/></TimeGroups></Time>
        <Time Id="P4"><Name>P4</Name><TimeGroups><TimeGroup Reference="gr_Day"/></TimeGroups></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1">
          <Name>T1</Name>
          <ResourceType Reference="Teacher"/>
          <ResourceGroups></ResourceGroups>
        </Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <LimitIdleTimesConstraint Id="LI1">
          <Name>NoIdle</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_Day"/></TimeGroups>
          <Minimum>0</Minimum>
          <Maximum>0</Maximum>
        </LimitIdleTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_limit_idle_times_constraint_counts_gap_using_full_duration_span():
    # Reproduces the exact real-world bug found via direct HSEval
    # comparison on BrazilInstance1 (teacher T2): a duration-2 occurrence
    # starting at P1 occupies P1,P2, and a duration-1 occurrence at P4
    # occupies P4 only. Start-only busy-checking would see gaps at P2,P3
    # (2 idle); full-span busy-checking correctly sees only P3 idle (1).
    instance = parse_archive(FOUR_PERIODS_ONE_TEACHER_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="P1", duration=2),
            SolutionEvent(event_ref="E2", time_ref="P4"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_limit_idle_times_constraint_is_zero_when_no_gap():
    instance = parse_archive(FOUR_PERIODS_ONE_TEACHER_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="P1"),
            SolutionEvent(event_ref="E2", time_ref="P2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_limit_idle_times_constraint_counts_gap_between_busy_times():
    instance = parse_archive(FOUR_PERIODS_ONE_TEACHER_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="P1"),
            SolutionEvent(event_ref="E2", time_ref="P3"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Busy at P1 and P3 -> P2 is idle (busy before and after) -> deviation 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


def _limit_busy_times_quadratic_archive() -> str:
    # 4 Mon periods (2 events land there, exceeding Maximum=0 by 2) and 4
    # Tue periods (2 events land there too, exceeding Maximum=0 by 2).
    return """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <TimeGroup Id="gr_Mon"><Name>Mon</Name></TimeGroup>
          <TimeGroup Id="gr_Tue"><Name>Tue</Name></TimeGroup>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><TimeGroups><TimeGroup Reference="gr_Tue"/></TimeGroups></Time>
        <Time Id="Tue_2"><Name>Tue_2</Name><TimeGroups><TimeGroup Reference="gr_Tue"/></TimeGroups></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
        <Event Id="E2"><Name>E2</Name><Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
        <Event Id="E3"><Name>E3</Name><Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
        <Event Id="E4"><Name>E4</Name><Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
      </Events>
      <Constraints>
        <LimitBusyTimesConstraint Id="LB1">
          <Name>LimitBusy</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Quadratic</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <TimeGroups>
            <TimeGroup Reference="gr_Mon"/>
            <TimeGroup Reference="gr_Tue"/>
          </TimeGroups>
          <Minimum>0</Minimum>
          <Maximum>0</Maximum>
        </LimitBusyTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_limit_busy_times_constraint_sums_deviations_before_applying_quadratic_cost_function():
    # Spec (verbatim): "the deviation ... is the SUM, over all time groups,
    # of the amount by which ... falls short of Minimum or exceeds
    # Maximum" -- ONE deviation per resource, CostFunction applied once.
    # Mon: 2 busy times vs Maximum=0 -> group deviation 2.
    # Tue: 2 busy times vs Maximum=0 -> group deviation 2.
    # Summed deviation = 4; Quadratic(4) = 16 -- NOT Quadratic(2)+Quadratic(2)=8.
    instance = parse_archive(_limit_busy_times_quadratic_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Mon_2"),
            SolutionEvent(event_ref="E3", time_ref="Tue_1"),
            SolutionEvent(event_ref="E4", time_ref="Tue_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 16


TWO_MON_PERIODS_ONE_TUE_PERIOD_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <TimeGroup Id="gr_Mon"><Name>Mon</Name></TimeGroup>
          <TimeGroup Id="gr_Tue"><Name>Tue</Name></TimeGroup>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><TimeGroups><TimeGroup Reference="gr_Tue"/></TimeGroups></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1">
          <Name>T1</Name>
          <ResourceType Reference="Teacher"/>
          <ResourceGroups></ResourceGroups>
        </Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <LimitBusyTimesConstraint Id="LB1">
          <Name>LimitBusy</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <TimeGroups>
            <TimeGroup Reference="gr_Mon"/>
            <TimeGroup Reference="gr_Tue"/>
          </TimeGroups>
          <Minimum>1</Minimum>
          <Maximum>1</Maximum>
        </LimitBusyTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def _workload_default_chain_archive() -> str:
    # EventResource has no <Workload> of its own -> must fall back to the
    # enclosing Event's <Workload> (5), NOT a flat 1.0.
    return """<HighSchoolTimetableArchive>
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
          <Workload>5</Workload>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <LimitWorkloadConstraint Id="LW1">
          <Name>LimitWorkload</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <Minimum>5</Minimum>
          <Maximum>5</Maximum>
        </LimitWorkloadConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_limit_workload_constraint_event_resource_falls_back_to_event_workload():
    instance = parse_archive(_workload_default_chain_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Day_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # If the (wrong) flat default of 1.0 were used, total workload would be
    # 1, falling short of Minimum=5 by 4. With the correct fallback to the
    # Event's Workload=5, total workload is 5 -> within bounds.
    assert evaluate_constraint(instance, occurrences, constraint) == 0


def _workload_split_rounding_archive() -> str:
    # Event duration=2, Workload=1 (defaults through to EventResource).
    # Split into two duration-1 sub-events -> each contributes
    # Workload(sr) = 1 * 1 / 2 = 0.5. Per-term rounding (spec) gives
    # ceil(0.5)+ceil(0.5)=2; rounding the total once would give ceil(1.0)=1.
    return """<HighSchoolTimetableArchive>
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
          <Duration>2</Duration>
          <Workload>1</Workload>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources>
        </Event>
      </Events>
      <Constraints>
        <LimitWorkloadConstraint Id="LW1">
          <Name>LimitWorkload</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <Minimum>2</Minimum>
          <Maximum>2</Maximum>
        </LimitWorkloadConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_limit_workload_constraint_rounds_each_solution_resource_before_summing():
    instance = parse_archive(_workload_split_rounding_archive())[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1", duration=1),
            SolutionEvent(event_ref="E1", time_ref="Day_2", duration=1),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Per-term rounding gives total workload 2 -> within [Minimum=2,
    # Maximum=2] -> deviation 0. (Rounding the raw sum 1.0 once would also
    # give 1 -> deviation 1 (short of Minimum=2) -- this test only proves
    # per-term rounding lands exactly on bounds where the wrong strategy
    # wouldn't; see the shortfall-detecting variant below for a case that
    # actually distinguishes the two strategies by an observable difference.)
    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_limit_workload_constraint_per_term_rounding_differs_from_total_rounding():
    instance = parse_archive(_workload_split_rounding_archive())[0]
    # Same split as above, but require exactly Minimum=Maximum=3: per-term
    # rounding gives total=2 (short by 1); total-then-round would give
    # ceil(0.5+0.5)=1 (short by 2). The two strategies disagree by 1.
    instance.constraints[0].params["Minimum"] = "3"
    instance.constraints[0].params["Maximum"] = "3"
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1", duration=1),
            SolutionEvent(event_ref="E1", time_ref="Day_2", duration=1),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 1


def _workload_archive(minimum: int, maximum: int) -> str:
    return f"""<HighSchoolTimetableArchive>
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
      <Constraints>
        <LimitWorkloadConstraint Id="LW1">
          <Name>LimitWorkload</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <Minimum>{minimum}</Minimum>
          <Maximum>{maximum}</Maximum>
        </LimitWorkloadConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_limit_workload_constraint_is_zero_within_bounds():
    instance = parse_archive(_workload_archive(minimum=2, maximum=2))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Day_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Workload=2, sub-event duration == event duration -> total workload 2.
    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_limit_workload_constraint_counts_shortfall():
    instance = parse_archive(_workload_archive(minimum=3, maximum=3))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Day_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Total workload 2 falls short of Minimum=3 by 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


def _single_event_archive(constraint_xml: str) -> str:
    return f"""<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources>
        <ResourceTypes></ResourceTypes>
        <ResourceGroups></ResourceGroups>
      </Resources>
      <Events>
        <EventGroups>
          <EventGroup Id="gr_All"><Name>All</Name></EventGroup>
        </EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>3</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_All"/></EventGroups>
        </Event>
      </Events>
      <Constraints>{constraint_xml}</Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


SPLIT_EVENTS_CONSTRAINT_XML = """
        <SplitEventsConstraint Id="SE1">
          <Name>Split</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_All"/></EventGroups></AppliesTo>
          <MinimumDuration>1</MinimumDuration>
          <MaximumDuration>2</MaximumDuration>
          <MinimumAmount>1</MinimumAmount>
          <MaximumAmount>999</MaximumAmount>
        </SplitEventsConstraint>"""


def test_split_events_constraint_is_zero_when_within_bounds():
    instance = parse_archive(_single_event_archive(SPLIT_EVENTS_CONSTRAINT_XML))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1", duration=2),
            SolutionEvent(event_ref="E1", time_ref="Day_2", duration=1),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_split_events_constraint_counts_out_of_range_duration():
    instance = parse_archive(_single_event_archive(SPLIT_EVENTS_CONSTRAINT_XML))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1", duration=3),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Duration 3 > MaximumDuration=2 -> 1 sub-event out of range; amount (1)
    # is within [MinimumAmount=1, MaximumAmount=999].
    assert evaluate_constraint(instance, occurrences, constraint) == 1


DISTRIBUTE_SPLIT_CONSTRAINT_XML = """
        <DistributeSplitEventsConstraint Id="DS1">
          <Name>AtLeastOneDouble</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_All"/></EventGroups></AppliesTo>
          <Duration>2</Duration>
          <Minimum>1</Minimum>
          <Maximum>1</Maximum>
        </DistributeSplitEventsConstraint>"""


def test_distribute_split_events_constraint_is_zero_when_within_bounds():
    instance = parse_archive(_single_event_archive(DISTRIBUTE_SPLIT_CONSTRAINT_XML))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1", duration=2),
            SolutionEvent(event_ref="E1", time_ref="Day_2", duration=1),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Exactly 1 sub-event of duration 2 -> within [Minimum=1, Maximum=1].
    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_distribute_split_events_constraint_counts_shortfall():
    instance = parse_archive(_single_event_archive(DISTRIBUTE_SPLIT_CONSTRAINT_XML))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Day_1", duration=1),
            SolutionEvent(event_ref="E1", time_ref="Day_2", duration=1),
            SolutionEvent(event_ref="E1", time_ref="Day_3", duration=1),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # 0 sub-events of duration 2 -> falls short of Minimum=1 by 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


PREFER_TIMES_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <TimeGroup Id="gr_Preferred"><Name>Preferred</Name></TimeGroup>
        </TimeGroups>
        <Time Id="P1"><Name>P1</Name><TimeGroups><TimeGroup Reference="gr_Preferred"/></TimeGroups></Time>
        <Time Id="P2"><Name>P2</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes></ResourceTypes>
        <ResourceGroups></ResourceGroups>
      </Resources>
      <Events>
        <EventGroups>
          <EventGroup Id="gr_All"><Name>All</Name></EventGroup>
        </EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>2</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_All"/></EventGroups>
        </Event>
      </Events>
      <Constraints>
        <PreferTimesConstraint Id="PT1">
          <Name>PreferTimes</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_All"/></EventGroups></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_Preferred"/></TimeGroups>
        </PreferTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_prefer_times_constraint_is_zero_on_preferred_time():
    instance = parse_archive(PREFER_TIMES_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="P1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_prefer_times_constraint_counts_duration_of_non_preferred_time():
    instance = parse_archive(PREFER_TIMES_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="P2")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # E1 (duration 2) lands on P2, which isn't in gr_Preferred.
    assert evaluate_constraint(instance, occurrences, constraint) == 2


SPREAD_EVENTS_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups>
          <TimeGroup Id="gr_Mon"><Name>Mon</Name></TimeGroup>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><TimeGroups><TimeGroup Reference="gr_Mon"/></TimeGroups></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes></ResourceTypes>
        <ResourceGroups></ResourceGroups>
      </Resources>
      <Events>
        <EventGroups>
          <EventGroup Id="gr_G"><Name>G</Name></EventGroup>
        </EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_G"/></EventGroups>
        </Event>
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_G"/></EventGroups>
        </Event>
      </Events>
      <Constraints>
        <SpreadEventsConstraint Id="SP1">
          <Name>SpreadMon</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_G"/></EventGroups></AppliesTo>
          <TimeGroups>
            <TimeGroup Reference="gr_Mon"><Minimum>0</Minimum><Maximum>1</Maximum></TimeGroup>
          </TimeGroups>
        </SpreadEventsConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_spread_events_constraint_is_zero_within_per_group_maximum():
    instance = parse_archive(SPREAD_EVENTS_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Tue_1"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_spread_events_constraint_counts_excess_in_one_time_group():
    instance = parse_archive(SPREAD_EVENTS_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Mon_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # Both E1 and E2 start within gr_Mon -> count=2, exceeding that group's
    # own Maximum=1 by 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


TWO_EVENTS_ONE_GROUP_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="Ta"><Name>Ta</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="Tb"><Name>Tb</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups>
          <EventGroup Id="gr_G"><Name>G</Name></EventGroup>
        </EventGroups>
        <Event Id="E1">
          <Name>E1</Name>
          <Duration>1</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_G"/></EventGroups>
        </Event>
        <Event Id="E2">
          <Name>E2</Name>
          <Duration>1</Duration>
          <Resources></Resources>
          <EventGroups><EventGroup Reference="gr_G"/></EventGroups>
        </Event>
      </Events>
      <Constraints>
        <AvoidSplitAssignmentsConstraint Id="AS1">
          <Name>SameTeacher</Name>
          <Required>false</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><EventGroups><EventGroup Reference="gr_G"/></EventGroups></AppliesTo>
          <Role>Teacher</Role>
        </AvoidSplitAssignmentsConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def test_avoid_split_assignments_constraint_is_zero_with_one_shared_resource():
    instance = parse_archive(TWO_EVENTS_ONE_GROUP_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(
                event_ref="E1",
                time_ref="T1",
                resources=[SolutionEventResource(role="Teacher", resource_ref="Ta")],
            ),
            SolutionEvent(
                event_ref="E2",
                time_ref="T2",
                resources=[SolutionEventResource(role="Teacher", resource_ref="Ta")],
            ),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_avoid_split_assignments_constraint_counts_extra_distinct_resources():
    instance = parse_archive(TWO_EVENTS_ONE_GROUP_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(
                event_ref="E1",
                time_ref="T1",
                resources=[SolutionEventResource(role="Teacher", resource_ref="Ta")],
            ),
            SolutionEvent(
                event_ref="E2",
                time_ref="T2",
                resources=[SolutionEventResource(role="Teacher", resource_ref="Tb")],
            ),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # 2 distinct teachers used for the "Teacher" role across the group ->
    # exceeds 1 by 1.
    assert evaluate_constraint(instance, occurrences, constraint) == 1


def test_link_events_constraint_is_zero_when_all_events_share_every_time():
    instance = parse_archive(TWO_EVENTS_ONE_GROUP_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1"),
            SolutionEvent(event_ref="E2", time_ref="T1"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    link_constraint = Constraint(
        type="LinkEventsConstraint",
        id="LE1",
        name="Link",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(event_groups=["gr_G"]),
    )

    assert evaluate_constraint(instance, occurrences, link_constraint) == 0


def test_link_events_constraint_counts_times_not_shared_by_all_events():
    instance = parse_archive(TWO_EVENTS_ONE_GROUP_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1"),
            SolutionEvent(event_ref="E2", time_ref="T2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    link_constraint = Constraint(
        type="LinkEventsConstraint",
        id="LE1",
        name="Link",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(event_groups=["gr_G"]),
    )

    # T1 (only in E1's set) and T2 (only in E2's set) both appear in some
    # but not all member events' time sets -> deviation 2.
    assert evaluate_constraint(instance, occurrences, link_constraint) == 2


THREE_PERIODS_TWO_EVENTS_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups></TimeGroups>
        <Time Id="T1"><Name>T1</Name></Time>
        <Time Id="T2"><Name>T2</Name></Time>
        <Time Id="T3"><Name>T3</Name></Time>
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


def _order_events_constraint(min_separation: int, max_separation: int | None) -> Constraint:
    return Constraint(
        type="OrderEventsConstraint",
        id="OE1",
        name="Order",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(
            event_pairs=[
                EventPair(
                    first_event="E1",
                    second_event="E2",
                    min_separation=min_separation,
                    max_separation=max_separation,
                )
            ]
        ),
    )


def test_order_events_constraint_is_zero_when_separation_satisfied():
    # NOTE: OrderEventsConstraint was never found in any of the 10 real
    # XHSTT-2014 instances sampled while building this evaluator, and the
    # research that produced this formula flagged its own uncertainty on
    # the exact ordinal-subtraction direction. This test only proves
    # self-consistency of the chosen formula, not agreement with HSEval --
    # re-verify against a real instance if one is ever found using it.
    instance = parse_archive(THREE_PERIODS_TWO_EVENTS_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1"),
            SolutionEvent(event_ref="E2", time_ref="T2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = _order_events_constraint(min_separation=0, max_separation=5)

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_order_events_constraint_counts_shortfall_below_min_separation():
    instance = parse_archive(THREE_PERIODS_TWO_EVENTS_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1"),
            SolutionEvent(event_ref="E2", time_ref="T2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    # E1 ends at T1, E2 starts at T2 -> 0 slots between them; require >= 2.
    constraint = _order_events_constraint(min_separation=2, max_separation=5)

    assert evaluate_constraint(instance, occurrences, constraint) == 2


def test_order_events_constraint_is_zero_when_one_of_several_sub_events_is_unassigned():
    # Spec: deviation is 0 if EITHER event has a solution event with
    # unassigned time -- not just when ALL of that event's sub-events lack
    # a time. Here E1 is split into two sub-events, one WITH a time and one
    # WITHOUT; the presence of the unassigned one must zero the deviation,
    # not be silently skipped while computing from the assigned one alone.
    instance = parse_archive(THREE_PERIODS_TWO_EVENTS_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1", duration=1),
            SolutionEvent(event_ref="E1", time_ref=None, duration=1),
            SolutionEvent(event_ref="E2", time_ref="T3"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = _order_events_constraint(min_separation=5, max_separation=5)

    # If the unassigned sub-event were silently skipped, this would compute
    # a large deviation (separation far from [5,5]); the correct behavior
    # is deviation 0.
    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_order_events_constraint_is_zero_when_second_event_unassigned():
    instance = parse_archive(THREE_PERIODS_TWO_EVENTS_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="T1"),
            SolutionEvent(event_ref="E2", time_ref=None),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = _order_events_constraint(min_separation=2, max_separation=5)

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_limit_busy_times_constraint_counts_all_spanned_slots_not_just_start():
    # Group has P1,P2,P3. One event, duration 3, starting at P1, spans all
    # three slots -- busy count should be 3, not 1 (start-only).
    instance = parse_archive(
        """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test</Name></MetaData>
      <Times>
        <TimeGroups><TimeGroup Id="gr_G"><Name>G</Name></TimeGroup></TimeGroups>
        <Time Id="P1"><Name>P1</Name><TimeGroups><TimeGroup Reference="gr_G"/></TimeGroups></Time>
        <Time Id="P2"><Name>P2</Name><TimeGroups><TimeGroup Reference="gr_G"/></TimeGroups></Time>
        <Time Id="P3"><Name>P3</Name><TimeGroups><TimeGroup Reference="gr_G"/></TimeGroups></Time>
      </Times>
      <Resources>
        <ResourceTypes><ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType></ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="T1"><Name>T1</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1"><Name>E1</Name><Duration>3</Duration>
          <Resources><Resource Reference="T1"><Role>Teacher</Role></Resource></Resources></Event>
      </Events>
      <Constraints>
        <LimitBusyTimesConstraint Id="LB1">
          <Name>Limit</Name><Required>false</Required><Weight>1</Weight><CostFunction>Linear</CostFunction>
          <AppliesTo><Resources><Resource Reference="T1"/></Resources></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_G"/></TimeGroups>
          <Minimum>0</Minimum><Maximum>1</Maximum>
        </LimitBusyTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""
    )[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="P1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    # busy count 3 exceeds Maximum=1 by 2.
    assert evaluate_constraint(instance, occurrences, constraint) == 2


def test_limit_busy_times_constraint_zero_busy_group_is_not_penalized():
    # Only E1 is placed; group Tue has 0 busy times. A naive sum-then-
    # threshold (ClusterBusyTimes-style) would charge Minimum-0=1 for Tue,
    # but LimitBusyTimesConstraint has a built-in AllowZero-like carve-out:
    # an empty group is never penalized.
    instance = parse_archive(TWO_MON_PERIODS_ONE_TUE_PERIOD_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Mon_1")],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 0


def test_limit_busy_times_constraint_counts_excess_in_one_group_independently():
    # Both events land on Monday -> Mon busy-count=2, exceeding Maximum=1 by
    # 1. Tue stays at 0 (carve-out, no penalty). This also proves the
    # per-time-group deviation isn't pre-summed like ClusterBusyTimes would.
    instance = parse_archive(TWO_MON_PERIODS_ONE_TUE_PERIOD_ARCHIVE)[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Mon_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)
    constraint = instance.constraints[0]

    assert evaluate_constraint(instance, occurrences, constraint) == 1
