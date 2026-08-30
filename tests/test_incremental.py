import random
from pathlib import Path

import pytest
from xhstt_core.construct import build_initial
from xhstt_core.cost import evaluate_cost
from xhstt_core.evaluator_ref import evaluate_constraint_costs
from xhstt_core.heuristics import MANUAL_HEURISTICS
from xhstt_core.incremental import IncrementalEvaluator, StructuralChangeError
from xhstt_core.model import Instance, Solution, SolutionEvent
from xhstt_core.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _instance(name: str) -> Instance:
    return parse_archive(_load(name))[0]


@pytest.mark.parametrize("fixture", ["ArtificialSudoku4x4.xml", "BrazilInstance1.xml"])
def test_initial_state_matches_the_reference_evaluator(fixture: str) -> None:
    instance = _instance(fixture)
    solution = build_initial(instance, random.Random(0))

    evaluator = IncrementalEvaluator(instance, solution)

    assert evaluator.cost == evaluate_cost(instance, solution)
    assert evaluator.constraint_costs() == evaluate_constraint_costs(instance, solution)


@pytest.mark.parametrize("fixture", ["ArtificialSudoku4x4.xml", "BrazilInstance1.xml"])
def test_probe_matches_full_evaluation_and_leaves_the_state_untouched(
    fixture: str,
) -> None:
    instance = _instance(fixture)
    rng = random.Random(1)
    solution = build_initial(instance, rng)
    evaluator = IncrementalEvaluator(instance, solution)
    before = evaluator.cost
    before_vector = evaluator.constraint_costs()

    for heuristic in MANUAL_HEURISTICS:
        try:
            candidate = heuristic.apply(solution, instance, rng)
        except ValueError:
            continue
        assert evaluator.probe(candidate) == evaluate_cost(instance, candidate)
        assert evaluator.cost == before
        assert evaluator.constraint_costs() == before_vector
        assert evaluator.solution is solution


def test_commit_advances_the_state_and_keeps_the_per_constraint_vector_exact() -> None:
    instance = _instance("BrazilInstance1.xml")
    rng = random.Random(2)
    solution = build_initial(instance, rng)
    evaluator = IncrementalEvaluator(instance, solution)

    applied = 0
    while applied < 50:
        heuristic = rng.choice(MANUAL_HEURISTICS)
        try:
            candidate = heuristic.apply(evaluator.solution, instance, rng)
        except ValueError:
            continue
        cost = evaluator.commit(candidate)
        assert cost == evaluate_cost(instance, candidate)
        assert evaluator.constraint_costs() == evaluate_constraint_costs(
            instance, candidate
        )
        applied += 1


def test_rejected_candidates_never_leak_into_the_accepted_state() -> None:
    # The reject path is what LAHC exercises most; a leaked aggregate would
    # show up as drift only much later, so assert the full vector each time.
    instance = _instance("BrazilInstance1.xml")
    rng = random.Random(3)
    solution = build_initial(instance, rng)
    evaluator = IncrementalEvaluator(instance, solution)
    expected_cost = evaluator.cost
    expected_vector = evaluator.constraint_costs()

    rejected = 0
    while rejected < 100:
        heuristic = rng.choice(MANUAL_HEURISTICS)
        try:
            candidate = heuristic.apply(solution, instance, rng)
        except ValueError:
            continue
        transaction = evaluator.evaluate(candidate)
        evaluator.rollback(transaction)
        assert evaluator.cost == expected_cost
        assert evaluator.constraint_costs() == expected_vector
        rejected += 1


def test_a_finished_transaction_cannot_be_rolled_back_twice() -> None:
    instance = _instance("ArtificialSudoku4x4.xml")
    rng = random.Random(4)
    solution = build_initial(instance, rng)
    evaluator = IncrementalEvaluator(instance, solution)
    heuristic = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    transaction = evaluator.evaluate(heuristic.apply(solution, instance, rng))
    evaluator.rollback(transaction)

    with pytest.raises(ValueError, match="already finished"):
        evaluator.rollback(transaction)


def test_structural_change_is_rejected_by_evaluate_and_absorbed_by_commit() -> None:
    instance = _instance("ArtificialSudoku4x4.xml")
    solution = build_initial(instance, random.Random(5))
    evaluator = IncrementalEvaluator(instance, solution)
    resplit = Solution(
        instance_ref=solution.instance_ref,
        events=[
            *solution.events,
            SolutionEvent(event_ref=solution.events[0].event_ref),
        ],
    )

    with pytest.raises(StructuralChangeError):
        evaluator.evaluate(resplit)

    assert evaluator.cost == evaluate_cost(instance, solution)
    assert evaluator.commit(resplit) == evaluate_cost(instance, resplit)


@pytest.mark.slow
def test_incremental_cost_matches_full_evaluation_over_10000_chained_moves() -> None:
    # The Etap 3 DoD invariant, now exercising the stateful path: every
    # candidate is scored incrementally, half of them are rolled back, and
    # the accepted ones chain -- so an error in either the apply or the undo
    # direction accumulates and gets caught.
    instance = _instance("BrazilInstance1.xml")
    rng = random.Random(0)
    evaluator = IncrementalEvaluator(instance, build_initial(instance, rng))

    applied = 0
    while applied < 10_000:
        heuristic = rng.choice(MANUAL_HEURISTICS)
        try:
            candidate = heuristic.apply(evaluator.solution, instance, rng)
        except ValueError:
            continue
        transaction = evaluator.evaluate(candidate)
        assert transaction.cost == evaluate_cost(instance, candidate)
        if applied % 2:
            evaluator.rollback(transaction)
        else:
            evaluator.accept(transaction)
        if applied % 500 == 0:
            assert evaluator.constraint_costs() == evaluate_constraint_costs(
                instance, evaluator.solution
            )
        applied += 1

    assert evaluator.cost == evaluate_cost(instance, evaluator.solution)
