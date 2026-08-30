import random
from pathlib import Path

import pytest
from xhstt_core.construct import build_initial
from xhstt_core.cost import Cost, evaluate_cost
from xhstt_core.delta import delta_cost
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
    one event must never change another constraint's contribution."""
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


def test_delta_cost_returns_the_same_cost_when_nothing_changed() -> None:
    instance = _independent_prefer_times_instance(3)
    solution = _all_at_p1(3)
    cost = evaluate_cost(instance, solution)

    assert delta_cost(instance, solution, cost, solution) == cost


def test_delta_cost_matches_full_evaluation_after_a_single_move() -> None:
    n = 5
    instance = _independent_prefer_times_instance(n)
    old_solution = _all_at_p1(n)
    old_cost = evaluate_cost(instance, old_solution)
    new_events = list(old_solution.events)
    new_events[0] = SolutionEvent(event_ref="E0", time_ref="P2")
    new_solution = Solution(instance_ref="I1", events=new_events)

    result = delta_cost(instance, old_solution, old_cost, new_solution)

    assert result == evaluate_cost(instance, new_solution)
    assert result == Cost(infeasibility=0, objective=10)


def test_delta_cost_ignores_a_stale_old_cost_rather_than_trusting_it() -> None:
    # The old implementation added its per-constraint changes onto whatever
    # the caller passed in, so a stale old_cost silently produced a wrong
    # answer. The engine derives it instead.
    instance = _independent_prefer_times_instance(3)
    solution = _all_at_p1(3)

    result = delta_cost(instance, solution, Cost(999, 999), solution)

    assert result == evaluate_cost(instance, solution)


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


def test_delta_cost_matches_full_evaluation_on_a_real_instance() -> None:
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    rng = random.Random(0)
    old_solution = build_initial(instance, rng)
    old_cost = evaluate_cost(instance, old_solution)

    for heuristic in MANUAL_HEURISTICS:
        try:
            new_solution = heuristic.apply(old_solution, instance, rng)
        except ValueError:
            continue
        result = delta_cost(instance, old_solution, old_cost, new_solution)
        assert result == evaluate_cost(instance, new_solution)
