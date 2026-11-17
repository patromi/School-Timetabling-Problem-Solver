"""Tests for instance profiling and preprocessing."""

from pathlib import Path

from src.heuristics import MANUAL_HEURISTICS, Heuristic
from src.parser import parse_archive
from src.preprocessing import (
    InstanceProfile,
    analyze_instance,
    filter_heuristic_pool,
    get_initial_q_map,
    get_recommended_solver_config,
)
from src.selectors import EpsilonGreedySelector, UCBSelector

FIXTURES_DIR = Path(__file__).parent / "fixtures"


def test_analyze_instance_brazil() -> None:
    path = FIXTURES_DIR / "BrazilInstance1.xml"
    instances = parse_archive(path.read_text(encoding="utf-8-sig"))
    assert len(instances) == 1
    inst = instances[0]

    profile = analyze_instance(inst)
    assert profile.instance_id.startswith("BrazilInstance1")
    assert profile.events_count == len(inst.events)
    assert profile.times_count == len(inst.times)
    assert profile.resources_count == len(inst.resources)
    assert profile.constraints_count == len(inst.constraints)
    assert 0.0 <= profile.mean_resource_utilization <= 1.0
    assert 0.0 <= profile.max_resource_utilization <= 1.0
    assert profile.scale in ("small", "medium", "large")
    assert profile.tightness_category in ("tight", "moderate", "loose")
    assert profile.domain_ratio > 0.0
    assert "BrazilInstance1" in profile.summary()


def test_analyze_instance_sudoku() -> None:
    path = FIXTURES_DIR / "ArtificialSudoku4x4.xml"
    instances = parse_archive(path.read_text(encoding="utf-8-sig"))
    assert len(instances) == 1
    inst = instances[0]

    profile = analyze_instance(inst)
    assert profile.instance_id.startswith("ArtificialSudoku4x4")
    assert profile.scale == "small"
    assert profile.summary() != ""


def test_filter_heuristic_pool_pruning() -> None:
    profile = InstanceProfile(
        instance_id="test",
        instance_name="test",
        events_count=100,
        times_count=30,
        resources_count=20,
        constraints_count=5,
        max_resource_utilization=0.8,
        mean_resource_utilization=0.5,
        max_class_utilization=0.9,
        mean_class_utilization=0.6,
        max_teacher_utilization=0.7,
        mean_teacher_utilization=0.5,
        has_avoid_clashes=True,
        has_unassigned_resources=False,  # Should prune resource_reassign
        has_splittable_events=False,    # Should prune split_resize
        is_tight=False,
    )

    filtered = filter_heuristic_pool(profile, MANUAL_HEURISTICS)
    filtered_ids = {h.id for h in filtered}

    assert "resource_reassign" not in filtered_ids
    assert "split_resize" not in filtered_ids
    assert "repair_clash" in filtered_ids
    assert "move_best" in filtered_ids
    assert "swap" in filtered_ids


def test_filter_heuristic_pool_without_clashes() -> None:
    profile = InstanceProfile(
        instance_id="test",
        instance_name="test",
        events_count=100,
        times_count=30,
        resources_count=20,
        constraints_count=5,
        max_resource_utilization=0.8,
        mean_resource_utilization=0.5,
        max_class_utilization=0.9,
        mean_class_utilization=0.6,
        max_teacher_utilization=0.7,
        mean_teacher_utilization=0.5,
        has_avoid_clashes=False,        # Should prune repair_clash
        has_unassigned_resources=True,
        has_splittable_events=True,
        is_tight=False,
    )

    filtered = filter_heuristic_pool(profile, MANUAL_HEURISTICS)
    filtered_ids = {h.id for h in filtered}

    assert "repair_clash" not in filtered_ids
    assert "resource_reassign" in filtered_ids
    assert "split_resize" in filtered_ids


def test_initial_q_map_tight_vs_loose() -> None:
    tight_profile = InstanceProfile(
        instance_id="tight_inst",
        instance_name="tight",
        events_count=200,
        times_count=40,
        resources_count=30,
        constraints_count=10,
        max_resource_utilization=1.0,
        mean_resource_utilization=0.85,
        max_class_utilization=1.0,
        mean_class_utilization=1.0,
        max_teacher_utilization=0.9,
        mean_teacher_utilization=0.8,
        is_tight=True,
        tightness_category="tight",
        scale="medium",
    )
    tight_priors = get_initial_q_map(tight_profile)
    # Swaps and Kempe chains should be prioritized over random move
    assert tight_priors["swap"] > tight_priors["move_random"]
    assert tight_priors["kempe_chain"] > tight_priors["move_random"]

    loose_profile = InstanceProfile(
        instance_id="loose_inst",
        instance_name="loose",
        events_count=200,
        times_count=60,
        resources_count=30,
        constraints_count=10,
        max_resource_utilization=0.5,
        mean_resource_utilization=0.35,
        max_class_utilization=0.5,
        mean_class_utilization=0.5,
        max_teacher_utilization=0.4,
        mean_teacher_utilization=0.3,
        is_tight=False,
        tightness_category="loose",
        scale="medium",
    )
    loose_priors = get_initial_q_map(loose_profile)
    # On loose instances, clash repair and move best are highly prioritized
    assert loose_priors["repair_clash"] >= 0.75
    assert loose_priors["move_best"] >= 0.70


def test_initial_q_map_large_scale_penalty() -> None:
    large_profile = InstanceProfile(
        instance_id="large_inst",
        instance_name="large",
        events_count=900,
        times_count=60,
        resources_count=50,
        constraints_count=10,
        max_resource_utilization=0.6,
        mean_resource_utilization=0.45,
        max_class_utilization=0.6,
        mean_class_utilization=0.5,
        max_teacher_utilization=0.5,
        mean_teacher_utilization=0.4,
        is_tight=False,
        tightness_category="moderate",
        scale="large",
    )
    priors = get_initial_q_map(large_profile)
    # ruin_and_recreate should be penalized to protect iteration throughput
    assert priors["ruin_and_recreate"] < 0.50


def test_recommended_solver_config() -> None:
    profile = InstanceProfile(
        instance_id="inst",
        instance_name="inst",
        events_count=800,
        times_count=60,
        resources_count=40,
        constraints_count=10,
        max_resource_utilization=0.5,
        mean_resource_utilization=0.4,
        max_class_utilization=0.5,
        mean_class_utilization=0.5,
        max_teacher_utilization=0.4,
        mean_teacher_utilization=0.3,
        is_tight=False,
        scale="large",
        has_avoid_clashes=True,
        has_unassigned_resources=False,
        has_splittable_events=False,
    )
    config = get_recommended_solver_config(profile)
    assert config["idle_limit"] == max(3000, 50 * 800)
    assert config["perturbations"] is False  # Large scale recommendation
    assert config["max_probe"] == 20
    assert isinstance(config["heuristics"], list)
    assert isinstance(config["initial_q_map"], dict)


def test_ucb_with_initial_q_map() -> None:
    q_map = {"move_best": 0.85, "swap": 0.30}
    selector = UCBSelector(initial_q=0.5, initial_q_map=q_map)

    # Initializing heuristics
    h_best = Heuristic("move_best", "Move Best", True, lambda s, i, r: s)
    h_swap = Heuristic("swap", "Swap", True, lambda s, i, r: s)
    h_other = Heuristic("move_random", "Move Random", True, lambda s, i, r: s)

    selector._init_heuristic(h_best.id)
    selector._init_heuristic(h_swap.id)
    selector._init_heuristic(h_other.id)

    # State (0, 0)
    assert selector._stats[((0, 0), "move_best")][0] == 0.85
    assert selector._stats[((0, 0), "swap")][0] == 0.30
    assert selector._stats[((0, 0), "move_random")][0] == 0.50  # default initial_q


def test_epsilon_greedy_with_initial_q_map() -> None:
    q_map = {"move_best": 0.9, "swap": 0.4}
    selector = EpsilonGreedySelector(initial_q_map=q_map)

    h_best = Heuristic("move_best", "Move Best", True, lambda s, i, r: s)
    h_other = Heuristic("move_random", "Move Random", True, lambda s, i, r: s)

    import random
    rng = random.Random(42)
    # In select(), heuristics are lazily initialized
    selector.select([h_best, h_other], rng)

    assert selector._q_values["move_best"] == 0.9
    assert selector._q_values["move_random"] == 0.0


def test_initial_q_map_structural_modifiers() -> None:
    # 1. Unassigned resource roles (e.g. dynamic teacher/room assignment)
    dyn_profile = InstanceProfile(
        instance_id="generic_dyn",
        instance_name="generic",
        events_count=200,
        times_count=40,
        resources_count=30,
        constraints_count=10,
        max_resource_utilization=0.6,
        mean_resource_utilization=0.4,
        max_class_utilization=0.6,
        mean_class_utilization=0.5,
        max_teacher_utilization=0.5,
        mean_teacher_utilization=0.4,
        has_unassigned_resources=True,
        unassigned_resources_ratio=0.35,
        tightness_category="moderate",
        scale="medium",
    )
    priors_dyn = get_initial_q_map(dyn_profile)
    assert priors_dyn["resource_reassign"] == 0.85

    # 2. Linked events interdependency (e.g. coupled simultaneous events)
    linked_profile = InstanceProfile(
        instance_id="generic_linked",
        instance_name="generic",
        events_count=200,
        times_count=40,
        resources_count=30,
        constraints_count=10,
        max_resource_utilization=0.6,
        mean_resource_utilization=0.4,
        max_class_utilization=0.6,
        mean_class_utilization=0.5,
        max_teacher_utilization=0.5,
        mean_teacher_utilization=0.4,
        has_link_events=True,
        link_events_count=12,
        tightness_category="moderate",
        scale="medium",
    )
    priors_linked = get_initial_q_map(linked_profile)
    assert priors_linked["kempe_chain"] == 0.80

    # 3. Avoid clashes constraint
    clash_profile = InstanceProfile(
        instance_id="generic_clash",
        instance_name="generic",
        events_count=200,
        times_count=40,
        resources_count=30,
        constraints_count=10,
        max_resource_utilization=0.6,
        mean_resource_utilization=0.4,
        max_class_utilization=0.6,
        mean_class_utilization=0.5,
        max_teacher_utilization=0.5,
        mean_teacher_utilization=0.4,
        has_avoid_clashes=True,
        tightness_category="moderate",
        scale="medium",
    )
    priors_clash = get_initial_q_map(clash_profile)
    assert priors_clash["repair_clash"] >= 0.80
