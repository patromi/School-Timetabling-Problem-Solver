import random
from pathlib import Path

import pytest
import xhstt_core.delta as delta
from xhstt_core.construct import build_initial
from xhstt_core.cost import Cost, evaluate_cost
from xhstt_core.delta import delta_cost
from xhstt_core.evaluator_ref import evaluate_constraint
from xhstt_core.heuristics import MANUAL_HEURISTICS
from xhstt_core.model import (
    AppliesTo,
    Constraint,
    Event,
    Group,
    Instance,
    Solution,
    SolutionEvent,
    Time,
)
from xhstt_core.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def _independent_prefer_times_instance(n: int) -> Instance:
    """n events, each duration 1, each with its OWN PreferTimesConstraint
    scoped to just that event (applies_to.events=[event_id]) preferring P1
    over P2. The constraints are fully independent of each other -- moving
    one event must never change another constraint's contribution, which is
    exactly what these tests check via the touched-constraint call count."""
    return Instance(
        id="I1",
        name="Test",
        time_groups=[Group(id="gr_Preferred", name="Preferred", kind="TimeGroup")],
        times=[
            Time(id="P1", name="P1", group_refs=["gr_Preferred"]),
            Time(id="P2", name="P2"),
        ],
        events=[Event(id=f"E{i}", name=f"E{i}", duration=1) for i in range(n)],
        constraints=[
            Constraint(
                type="PreferTimesConstraint",
                id=f"PT{i}",
                name=f"PreferTimes{i}",
                required=False,
                weight=10,
                cost_function="Linear",
                applies_to=AppliesTo(events=[f"E{i}"]),
                params={"TimeGroups": [{"reference": "gr_Preferred"}]},
            )
            for i in range(n)
        ],
    )


def _all_at_p1(n: int) -> Solution:
    return Solution(
        instance_ref="I1",
        events=[SolutionEvent(event_ref=f"E{i}", time_ref="P1") for i in range(n)],
    )


def test_delta_cost_returns_old_cost_unchanged_when_nothing_changed(
    monkeypatch,  # noqa: ANN001
) -> None:
    instance = _independent_prefer_times_instance(3)
    solution = _all_at_p1(3)
    cost = evaluate_cost(instance, solution)
    calls: list[int] = []
    real = evaluate_constraint
    monkeypatch.setattr(
        delta,
        "evaluate_constraint",
        lambda *a, **kw: (calls.append(1), real(*a, **kw))[1],
    )

    result = delta_cost(instance, solution, cost, solution)

    assert result == cost
    assert calls == []


def test_delta_cost_matches_full_evaluation_and_skips_unrelated_constraints(
    monkeypatch,  # noqa: ANN001
) -> None:
    n = 5
    instance = _independent_prefer_times_instance(n)
    old_solution = _all_at_p1(n)
    old_cost = evaluate_cost(instance, old_solution)
    new_events = list(old_solution.events)
    new_events[0] = SolutionEvent(event_ref="E0", time_ref="P2")
    new_solution = Solution(instance_ref="I1", events=new_events)

    calls: list[int] = []
    real = evaluate_constraint
    monkeypatch.setattr(
        delta,
        "evaluate_constraint",
        lambda *a, **kw: (calls.append(1), real(*a, **kw))[1],
    )

    result = delta_cost(instance, old_solution, old_cost, new_solution)

    assert result == evaluate_cost(instance, new_solution)
    assert result == Cost(infeasibility=0, objective=10)
    # Only PT0 (the constraint scoped to the moved event E0) is touched --
    # one call for its old contribution, one for its new contribution.
    # PT1..PT4 (scoped to untouched E1..E4) must never be re-evaluated.
    assert len(calls) == 2


def test_delta_cost_raises_when_solutions_have_a_different_occurrence_count() -> None:
    instance = Instance(
        id="I1",
        name="Test",
        times=[Time(id="P1", name="P1")],
        events=[Event(id="E1", name="E1", duration=1)],
    )
    old_solution = Solution(
        instance_ref="I1",
        events=[SolutionEvent(event_ref="E1", time_ref="P1", duration=1)],
    )
    new_solution = Solution(
        instance_ref="I1",
        events=[
            SolutionEvent(event_ref="E1", time_ref="P1", duration=1),
            SolutionEvent(event_ref="E1", time_ref="P1", duration=1),
        ],
    )
    old_cost = evaluate_cost(instance, old_solution)

    with pytest.raises(ValueError, match="different number of occurrences"):
        delta_cost(instance, old_solution, old_cost, new_solution)


def test_delta_cost_matches_full_evaluation_on_a_real_instance_after_one_lahc_style_move() -> (  # noqa: E501
    None
):
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    rng = random.Random(0)
    old_solution = build_initial(instance, rng)
    old_cost = evaluate_cost(instance, old_solution)
    heuristic = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    new_solution = heuristic.apply(old_solution, instance, rng)

    result = delta_cost(instance, old_solution, old_cost, new_solution)

    assert result == evaluate_cost(instance, new_solution)
