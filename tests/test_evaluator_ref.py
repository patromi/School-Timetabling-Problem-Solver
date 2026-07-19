from pathlib import Path

import pytest

from xhstt_core.evaluator_ref import (
    apply_cost_function,
    evaluate_constraint,
    resolve_occurrences,
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
    assert event1.resource_assignments == {
        "Class": "C1",
        "Teacher": "T1",
        "RoomRT1": "R1",
    }


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
      <Times><TimeGroups></TimeGroups></Times>
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
    event1.resource_assignments["RoomRT1"] = None

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
    event1.resource_assignments["RoomRT1"] = "R2"

    assert evaluate_constraint(instance, occurrences, constraint) == 1


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
