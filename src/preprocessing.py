"""Instance profiling and preprocessing for school timetabling.

Analyzes instance size, resource utilization/tightness, and constraint structures
to adapt the heuristic pool, initial selector weights (priors), and solver
hyperparameters. Based on empirical findings in docs/wnioski_2026-10-18.md.
"""

from dataclasses import dataclass, field
from typing import Any

from src.construct import _split_duration_bounds
from src.evaluator_ref import valid_start_time_ids
from src.heuristics import MANUAL_HEURISTICS, Heuristic
from src.model import Instance


@dataclass(frozen=True)
class ResourceUtilization:
    """Utilization statistics for a specific resource type."""

    type_id: str
    count: int
    mean: float
    max: float
    min: float


@dataclass(frozen=True)
class InstanceProfile:
    """Structural complexity profile of a timetabling instance."""

    instance_id: str
    instance_name: str
    events_count: int
    times_count: int
    resources_count: int
    constraints_count: int

    # Resource tightness and utilization
    max_resource_utilization: float
    mean_resource_utilization: float
    max_class_utilization: float
    mean_class_utilization: float
    max_teacher_utilization: float
    mean_teacher_utilization: float
    utilization_by_type: dict[str, ResourceUtilization] = field(default_factory=dict)
    is_tight: bool = False

    # Heuristic applicability flags
    has_avoid_clashes: bool = True
    has_unassigned_resources: bool = False
    unassigned_resources_count: int = 0
    unassigned_resources_ratio: float = 0.0
    has_splittable_events: bool = False
    has_link_events: bool = False
    link_events_count: int = 0
    mean_domain_size: float = 0.0
    domain_ratio: float = 1.0

    # Categorical classifications
    scale: str = "medium"  # "small" (<200), "medium" (200-500), "large" (>500)
    tightness_category: str = "moderate"  # "tight", "moderate", "loose"

    def summary(self) -> str:
        """Human-readable one-line summary for CLI and logs."""
        return (
            f"[{self.instance_id}] scale={self.scale} ({self.events_count} events, "
            f"{self.times_count} times, {self.resources_count} resources), "
            f"tightness={self.tightness_category} "
            f"(mean util={self.mean_resource_utilization:.1%}, "
            f"class max={self.max_class_utilization:.1%}), "
            f"domain_ratio={self.domain_ratio:.1%}"
        )


def _is_class_type(type_id: str) -> bool:
    """Check if resource type represents classes or student groups."""
    lower = type_id.lower()
    return any(keyword in lower for keyword in ("class", "student", "group", "grade"))


def _is_teacher_type(type_id: str) -> bool:
    """Check if resource type represents teachers or instructors."""
    lower = type_id.lower()
    return any(keyword in lower for keyword in ("teach", "prof", "instruct"))


def _compute_demands(
    instance: Instance,
) -> tuple[dict[str, int], bool, int, float]:
    """Sum duration demanded per resource and count unassigned resource roles."""
    demands: dict[str, int] = {r.id: 0 for r in instance.resources}
    unassigned_count = 0
    total_er = 0

    for event in instance.events:
        for er in event.resources:
            total_er += 1
            if er.resource_ref is not None:
                if er.resource_ref in demands:
                    demands[er.resource_ref] += event.duration
            else:
                unassigned_count += 1

    has_unassigned = unassigned_count > 0
    unassigned_ratio = (unassigned_count / total_er) if total_er > 0 else 0.0
    return demands, has_unassigned, unassigned_count, unassigned_ratio


def _compute_utilization_by_type(
    instance: Instance, utils: dict[str, float]
) -> tuple[dict[str, ResourceUtilization], list[float], list[float]]:
    """Group resource utilization by type and separate class/teacher lists."""
    r_to_type = {r.id: r.resource_type_ref for r in instance.resources}
    type_utils: dict[str, list[float]] = {}
    for r_id, u in utils.items():
        t_id = r_to_type.get(r_id, "unknown")
        type_utils.setdefault(t_id, []).append(u)

    util_by_type: dict[str, ResourceUtilization] = {}
    class_utils: list[float] = []
    teacher_utils: list[float] = []

    for t_id, vals in type_utils.items():
        if not vals:
            continue
        stat = ResourceUtilization(
            type_id=t_id,
            count=len(vals),
            mean=sum(vals) / len(vals),
            max=max(vals),
            min=min(vals),
        )
        util_by_type[t_id] = stat

        if _is_class_type(t_id):
            class_utils.extend(vals)
        elif _is_teacher_type(t_id):
            teacher_utils.extend(vals)

    return util_by_type, class_utils, teacher_utils


def _classify_tightness(
    mean_util: float,
    max_class: float,
    mean_class: float,
) -> str:
    """Classify tightness as 'tight', 'moderate', or 'loose'."""
    # Instances like ZA-WD-09 have mean_util ~0.89 and class max 1.0 (skrajnie ciasna).
    # Instances like KS-PR-11 have mean_util ~0.38 and class max 0.50 (luźna).
    if mean_util >= 0.70 or (
        max_class >= 0.95 and mean_class >= 0.90 and mean_util >= 0.55
    ):
        return "tight"
    if mean_util < 0.40:
        return "loose"
    return "moderate"


def _check_splittable_events(instance: Instance) -> bool:
    """Check if any event in the instance can legally be split."""
    has_split_constraint = any(
        c.type == "SplitEventsConstraint" for c in instance.constraints
    )
    if not has_split_constraint:
        return False

    for e in instance.events:
        if e.duration > 1:
            min_d, _ = _split_duration_bounds(instance, e)
            if min_d < e.duration:
                return True
    return False


def analyze_instance(instance: Instance) -> InstanceProfile:
    """Extract structural metrics and complexity features from an Instance.

    Runs in a few milliseconds without requiring any search or evaluation state.
    """
    times_count = max(1, len(instance.times))
    events_count = len(instance.events)
    resources_count = len(instance.resources)
    constraints_count = len(instance.constraints)

    demands, has_unassigned, unassigned_count, unassigned_ratio = _compute_demands(
        instance
    )
    utils: dict[str, float] = {r_id: d / times_count for r_id, d in demands.items()}
    util_by_type, class_utils, teacher_utils = _compute_utilization_by_type(
        instance, utils
    )

    all_utils = list(utils.values())
    mean_res_util = sum(all_utils) / len(all_utils) if all_utils else 0.0
    max_res_util = max(all_utils) if all_utils else 0.0

    mean_class_util = sum(class_utils) / len(class_utils) if class_utils else 0.0
    max_class_util = max(class_utils) if class_utils else 0.0

    mean_teach_util = sum(teacher_utils) / len(teacher_utils) if teacher_utils else 0.0
    max_teach_util = max(teacher_utils) if teacher_utils else 0.0

    tightness = _classify_tightness(mean_res_util, max_class_util, mean_class_util)

    scale = (
        "small"
        if events_count < 200
        else "medium"
        if events_count <= 500
        else "large"
    )

    has_avoid_clashes = any(
        c.type == "AvoidClashesConstraint" and c.required for c in instance.constraints
    )
    has_splittable = _check_splittable_events(instance)

    link_events_count = sum(
        1 for c in instance.constraints if c.type == "LinkEventsConstraint"
    )
    has_link_events = link_events_count > 0

    domain_sizes = [
        len(valid_start_time_ids(instance, e.duration)) for e in instance.events
    ]
    mean_dom = (
        sum(domain_sizes) / len(domain_sizes) if domain_sizes else float(times_count)
    )
    domain_ratio = mean_dom / times_count

    return InstanceProfile(
        instance_id=instance.id,
        instance_name=instance.name,
        events_count=events_count,
        times_count=times_count,
        resources_count=resources_count,
        constraints_count=constraints_count,
        max_resource_utilization=round(max_res_util, 4),
        mean_resource_utilization=round(mean_res_util, 4),
        max_class_utilization=round(max_class_util, 4),
        mean_class_utilization=round(mean_class_util, 4),
        max_teacher_utilization=round(max_teach_util, 4),
        mean_teacher_utilization=round(mean_teach_util, 4),
        utilization_by_type=util_by_type,
        is_tight=tightness == "tight",
        has_avoid_clashes=has_avoid_clashes,
        has_unassigned_resources=has_unassigned,
        unassigned_resources_count=unassigned_count,
        unassigned_resources_ratio=round(unassigned_ratio, 4),
        has_splittable_events=has_splittable,
        has_link_events=has_link_events,
        link_events_count=link_events_count,
        mean_domain_size=round(mean_dom, 2),
        domain_ratio=round(domain_ratio, 4),
        scale=scale,
        tightness_category=tightness,
    )


def filter_heuristic_pool(
    profile: InstanceProfile,
    pool: list[Heuristic] | None = None,
) -> list[Heuristic]:
    """Prune operators from the pool that are inapplicable to this instance.

    Prevents wasting UCB exploration pulls and avoids unhandled ValueErrors
    during local search.
    """
    source = pool if pool is not None else MANUAL_HEURISTICS
    excluded: set[str] = set()

    if not profile.has_unassigned_resources:
        excluded.add("resource_reassign")
    if not profile.has_splittable_events:
        excluded.add("split_resize")
    if not profile.has_avoid_clashes:
        excluded.add("repair_clash")

    return [h for h in source if h.id not in excluded]


def get_initial_q_map(profile: InstanceProfile) -> dict[str, float]:
    """Calculate recommended initial Q-values (priors) for the UCB selector.

    Derived from empirical benchmarks across 75M+ solver iterations on XHSTT:
    - ruin_and_recreate, move_best, and repair_hard_violation are core workhorses.
    - On tight instances, ruin_and_recreate unblocks congested schedules, while
      swaps and random moves almost always cause clashes.
    - On loose instances, greedy best moves rapidly find abundant open slots.
    - On instances with unassigned roles, resource_reassign is strongly promoted.
    - On instances with linked events, kempe_chain swaps coupled components.
    - On large instances, ruin_and_recreate starts slightly lower for throughput.
    """
    priors: dict[str, float] = {}

    if profile.tightness_category == "tight":
        # Tight packing: ruin_and_recreate unblocks congested schedules,
        # greedy repair cleans up hard violations, while random swap causes clashes.
        priors["ruin_and_recreate"] = 0.80
        priors["repair_hard_violation"] = 0.75
        priors["move_best"] = 0.70
        priors["repair_clash"] = 0.75
        priors["kempe_chain"] = 0.60
        priors["move_random"] = 0.30
        priors["swap"] = 0.35
        priors["split_resize"] = 0.25
    elif profile.tightness_category == "loose":
        # Abundant free slots: greedy best moves and direct hard violation repairs
        # rapidly establish a zero-conflict baseline schedule.
        priors["move_best"] = 0.85
        priors["repair_hard_violation"] = 0.80
        priors["repair_clash"] = 0.80
        priors["ruin_and_recreate"] = 0.65
        priors["kempe_chain"] = 0.50
        priors["move_random"] = 0.35
        priors["swap"] = 0.30
        priors["split_resize"] = 0.30
    else:  # moderate
        priors["move_best"] = 0.80
        priors["repair_hard_violation"] = 0.75
        priors["ruin_and_recreate"] = 0.70
        priors["repair_clash"] = 0.75
        priors["kempe_chain"] = 0.55
        priors["move_random"] = 0.30
        priors["swap"] = 0.30
        priors["split_resize"] = 0.25

    # Structural feature modifiers (derived purely from instance characteristics)

    # 1. Dynamic / Unassigned Resource Roles
    if profile.has_unassigned_resources:
        if profile.unassigned_resources_ratio >= 0.15:
            priors["resource_reassign"] = 0.85
        else:
            priors["resource_reassign"] = 0.70

    # 2. Linked Events / High Interdependency Structures
    if profile.link_events_count >= 5:
        priors["kempe_chain"] = max(priors.get("kempe_chain", 0.50), 0.80)

    # 3. Clash Pressure
    if profile.has_avoid_clashes:
        priors["repair_clash"] = max(priors.get("repair_clash", 0.70), 0.80)

    # 4. Large scale throughput protection (slowest operator on 800+ events)
    if profile.scale == "large":
        if "ruin_and_recreate" in priors:
            priors["ruin_and_recreate"] = max(0.20, priors["ruin_and_recreate"] - 0.25)

    return priors


def get_recommended_solver_config(profile: InstanceProfile) -> dict[str, Any]:
    """Return recommended solver hyperparameters based on instance complexity.

    Based on the empirical findings in docs/wnioski_2026-10-18.md:
    - Large instances scale best without perturbations (avoiding huge SA reheats).
    - idle_limit scales with the event count: max(3000, 50 * events_count).
    - max_probe is capped on large instances to avoid exhaustive scan slowdowns.
    """
    idle_limit = max(3000, 50 * profile.events_count)
    perturbations = profile.scale != "large"
    max_probe = 20 if (profile.scale == "large" or profile.times_count > 60) else None

    return {
        "idle_limit": idle_limit,
        "perturbations": perturbations,
        "max_probe": max_probe,
        "heuristics": filter_heuristic_pool(profile),
        "initial_q_map": get_initial_q_map(profile),
    }
