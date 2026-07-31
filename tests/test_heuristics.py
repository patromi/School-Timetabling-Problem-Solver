import random
from pathlib import Path

import pytest

from xhstt_core.construct import build_initial
from xhstt_core.evaluator_ref import total_cost
from xhstt_core.heuristics import MANUAL_HEURISTICS, move_best
from xhstt_core.model import Solution
from xhstt_core.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _sudoku_solution() -> tuple:
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = build_initial(instance, random.Random(0))
    return instance, solution


def _snapshot(solution: Solution) -> list:
    return [
        (
            se.event_ref,
            se.time_ref,
            se.duration,
            tuple((r.role, r.resource_ref) for r in se.resources),
        )
        for se in solution.events
    ]


@pytest.mark.parametrize("heuristic", MANUAL_HEURISTICS, ids=lambda h: h.id)
def test_heuristic_preserves_events_and_does_not_mutate_input(heuristic) -> None:
    instance, solution = _sudoku_solution()
    before = _snapshot(solution)

    new_solution = None
    for seed in range(20):
        try:
            new_solution = heuristic.apply(solution, instance, random.Random(seed))
            break
        except ValueError:
            continue
    assert new_solution is not None, (
        f"{heuristic.id}: no seed in range(20) produced an applicable move"
    )

    assert _snapshot(solution) == before, f"{heuristic.id} mutated its input solution"
    assert isinstance(new_solution, Solution), f"{heuristic.id} did not return a Solution"
    assert sorted(se.event_ref for se in new_solution.events) == sorted(
        se.event_ref for se in solution.events
    ), f"{heuristic.id} lost or duplicated an event"


def test_move_best_never_increases_total_cost() -> None:
    instance, solution = _sudoku_solution()
    before = total_cost(instance, solution)

    ran_at_least_once = False
    for seed in range(20):
        new_solution = move_best(solution, instance, random.Random(seed))
        ran_at_least_once = True
        after = total_cost(instance, new_solution)
        assert after <= before, f"seed={seed}: move_best increased total_cost ({before} -> {after})"
    assert ran_at_least_once
