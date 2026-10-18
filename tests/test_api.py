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
