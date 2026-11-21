from pathlib import Path

from src.parser import parse_archive
from src.xml_writer import render_instance_xml

FIXTURES = Path(__file__).parent / "fixtures"


def test_instance_xml_render_and_parse():
    xml_text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    original = parse_archive(xml_text)[0]

    rendered_xml = render_instance_xml(original)
    assert "<HighSchoolTimetableArchive" in rendered_xml
    assert f'<Instance Id="{original.id}"' in rendered_xml or f"<Instance Id='{original.id}'" in rendered_xml

    # Parse back
    reparsed = parse_archive(rendered_xml)[0]
    assert reparsed.id == original.id
    assert reparsed.name == original.name
    assert len(reparsed.times) == len(original.times)
    assert len(reparsed.resources) == len(original.resources)
    assert len(reparsed.events) == len(original.events)
    assert len(reparsed.constraints) == len(original.constraints)
