from xhstt_core.evaluator_ref import resolve_occurrences
from xhstt_core.html_report import (
    build_constraint_scores,
    build_days,
    build_resource_grid,
    render_timetable_page,
)
from xhstt_core.model import Solution, SolutionEvent, SolutionEventResource
from xhstt_core.parser import parse_archive

ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test School</Name></MetaData>
      <Times>
        <TimeGroups>
          <Day Id="gr_Mon"><Name>Poniedzialek</Name></Day>
          <Day Id="gr_Tue"><Name>Wtorek</Name></Day>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Mon_3"><Name>Mon_3</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><Day Reference="gr_Tue"/></Time>
        <Time Id="Tue_2"><Name>Tue_2</Name><Day Reference="gr_Tue"/></Time>
      </Times>
      <Resources>
        <ResourceTypes>
          <ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType>
          <ResourceType Id="Class"><Name>Class</Name></ResourceType>
          <ResourceType Id="Room"><Name>Room</Name></ResourceType>
        </ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="C1"><Name>7A</Name><ResourceType Reference="Class"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="T1"><Name>Kowalski</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="T2"><Name>Nowak</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="R1"><Name>Sala 12</Name><ResourceType Reference="Room"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>Matematyka</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
            <Resource Reference="R1"><Role>Room</Role></Resource>
          </Resources>
        </Event>
        <Event Id="E2">
          <Name>Fizyka</Name>
          <Duration>2</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T2"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
        <Event Id="E3">
          <Name>Chemia</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""

CONSTRAINTS_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test School</Name></MetaData>
      <Times>
        <TimeGroups>
          <Day Id="gr_Mon"><Name>Poniedzialek</Name></Day>
          <Day Id="gr_Tue"><Name>Wtorek</Name></Day>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><Day Reference="gr_Tue"/></Time>
        <Time Id="Tue_2"><Name>Tue_2</Name><Day Reference="gr_Tue"/></Time>
      </Times>
      <Resources>
        <ResourceTypes>
          <ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType>
          <ResourceType Id="Class"><Name>Class</Name></ResourceType>
        </ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="C1"><Name>7A</Name><ResourceType Reference="Class"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="T1"><Name>Kowalski</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>Matematyka</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
        <Event Id="E2">
          <Name>Fizyka musi miec czas</Name>
          <Duration>2</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
        <Event Id="E3">
          <Name>Chemia musi miec czas</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints>
        <AssignTimeConstraint Id="AT1">
          <Name>Fizyka musi miec czas</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Events><Event Reference="E2"/></Events></AppliesTo>
        </AssignTimeConstraint>
        <AssignTimeConstraint Id="AT2">
          <Name>Chemia musi miec czas</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Events><Event Reference="E3"/></Events></AppliesTo>
        </AssignTimeConstraint>
        <PreferTimesConstraint Id="PT1">
          <Name>Matematyka wolimy we wtorek</Name>
          <Required>false</Required>
          <Weight>5</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Events><Event Reference="E1"/></Events></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_Tue"/></TimeGroups>
        </PreferTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def _instance():
    return parse_archive(ARCHIVE)[0]


def _instance_with_constraints():
    return parse_archive(CONSTRAINTS_ARCHIVE)[0]


def test_build_days_returns_day_groups_in_file_order_with_their_times():
    instance = _instance()

    days = build_days(instance)

    assert [d.id for d in days] == ["gr_Mon", "gr_Tue"]
    assert days[0].name == "Poniedzialek"
    assert [t.id for t in days[0].periods] == ["Mon_1", "Mon_2", "Mon_3"]
    assert [t.id for t in days[1].periods] == ["Tue_1", "Tue_2"]


def test_build_resource_grid_places_event_at_correct_day_and_period():
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Mon_1")],
    )
    occurrences = resolve_occurrences(instance, solution)

    grid = build_resource_grid(instance, occurrences, "C1")

    cells = grid[("gr_Mon", 0)]
    assert len(cells) == 1
    assert cells[0].event_name == "Matematyka"
    assert cells[0].duration == 1
    assert ("Teacher", "Kowalski") in cells[0].other_resources
    assert ("Room", "Sala 12") in cells[0].other_resources
    # The viewed resource (C1/7A) itself must not appear in its own "other
    # resources" list -- that would be redundant on its own grid.
    assert all(name != "7A" for _, name in cells[0].other_resources)


def test_build_resource_grid_records_duration_for_a_multi_period_lesson():
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E2", time_ref="Mon_2", duration=2)],
    )
    occurrences = resolve_occurrences(instance, solution)

    grid = build_resource_grid(instance, occurrences, "C1")

    cells = grid[("gr_Mon", 1)]
    assert len(cells) == 1
    assert cells[0].event_name == "Fizyka"
    assert cells[0].duration == 2
    # The second period the lesson spans must not get its own separate
    # entry -- the renderer uses `duration` to span rows instead.
    assert ("gr_Mon", 2) not in grid


def test_build_resource_grid_collects_multiple_events_at_a_clash():
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E3", time_ref="Mon_1"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    grid = build_resource_grid(instance, occurrences, "C1")

    cells = grid[("gr_Mon", 0)]
    assert {c.event_name for c in cells} == {"Matematyka", "Chemia"}


def test_build_resource_grid_only_includes_occurrences_for_the_given_resource():
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),  # T1 teaches
            SolutionEvent(event_ref="E2", time_ref="Tue_1", duration=1),  # T2 teaches
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    grid = build_resource_grid(instance, occurrences, "T1")

    assert list(grid.keys()) == [("gr_Mon", 0)]


def test_render_timetable_page_includes_lesson_and_resource_names():
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E2", time_ref="Tue_1", duration=1),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    html = render_timetable_page(instance, occurrences, infeasibility=0, objective=3)

    assert html.startswith("<!doctype html>")
    assert "Test School" in html
    assert "Matematyka" in html
    assert "Kowalski" in html
    assert "7A" in html  # resource chip for the class
    # Every open table/div/ul/li tag has a matching close (rough balance
    # check -- not a full HTML validator, just a smoke test against
    # unclosed elements from the string-building above). An opening tag is
    # "<table" or "<table ...>", never "<table..." mid-word, so matching
    # "<table" followed by a space or ">" distinguishes it from other tags
    # that merely start with the same prefix (there are none here, but be
    # precise regardless).
    for tag in ("table", "div", "ul", "li", "dd", "dl", "html", "body"):
        opens = html.count(f"<{tag}>") + html.count(f"<{tag} ")
        closes = html.count(f"</{tag}>")
        assert opens == closes, f"<{tag}>: {opens} opens vs {closes} closes"


def test_build_constraint_scores_computes_cost_per_constraint():
    instance = _instance_with_constraints()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E3", time_ref="Tue_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    scores = build_constraint_scores(instance, occurrences)

    by_id = {s.id: s for s in scores}
    assert len(scores) == 3
    # E2 never gets a time in the solution -> AssignTimeConstraint
    # deviation is its full duration (2), weight 1 -> cost 2.
    assert by_id["AT1"].cost == 2
    assert by_id["AT1"].required is True
    assert by_id["AT1"].type == "AssignTimeConstraint"
    assert by_id["AT1"].weight == 1
    assert by_id["AT1"].cost_function == "Linear"
    # E3 gets Tue_2 -> fully assigned -> deviation 0.
    assert by_id["AT2"].cost == 0
    # E1 gets Mon_1, preferred group is gr_Tue -> deviation = duration 1,
    # weight 5 -> cost 5.
    assert by_id["PT1"].cost == 5
    assert by_id["PT1"].required is False
