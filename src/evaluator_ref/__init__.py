from src.evaluator_ref._cache import (
    INFEASIBILITY_WEIGHT,
    _INSTANCE_REGISTRY,
    _event_group_members,
    _referenced_ids,
    _register_instance,
    _shortfall_or_excess,
    _time_ids_ordered,
    apply_cost_function,
    valid_start_time_ids,
)
from src.evaluator_ref.constraints import (
    _event_resource_workload,
    _preferred_resource_ids,
    _preferred_time_ids,
    evaluate_constraint,
)
from src.evaluator_ref.occurrences import (
    Occurrence,
    _assigned_resource,
    _assigned_resource_ids,
    _build_occupancy_index,
    _events_in_applies_to,
    _resources_in_applies_to,
    occupancy_index,
    occupancy_index_scope,
    resolve_occurrence,
    resolve_occurrences,
)
from src.model import Instance, Solution

__all__ = [
    "INFEASIBILITY_WEIGHT",
    "_INSTANCE_REGISTRY",
    "Occurrence",
    "_assigned_resource",
    "_assigned_resource_ids",
    "_event_resource_workload",
    "_shortfall_or_excess",
    "_build_occupancy_index",
    "_event_group_members",
    "_events_in_applies_to",
    "_preferred_resource_ids",
    "_preferred_time_ids",
    "_referenced_ids",
    "_register_instance",
    "_resources_in_applies_to",
    "_time_ids_ordered",
    "apply_cost_function",
    "evaluate_constraint",
    "evaluate_constraint_costs",
    "evaluate_cost_components",
    "occupancy_index",
    "occupancy_index_scope",
    "resolve_occurrence",
    "resolve_occurrences",
    "total_cost",
    "valid_start_time_ids",
]


def evaluate_constraint_costs(
    instance: Instance, solution: Solution
) -> tuple[int, ...]:
    """Per-constraint cost vector, in instance.constraints order. Same
    numbers evaluate_cost_components sums up, kept separate so a test can
    tell an incremental evaluator's per-constraint bookkeeping apart from
    the aggregate -- two cancelling errors are invisible in the total."""
    occ = resolve_occurrences(instance, solution)
    with occupancy_index(_build_occupancy_index(instance, occ)):
        return tuple(evaluate_constraint(instance, occ, c) for c in instance.constraints)


def evaluate_cost_components(instance: Instance, solution: Solution) -> tuple[int, int]:
    """Returns (infeasibility, objective) separately -- infeasibility is the
    sum of Required=true constraint costs, objective the sum of
    Required=false costs. `total_cost` below is just these two flattened
    into one scalar; `src.cost` builds the (infeasibility, objective)
    vector representation on top of this instead, for lexicographic
    comparison of two solutions without conflating the two."""
    occ = resolve_occurrences(instance, solution)
    with occupancy_index(_build_occupancy_index(instance, occ)):
        infeasibility = 0
        objective = 0
        for c in instance.constraints:
            cost = evaluate_constraint(instance, occ, c)
            if c.required:
                infeasibility += cost
            else:
                objective += cost
        return infeasibility, objective


def total_cost(instance: Instance, solution: Solution) -> int:
    """Lexicographic total: infeasibility (sum of Required=true constraint
    costs) dominates objective (sum of Required=false costs), matching the
    Env sketch in the thesis plan (`infeas * 1_000_000 + obj`) -- a single
    point of infeasibility always outweighs any amount of objective cost,
    so a solver comparing this scalar naturally prioritizes feasibility
    first."""
    infeasibility, objective = evaluate_cost_components(instance, solution)
    return infeasibility * INFEASIBILITY_WEIGHT + objective


def __getattr__(name: str) -> object:
    # Proxy live module-level state from submodules so that
    # `evaluator_ref._current_occupancy_index` works the same way it did
    # when evaluator_ref was a flat module instead of a package.
    if name == "_current_occupancy_index":
        from src.evaluator_ref import occurrences as _occ
        return _occ._current_occupancy_index
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
