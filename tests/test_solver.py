"""Tests for the TimetableSolver class."""

from solver.main import TimetableSolver


def test_solver_initialization() -> None:
    """Test that TimetableSolver initializes correctly."""
    solver = TimetableSolver("data/raw/instance.xml")
    assert solver.instance_path == "data/raw/instance.xml"
    assert solver.parsed_data is None


def test_solver_load_data() -> None:
    """Test that TimetableSolver loads data correctly."""
    solver = TimetableSolver("data/raw/instance.xml")
    data = solver.load_data()
    assert data["status"] == "loaded"
    assert data["source"] == "data/raw/instance.xml"
    assert solver.parsed_data == data


def test_solver_solve() -> None:
    """Test that TimetableSolver solves the instance correctly."""
    solver = TimetableSolver("data/raw/instance.xml")
    results = solver.solve()
    assert len(results) == 1
    assert results[0] == "Solution found successfully"
