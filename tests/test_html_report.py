from src.evaluator_ref import resolve_occurrences
from src.html_report import (
    build_constraint_scores,
    build_days,
    build_resource_grid,
    render_timetable_page,
)
from src.model import Solution, SolutionEvent, SolutionEventResource
from src.parser import parse_archive

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
    for tag in ("table", "div", "ul", "li", "dd", "dl", "html", "body", "section"):
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


def test_render_timetable_page_shows_evaluation_section():
    instance = _instance_with_constraints()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E3", time_ref="Tue_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    html = render_timetable_page(instance, occurrences, infeasibility=2, objective=5)

    assert "Ocena rozwiązania" in html
    assert "Ograniczenia wymagane" in html
    assert "Ograniczenia preferowane" in html
    assert "suma = 2" in html
    assert "suma = 5" in html

    # AT1 (Fizyka, cost 2) is violated -> its row is visible by default.
    at1_pos = html.index("Fizyka musi miec czas", html.index("Ocena rozwiązania"))
    row1_start = html.rindex("<tr", 0, at1_pos)
    row1_tag = html[row1_start : html.index(">", row1_start) + 1]
    assert "row--bad" in row1_tag
    assert "hidden" not in row1_tag

    # AT2 (Chemia, cost 0) is satisfied -> its row starts hidden.
    at2_pos = html.index("Chemia musi miec czas", html.index("Ocena rozwiązania"))
    row2_start = html.rindex("<tr", 0, at2_pos)
    row2_tag = html[row2_start : html.index(">", row2_start) + 1]
    assert "row--ok" in row2_tag
    assert "hidden" in row2_tag


def test_render_timetable_page_evaluation_section_handles_no_constraints():
    # The base fixture instance (_instance(), from the top of this file)
    # has an empty <Constraints/> block -- both groups must fall back to
    # their "brak ograniczen" message instead of rendering empty/broken
    # tables.
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Mon_1")],
    )
    occurrences = resolve_occurrences(instance, solution)

    html = render_timetable_page(instance, occurrences, infeasibility=0, objective=0)

    assert "Brak ograniczeń wymaganych w tej instancji." in html
    assert "Brak ograniczeń preferowanych w tej instancji." in html


def test_violation_event_refs_covers_resource_group_and_pair_constraints():
    from src.html_report.evaluation import violation_event_refs
    from src.model import AppliesTo, Constraint, EventPair, Group

    base = _instance()
    # Add an EventGroup containing E1 and E3 for group-based constraints
    base.event_groups.append(Group(id="EG1", name="EG1", kind="EventGroup"))
    base.events[0].group_refs.append("EG1")
    base.events[2].group_refs.append("EG1")
    # Also test an unassigned Teacher slot on E3 copy or temporary event
    occ = resolve_occurrences(
        base,
        Solution(
            instance_ref=base.id,
            events=[
                SolutionEvent(event_ref="E1", time_ref="Mon_1"),
                SolutionEvent(event_ref="E2", time_ref="Tue_1", duration=2),
                # Gap on Mon between Mon_1 and Mon_3 for C1 & T1; also 2 events on Mon for EG1
                SolutionEvent(event_ref="E3", time_ref="Mon_3"),
            ],
        ),
    )

    # LimitIdleTimesConstraint: C1 has an idle gap on Mon (Mon_2), not on Tue
    c_idle = Constraint(
        type="LimitIdleTimesConstraint",
        id="C_IDLE",
        name="No gaps",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(resources=["C1"]),
        params={"TimeGroups": [{"reference": "gr_Mon"}, {"reference": "gr_Tue"}], "Minimum": 0, "Maximum": 0},
    )
    assert set(violation_event_refs(base, occ, c_idle)) == {"E1", "E3"}

    # LimitBusyTimesConstraint: C1 has 2 events on Mon and 2 hours on Tue; max 1 -> both days violate
    c_busy = Constraint(
        type="LimitBusyTimesConstraint",
        id="C_BUSY",
        name="Max 1",
        required=False,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(resources=["C1"]),
        params={"TimeGroups": [{"reference": "gr_Mon"}], "Minimum": 0, "Maximum": 1},
    )
    assert set(violation_event_refs(base, occ, c_busy)) == {"E1", "E3"}

    # ClusterBusyTimesConstraint: C1 active on 2 days, max 1 day -> all events on those days
    c_cluster = Constraint(
        type="ClusterBusyTimesConstraint",
        id="C_CLUSTER",
        name="Max 1 day",
        required=False,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(resources=["C1"]),
        params={"TimeGroups": [{"reference": "gr_Mon"}, {"reference": "gr_Tue"}], "Minimum": 0, "Maximum": 1},
    )
    assert set(violation_event_refs(base, occ, c_cluster)) == {"E1", "E2", "E3"}

    # LimitWorkloadConstraint: T1 teaches E1(1h) + E3(1h) = 2h; max 1h
    c_work = Constraint(
        type="LimitWorkloadConstraint",
        id="C_WORK",
        name="Max 1h",
        required=False,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(resources=["T1", "T2"]),
        params={"Minimum": 0, "Maximum": 1},
    )
    # T1 (E1, E3) violates (2 > 1) and T2 (E2, duration 2 > 1) violates
    assert set(violation_event_refs(base, occ, c_work)) == {"E1", "E2", "E3"}

    # SpreadEventsConstraint: EG1 has 2 events starting in gr_Mon (max 1)
    c_spread = Constraint(
        type="SpreadEventsConstraint",
        id="C_SPREAD",
        name="Spread",
        required=False,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(event_groups=["EG1"]),
        params={"TimeGroups": [{"reference": "gr_Mon", "Minimum": 0, "Maximum": 1}]},
    )
    assert set(violation_event_refs(base, occ, c_spread)) == {"E1", "E3"}

    # LinkEventsConstraint: E1 at Mon_1, E3 at Mon_3 -> misaligned
    c_link = Constraint(
        type="LinkEventsConstraint",
        id="C_LINK",
        name="Link",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(event_groups=["EG1"]),
    )
    assert set(violation_event_refs(base, occ, c_link)) == {"E1", "E3"}

    # OrderEventsConstraint: E3 (Mon_3) before E1 (Mon_1) -> violated
    c_order = Constraint(
        type="OrderEventsConstraint",
        id="C_ORDER",
        name="Order",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(event_pairs=[EventPair(first_event="E3", second_event="E1", min_separation=0)]),
    )
    assert set(violation_event_refs(base, occ, c_order)) == {"E3", "E1"}

    # AvoidSplitAssignmentsConstraint & AssignResourceConstraint require unpreassigned resource slots
    from dataclasses import replace as dc_replace

    for ev in (base.events[0], base.events[2]):
        ev.resources = [
            dc_replace(er, resource_ref=None) if er.role == "Teacher" else er
            for er in ev.resources
        ]
    occ_split = resolve_occurrences(
        base,
        Solution(
            instance_ref=base.id,
            events=[
                SolutionEvent(event_ref="E1", time_ref="Mon_1", resources=[SolutionEventResource(role="Teacher", resource_ref="T1")]),
                SolutionEvent(event_ref="E3", time_ref="Mon_3", resources=[SolutionEventResource(role="Teacher", resource_ref="T2")]),
            ],
        ),
    )
    c_split_assign = Constraint(
        type="AvoidSplitAssignmentsConstraint",
        id="C_ASA",
        name="One teacher",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(event_groups=["EG1"]),
        params={"Role": "Teacher"},
    )
    assert set(violation_event_refs(base, occ_split, c_split_assign)) == {"E1", "E3"}

    occ_unassigned = resolve_occurrences(
        base,
        Solution(
            instance_ref=base.id,
            events=[
                SolutionEvent(event_ref="E1", time_ref="Mon_1", resources=[]),
                SolutionEvent(event_ref="E3", time_ref="Mon_3", resources=[SolutionEventResource(role="Teacher", resource_ref="T2")]),
            ],
        ),
    )
    c_assign_res = Constraint(
        type="AssignResourceConstraint",
        id="C_AR",
        name="Assign teacher",
        required=True,
        weight=1,
        cost_function="Linear",
        applies_to=AppliesTo(events=["E1", "E3"]),
        params={"Role": "Teacher"},
    )
    assert violation_event_refs(base, occ_unassigned, c_assign_res) == ["E1"]
