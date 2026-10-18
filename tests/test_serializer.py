from pathlib import Path

from src.parser import parse_archive
from src.serializer import dict_to_instance, instance_to_dict

FIXTURES = Path(__file__).parent / "fixtures"


def test_instance_dict_round_trip():
    xml_text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    original = parse_archive(xml_text)[0]

    # Convert to dict
    d = instance_to_dict(original)
    assert d["id"] == original.id
    assert d["name"] == original.name
    assert len(d["times"]) == len(original.times)
    assert len(d["resources"]) == len(original.resources)
    assert len(d["events"]) == len(original.events)
    assert len(d["constraints"]) == len(original.constraints)

    # Convert back to Instance
    reconstructed = dict_to_instance(d)
    assert reconstructed.id == original.id
    assert reconstructed.name == original.name
    assert len(reconstructed.times) == len(original.times)
    for t1, t2 in zip(original.times, reconstructed.times):
        assert t1.id == t2.id
        assert t1.name == t2.name
        assert t1.group_refs == t2.group_refs

    assert len(reconstructed.resources) == len(original.resources)
    for r1, r2 in zip(original.resources, reconstructed.resources):
        assert r1.id == r2.id
        assert r1.name == r2.name
        assert r1.resource_type_ref == r2.resource_type_ref

    assert len(reconstructed.events) == len(original.events)
    for e1, e2 in zip(original.events, reconstructed.events):
        assert e1.id == e2.id
        assert e1.duration == e2.duration
        assert len(e1.resources) == len(e2.resources)

    assert len(reconstructed.constraints) == len(original.constraints)
    for c1, c2 in zip(original.constraints, reconstructed.constraints):
        assert c1.id == c2.id
        assert c1.type == c2.type
        assert c1.required == c2.required
        assert c1.weight == c2.weight
