from pathlib import Path

from xhstt_core.cost import (
    AdaGenSchedule,
    Cost,
    evaluate_cost,
    min_working_days_cost,
    resources_of_type,
)
from xhstt_core.evaluator_ref import total_cost
from xhstt_core.model import (
    AppliesTo,
    Constraint,
    Event,
    EventResource,
    Group,
    Instance,
    Resource,
    ResourceType,
    Solution,
    SolutionEvent,
    Time,
)
from xhstt_core.parser import parse_archive, parse_solution_groups

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def test_evaluate_cost_is_zero_on_a_fully_feasible_reference_solution():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    solution = parse_solution_groups(_load("ArtificialSudoku4x4.xml"))[0].solutions[0]

    cost = evaluate_cost(instance, solution)

    assert cost == Cost(0, 0)
    assert cost.as_scalar() == total_cost(instance, solution) == 0


def _infeasibility_vs_objective_instance() -> Instance:
    return Instance(
        id="I1",
        name="Test",
        time_groups=[Group(id="gr_Preferred", name="Preferred", kind="TimeGroup")],
        times=[
            Time(id="P1", name="P1", group_refs=["gr_Preferred"]),
            Time(id="P2", name="P2"),
        ],
        event_groups=[Group(id="gr_All", name="All", kind="EventGroup")],
        events=[Event(id="E1", name="E1", duration=1, group_refs=["gr_All"])],
        constraints=[
            Constraint(
                type="AssignTimeConstraint",
                id="AT1",
                name="AssignTimes",
                required=True,
                weight=1,
                cost_function="Linear",
                applies_to=AppliesTo(event_groups=["gr_All"]),
            ),
            Constraint(
                type="PreferTimesConstraint",
                id="PT1",
                name="PreferTimes",
                required=False,
                weight=100,
                cost_function="Linear",
                applies_to=AppliesTo(event_groups=["gr_All"]),
                params={"TimeGroups": [{"reference": "gr_Preferred"}]},
            ),
        ],
    )


def test_cost_orders_infeasibility_lexicographically_above_objective():
    # A single point of infeasibility must outweigh any amount of
    # objective-only cost, even though the objective-only solution's raw
    # objective number (100) dwarfs the infeasible one's (0).
    instance = _infeasibility_vs_objective_instance()
    feasible_but_costly = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="P2")],
    )
    infeasible_but_cheap = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref=None)],
    )

    cost_costly = evaluate_cost(instance, feasible_but_costly)
    cost_cheap = evaluate_cost(instance, infeasible_but_cheap)

    assert cost_costly == Cost(infeasibility=0, objective=100)
    assert cost_cheap == Cost(infeasibility=1, objective=0)
    assert cost_cheap > cost_costly

    # The scalar flattening must stay consistent with evaluator_ref.total_cost.
    assert cost_costly.as_scalar() == total_cost(instance, feasible_but_costly)
    assert cost_cheap.as_scalar() == total_cost(instance, infeasible_but_cheap)


def _min_working_days_instance(event_times: list[str]) -> Instance:
    return Instance(
        id="I1",
        name="Test",
        time_groups=[
            Group(id="gr_D1", name="Day1", kind="Day"),
            Group(id="gr_D2", name="Day2", kind="Day"),
        ],
        times=[
            Time(id="P1", name="P1", group_refs=["gr_D1"]),
            Time(id="P2", name="P2", group_refs=["gr_D1"]),
            Time(id="P3", name="P3", group_refs=["gr_D2"]),
            Time(id="P4", name="P4", group_refs=["gr_D2"]),
        ],
        resource_types=[ResourceType(id="RT_Class", name="Class")],
        resources=[Resource(id="Class1", name="Class1", resource_type_ref="RT_Class")],
        events=[
            Event(
                id=f"E{i}",
                name=f"E{i}",
                duration=1,
                time_ref=t,
                resources=[EventResource(role="Class", resource_ref="Class1")],
            )
            for i, t in enumerate(event_times)
        ],
    )


def test_min_working_days_cost_penalizes_shortfall_below_minimum():
    # Both events land on day 1 only -> 1 busy day, short of minimum_days=2.
    instance = _min_working_days_instance(["P1", "P2"])
    solution = Solution(instance_ref=instance.id, events=[])

    assert min_working_days_cost(instance, solution, ["Class1"], minimum_days=2) == 1


def test_min_working_days_cost_is_zero_when_minimum_met():
    # One event per day -> 2 busy days, meets minimum_days=2.
    instance = _min_working_days_instance(["P1", "P3"])
    solution = Solution(instance_ref=instance.id, events=[])

    assert min_working_days_cost(instance, solution, ["Class1"], minimum_days=2) == 0


def test_min_working_days_cost_applies_the_given_cost_function():
    # No events at all -> 0 busy days, shortfall of 3 against minimum_days=3.
    instance = _min_working_days_instance([])
    solution = Solution(instance_ref=instance.id, events=[])

    assert (
        min_working_days_cost(
            instance, solution, ["Class1"], minimum_days=3, cost_function="Quadratic"
        )
        == 9
    )


def test_resources_of_type_filters_by_resource_type_ref():
    instance = _min_working_days_instance(["P1"])

    assert resources_of_type(instance, "RT_Class") == ["Class1"]
    assert resources_of_type(instance, "RT_Other") == []


def test_adagen_multiplier_grows_with_stagnation_and_caps():
    schedule = AdaGenSchedule(
        base_multiplier=1000, growth_per_stagnant_step=1.0, max_multiplier=5000
    )

    assert schedule.multiplier(0) == 1000
    assert schedule.multiplier(1) == 2000
    assert schedule.multiplier(10) == 5000  # capped


def test_adagen_scalar_uses_the_grown_multiplier():
    schedule = AdaGenSchedule(
        base_multiplier=10, growth_per_stagnant_step=1.0, max_multiplier=1000
    )
    cost = Cost(infeasibility=2, objective=3)

    assert schedule.scalar(cost, stagnant_iterations=1) == cost.as_scalar(20)
