import random
from collections.abc import Callable
from dataclasses import replace
from pathlib import Path

import pytest
from src.construct import build_greedy, build_initial
from src.cost import evaluate_cost
from src.evaluator_ref import evaluate_constraint, resolve_occurrences
from src.heuristics import (
    MANUAL_HEURISTICS,
    RESOURCE_HEURISTICS,
    default_pool,
    repair_resource_clash,
)
from src.incremental import IncrementalEvaluator
from src.model import Instance, Solution
from src.moves import resource_reassign_move, resource_swap_move
from src.parser import parse_archive
from src.resource_domains import (
    assignment_group,
    has_resource_choices,
    resource_domain,
    resource_units,
    soft_preferred,
    with_unit_resource,
)

FIXTURES = Path(__file__).parent / "fixtures"
ROOMS = ("R1", "R2", "Lab1", "Lab2")
TEACHERS = ("T1", "T2", "T3")


def _instance(name: str = "ResourceAssignment.xml") -> Instance:
    return parse_archive((FIXTURES / name).read_text(encoding="utf-8"))[0]


def _assignments(solution: Solution) -> list[tuple[str, str, str]]:
    return [
        (se.event_ref, sr.role, sr.resource_ref)
        for se in solution.events
        for sr in se.resources
    ]


def _teacher_of(solution: Solution, event_ref: str) -> set[str]:
    return {
        sr.resource_ref
        for se in solution.events
        if se.event_ref == event_ref
        for sr in se.resources
        if sr.role == "Teacher"
    }


def test_domain_applies_required_preferences_only() -> None:
    instance = _instance()

    assert resource_domain(instance, "E3", "Room") == ("Lab1", "Lab2")
    assert resource_domain(instance, "E1", "Room") == ROOMS
    assert resource_domain(instance, "E1", "Teacher") == TEACHERS
    assert resource_domain(instance, "E3", "Teacher") == ()  # preassigned
    assert soft_preferred(instance, "E5", "Room") == {"R2"}
    assert soft_preferred(instance, "E1", "Room") == set(ROOMS)


def test_assignment_group_follows_avoid_split_assignments() -> None:
    instance = _instance()

    assert assignment_group(instance, "E1", "Teacher") == {"E1", "E2"}
    assert assignment_group(instance, "E1", "Room") == {"E1"}
    assert assignment_group(instance, "E5", "Teacher") == {"E5"}


def test_has_resource_choices() -> None:
    assert has_resource_choices(_instance())
    assert not has_resource_choices(_instance("BrazilInstance1.xml"))
    # every Sudoku room slot has a one-room domain
    assert not has_resource_choices(_instance("ArtificialSudoku4x4.xml"))


@pytest.mark.parametrize("build", [build_initial, build_greedy])
@pytest.mark.parametrize("seed", range(5))
def test_constructors_fill_every_slot_from_its_domain(
    build: Callable[[Instance, random.Random], Solution], seed: int
) -> None:
    instance = _instance()
    solution = build(instance, random.Random(seed))

    for event_ref, role, resource in _assignments(solution):
        assert resource in resource_domain(instance, event_ref, role)
    assert len(_teacher_of(solution, "E1") | _teacher_of(solution, "E2")) == 1


@pytest.mark.parametrize("seed", range(5))
def test_greedy_constructor_avoids_resource_clashes(seed: int) -> None:
    instance = _instance()
    solution = build_greedy(instance, random.Random(seed))

    assert evaluate_cost(instance, solution).infeasibility == 0


def test_resource_units_group_pieces_tied_by_avoid_split_assignments() -> None:
    instance = _instance()
    solution = build_initial(instance, random.Random(0))

    units = resource_units(instance, solution)
    math_teacher = [u for u in units if u.role == "Teacher" and len(u.entries) > 1]

    assert len(math_teacher) == 1
    events = {solution.events[i].event_ref for i, _ in math_teacher[0].entries}
    assert events == {"E1", "E2"}
    assert len(math_teacher[0].entries) == 3  # two pieces of E1, one of E2
    assert all(len(u.entries) == 1 for u in units if u.role == "Room")


def test_with_unit_resource_leaves_input_untouched() -> None:
    instance = _instance()
    solution = build_initial(instance, random.Random(0))
    before = _assignments(solution)
    unit = next(u for u in resource_units(instance, solution) if len(u.entries) > 1)
    other = next(t for t in TEACHERS if t != unit.resource)

    new = with_unit_resource(solution, unit, other)

    assert _assignments(solution) == before
    assert _teacher_of(new, "E1") == _teacher_of(new, "E2") == {other}


@pytest.mark.parametrize("seed", range(10))
def test_resource_reassign_move_changes_one_unit_within_its_domain(seed: int) -> None:
    instance = _instance()
    solution = build_initial(instance, random.Random(0))
    before = _assignments(solution)

    new = resource_reassign_move(instance, solution, random.Random(seed))

    assert _assignments(solution) == before
    changed = [(a, b) for a, b in zip(before, _assignments(new), strict=True) if a != b]
    assert changed
    assert len({b[2] for _, b in changed}) == 1
    for _, (event_ref, role, resource) in changed:
        assert resource in resource_domain(instance, event_ref, role)
    assert len(_teacher_of(new, "E1") | _teacher_of(new, "E2")) == 1


@pytest.mark.parametrize("seed", range(10))
def test_resource_swap_move_exchanges_two_resources(seed: int) -> None:
    instance = _instance()
    solution = build_initial(instance, random.Random(1))
    before = _assignments(solution)

    new = resource_swap_move(instance, solution, random.Random(seed))

    after = _assignments(new)
    assert _assignments(solution) == before
    assert sorted(r for *_, r in before) == sorted(r for *_, r in after)
    assert before != after
    for event_ref, role, resource in after:
        assert resource in resource_domain(instance, event_ref, role)


def _room_clash(instance: Instance) -> Solution:
    """A greedy start with E5 moved into E6's time and room."""
    solution = build_greedy(instance, random.Random(0))
    events = list(solution.events)
    e5 = next(i for i, se in enumerate(events) if se.event_ref == "E5")
    e6 = next(i for i, se in enumerate(events) if se.event_ref == "E6")
    e6_room = next(sr for sr in events[e6].resources if sr.role == "Room")
    events[e5] = replace(
        events[e5],
        time_ref=events[e6].time_ref,
        resources=[
            replace(sr, resource_ref=e6_room.resource_ref) if sr.role == "Room" else sr
            for sr in events[e5].resources
        ],
    )
    return replace(solution, events=events)


def _clash_cost(instance: Instance, solution: Solution) -> int:
    occurrences = resolve_occurrences(instance, solution)
    return sum(
        evaluate_constraint(instance, occurrences, c)
        for c in instance.constraints
        if c.type == "AvoidClashesConstraint"
    )


@pytest.mark.parametrize("seed", range(5))
def test_repair_resource_clash_moves_a_clashing_resource(seed: int) -> None:
    instance = _instance()
    solution = _room_clash(instance)
    assert _clash_cost(instance, solution) > 0

    repaired = repair_resource_clash(solution, instance, random.Random(seed))

    assert _clash_cost(instance, repaired) < _clash_cost(instance, solution)
    for event_ref, role, resource in _assignments(repaired):
        assert resource in resource_domain(instance, event_ref, role)


def test_repair_resource_clash_raises_without_a_clash() -> None:
    instance = _instance()
    solution = build_greedy(instance, random.Random(0))
    with pytest.raises(ValueError):
        repair_resource_clash(solution, instance, random.Random(0))


def test_default_pool_adds_resource_heuristics_only_when_needed() -> None:
    ids = {h.id for h in RESOURCE_HEURISTICS}

    assert ids <= {h.id for h in default_pool(_instance())}
    assert default_pool(_instance("BrazilInstance1.xml")) == MANUAL_HEURISTICS


def test_incremental_cost_matches_full_evaluation_after_resource_moves() -> None:
    instance = _instance()
    rng = random.Random(7)
    solution = build_initial(instance, rng)
    evaluator = IncrementalEvaluator(instance, solution)
    pool = [h for h in default_pool(instance) if h.incremental_safe]

    applied = 0
    for _ in range(300):
        heuristic = rng.choice(pool)
        try:
            candidate = heuristic.apply(solution, instance, rng)
        except ValueError:
            continue
        assert evaluator.probe(candidate) == evaluate_cost(instance, candidate)
        evaluator.commit(candidate)
        assert evaluator.cost == evaluate_cost(instance, candidate)
        solution = candidate
        applied += 1
    assert applied > 100
