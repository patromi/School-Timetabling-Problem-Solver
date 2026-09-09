from src.evaluator_ref._cache import (
    INFEASIBILITY_WEIGHT,
    apply_cost_function,
    valid_start_time_ids,
)
from src.evaluator_ref.constraints import evaluate_constraint
from src.evaluator_ref.occurrences import (
    Occurrence,
    _assigned_resource_ids,
    _build_occupancy_index,
    _events_in_applies_to,
    _resources_in_applies_to,
    occupancy_index,
    resolve_occurrences,
)
from src.model import Instance, Solution

__all__ = [
    "INFEASIBILITY_WEIGHT",
    "Occurrence",
    "_assigned_resource_ids",
    "_build_occupancy_index",
    "_events_in_applies_to",
    "_resources_in_applies_to",
    "apply_cost_function",
    "evaluate_constraint",
    "evaluate_cost_components",
    "occupancy_index",
    "resolve_occurrences",
    "total_cost",
    "valid_start_time_ids",
]


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
