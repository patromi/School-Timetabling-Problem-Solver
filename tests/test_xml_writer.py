from pathlib import Path

import pytest
from src.model import (
    Solution,
    SolutionEvent,
    SolutionEventResource,
    SolutionGroup,
)
from src.parser import parse_archive, parse_solution_groups
from src.xml_writer import (
    extract_instance_archive,
    render_archive_with_solution_groups,
    render_solution_group,
)

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def test_round_trips_a_simple_solution_through_parse_and_render():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(
                event_ref="Event1",
                time_ref="Day_1",
                resources=[SolutionEventResource(role="RoomRT1", resource_ref="R1")],
            )
        ],
    )
    group = SolutionGroup(id="MyGroup", solutions=[solution])

    xml_text = render_archive_with_solution_groups(
        _load("ArtificialSudoku4x4.xml"), [group]
    )
    parsed_back = parse_solution_groups(xml_text)

    assert len(parsed_back) == 1
    assert parsed_back[0].id == "MyGroup"
    assert len(parsed_back[0].solutions) == 1
    parsed_solution = parsed_back[0].solutions[0]
    assert parsed_solution.instance_ref == instance.id
    assert len(parsed_solution.events) == 1
    event = parsed_solution.events[0]
    assert event.event_ref == "Event1"
    assert event.time_ref == "Day_1"
    assert len(event.resources) == 1
    assert event.resources[0].role == "RoomRT1"
    assert event.resources[0].resource_ref == "R1"


def test_round_trips_split_events_and_missing_time():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="Event1", time_ref="Day_1", duration=1),
            SolutionEvent(event_ref="Event1", time_ref=None, duration=1),
        ],
    )
    group = SolutionGroup(id="G", solutions=[solution])

    xml_text = render_archive_with_solution_groups(
        _load("ArtificialSudoku4x4.xml"), [group]
    )
    parsed_back = parse_solution_groups(xml_text)[0].solutions[0]

    assert len(parsed_back.events) == 2
    assert parsed_back.events[0].duration == 1
    assert parsed_back.events[0].time_ref == "Day_1"
    assert parsed_back.events[1].time_ref is None


def test_replaces_existing_solution_groups_rather_than_duplicating():
    # ArtificialSudoku4x4.xml already ships with one SolutionGroup baked in
    # -- rendering a new one must replace it, not append alongside it
    # (otherwise HSEval would report on both).
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    original_groups = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))
    assert len(original_groups) == 1  # sanity check on the fixture

    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="Event1", time_ref="Day_1")],
    )
    group = SolutionGroup(id="ReplacementGroup", solutions=[solution])

    xml_text = render_archive_with_solution_groups(
        _load("ArtificialSudoku4x4.xml"), [group]
    )
    parsed_back = parse_solution_groups(xml_text)

    assert len(parsed_back) == 1
    assert parsed_back[0].id == "ReplacementGroup"


def test_render_solution_group_includes_metadata_element_required_by_hseval():
    # HSEval's XML validator rejects a <SolutionGroup> missing <MetaData>
    # ("child <MetaData> missing or out of order") -- confirmed by
    # submitting a MetaData-less render to the real HSEval endpoint.
    solution = Solution(
        instance_ref="I1",
        events=[SolutionEvent(event_ref="Event1", time_ref="Day_1")],
    )
    group = SolutionGroup(id="G", solutions=[solution])

    xml_text = render_solution_group(group)

    assert "<MetaData>" in xml_text
    # MetaData must come before Solution, per HSEval's ordering requirement.
    assert xml_text.index("<MetaData>") < xml_text.index("<Solution ")
    # HSEval also rejects MetaData missing <Date> ("child <Date> missing or
    # out of order"), confirmed against the real endpoint.
    assert "<Date>" in xml_text


def test_extract_instance_archive_returns_a_self_contained_single_instance_archive():
    # A multi-instance archive (like the real XHSTT-2014.xml, 25 instances
    # in one file) needs to be sliceable down to just one Instance so a
    # solution can be written and submitted to HSEval without dragging the
    # other 24 instances along.
    multi_instance_xml = """<HighSchoolTimetableArchive Id="Archive">
  <Instances>
    <Instance Id="First">
      <MetaData><Name>First</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events><EventGroups></EventGroups></Events>
      <Constraints></Constraints>
    </Instance>
    <Instance Id="Second">
      <MetaData><Name>Second</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events><EventGroups></EventGroups></Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""

    extracted = extract_instance_archive(multi_instance_xml, "Second")
    instances = parse_archive(extracted)

    assert len(instances) == 1
    assert instances[0].id == "Second"
    assert instances[0].name == "Second"


def test_extract_instance_archive_raises_for_an_unknown_instance_id():
    minimal = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="Only">
      <MetaData><Name>Only</Name></MetaData>
      <Times><TimeGroups></TimeGroups></Times>
      <Resources><ResourceTypes></ResourceTypes><ResourceGroups></ResourceGroups></Resources>
      <Events><EventGroups></EventGroups></Events>
      <Constraints></Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""

    with pytest.raises(ValueError, match="Missing"):
        extract_instance_archive(minimal, "Missing")


def test_render_escapes_special_characters_in_ids():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="Event1", time_ref="Day_1")],
    )
    group = SolutionGroup(id='Weird&<>"Id', solutions=[solution])

    xml_text = render_archive_with_solution_groups(
        _load("ArtificialSudoku4x4.xml"), [group]
    )
    parsed_back = parse_solution_groups(xml_text)

    assert parsed_back[0].id == 'Weird&<>"Id'
