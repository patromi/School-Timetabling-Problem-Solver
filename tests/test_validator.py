"""Unit tests for the KHE / HSEval validator."""

from pathlib import Path

import pytest
from src.evaluator_ref import evaluate_cost_components
from src.parser import parse_archive, parse_solution_groups
from src.validator import KHEValidator


@pytest.fixture
def br_sa_00_solution_path() -> Path:
    p = Path(__file__).resolve().parent.parent / "output" / "BR-SA-00_solution.xml"
    if not p.is_file():
        pytest.skip("Sample solution file output/BR-SA-00_solution.xml not present")
    return p


def test_validator_file(br_sa_00_solution_path: Path) -> None:
    validator = KHEValidator()
    result = validator.validate_file(br_sa_00_solution_path)

    xml_content = br_sa_00_solution_path.read_text(encoding="utf-8")
    instances = parse_archive(xml_content)
    groups = parse_solution_groups(xml_content)
    expected_infeas, expected_obj = evaluate_cost_components(
        instances[0], groups[0].solutions[0]
    )

    assert result.is_valid is True
    assert result.instance_id == "BR-SA-00"
    assert result.infeasibility == expected_infeas
    assert result.objective == expected_obj
    assert result.total_cost_lex == (expected_infeas, expected_obj)


def test_validator_xml_string(br_sa_00_solution_path: Path) -> None:
    validator = KHEValidator()
    xml_content = br_sa_00_solution_path.read_text(encoding="utf-8")
    result = validator.validate_xml_string(xml_content)

    instances = parse_archive(xml_content)
    groups = parse_solution_groups(xml_content)
    expected_infeas, expected_obj = evaluate_cost_components(
        instances[0], groups[0].solutions[0]
    )

    assert result.is_valid is True
    assert result.infeasibility == expected_infeas
    assert result.objective == expected_obj


def test_validator_matches_evaluator_ref(br_sa_00_solution_path: Path) -> None:
    """Verifies that our Python evaluator produces EXACTLY the same cost
    as official HSEval (Definition of Done for Stage 2)."""
    xml_content = br_sa_00_solution_path.read_text(encoding="utf-8")
    instances = parse_archive(xml_content)
    groups = parse_solution_groups(xml_content)

    assert len(instances) == 1
    assert len(groups) >= 1
    instance = instances[0]
    solution = groups[0].solutions[0]

    # HSEval result
    validator = KHEValidator()
    hseval_result = validator.validate_solution(xml_content, solution)

    # Reference Python evaluator result
    py_infeasibility, py_objective = evaluate_cost_components(instance, solution)

    assert hseval_result.is_valid is True
    assert hseval_result.infeasibility == py_infeasibility
    assert hseval_result.objective == py_objective


def test_validator_nonexistent_file() -> None:
    validator = KHEValidator()
    result = validator.validate_file("nonexistent_solution_123.xml")
    assert result.is_valid is False
    assert result.infeasibility == -1
    assert result.error_message is not None
