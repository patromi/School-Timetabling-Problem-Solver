from fastapi.testclient import TestClient
from src.api import app

client = TestClient(app)


def test_list_instances():
    response = client.get("/api/instances")
    assert response.status_code == 200
    data = response.json()
    assert isinstance(data, list)
    assert len(data) > 0
    # Check that known benchmark instances are present
    ids = [i["id"] for i in data]
    assert any("AU" in id_ or "BR" in id_ or "Sudoku" in id_ or "Artificial" in id_ for id_ in ids)


def test_get_instance_details():
    response = client.get("/api/instances")
    first_id = response.json()[0]["id"]

    detail_res = client.get(f"/api/instances/{first_id}")
    assert detail_res.status_code == 200
    inst_data = detail_res.json()
    assert inst_data["id"] == first_id
    assert "times" in inst_data
    assert "resources" in inst_data
    assert "events" in inst_data
    assert "constraints" in inst_data


def test_solver_status_lifecycle():
    status_res = client.get("/api/solver/status")
    assert status_res.status_code == 200
    status_data = status_res.json()
    assert status_data["status"] in ("idle", "running", "completed", "stopped")


def test_build_timetable_payload_includes_constraint_type():
    from src.api import build_timetable_payload
    from src.model import Solution, SolutionEvent
    from tests.test_html_report import _instance_with_constraints

    instance = _instance_with_constraints()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E3", time_ref="Tue_2"),
        ],
    )
    payload = build_timetable_payload(instance, solution)
    scores_by_id = {s["id"]: s for s in payload["scores"]}
    assert scores_by_id["AT1"]["type"] == "AssignTimeConstraint"
    assert scores_by_id["PT1"]["type"] == "PreferTimesConstraint"
    assert "E1" in payload["violation_map"]["PT1"]
