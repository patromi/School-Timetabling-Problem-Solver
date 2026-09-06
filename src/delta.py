"""Incremental cost evaluation (Etap 3). delta_cost recomputes only the
constraints whose AppliesTo scope overlaps events/resources that actually
changed between old_solution and new_solution, instead of re-running every
constraint from scratch like src.cost.evaluate_cost does. See
docs/superpowers/specs/2026-08-16-delta-evaluation-design.md for the design
rationale and the correctness argument for why skipping untouched
constraints is sound, not just fast."""

from src import evaluator_ref
from src.cost import Cost
from src.evaluator_ref import (
    _assigned_resource_ids,
    _build_occupancy_index,
    _events_in_applies_to,
    _resources_in_applies_to,
    evaluate_constraint,
    resolve_occurrences,
)
from src.model import Constraint, Instance, Solution

# The only constraint types whose _evaluate_*_constraint function ever reads
# evaluator_ref._current_occupancy_index (directly, or via
# _full_span_busy_times) -- confirmed by grepping evaluator_ref.py. Building
# the occupancy index is itself an O(occurrences) pass done twice (old/new)
# per delta_cost call; skipping it when no touched constraint would even
# look at it avoids that cost entirely.
_OCCUPANCY_INDEX_CONSTRAINT_TYPES = frozenset(
    {
        "AvoidClashesConstraint",
        "ClusterBusyTimesConstraint",
        "AvoidUnavailableTimesConstraint",
        "LimitIdleTimesConstraint",
        "LimitBusyTimesConstraint",
    }
)


def _constraint_touches(
    instance: Instance,
    constraint: Constraint,
    touched_events: frozenset[str],
    touched_resources: frozenset[str],
) -> bool:
    """True if constraint's resolved AppliesTo scope (events/event groups,
    resources/resource groups, or event pairs) overlaps anything that
    changed -- i.e. its contribution to the total cost might differ
    between old_solution and new_solution. False means it's PROVEN
    identical (the constraint would see byte-for-byte the same Occurrence
    data on every position it looks at), not just probably unaffected."""
    if touched_events & _events_in_applies_to(instance, constraint.applies_to):
        return True
    if touched_resources & _resources_in_applies_to(instance, constraint.applies_to):
        return True
    return any(
        pair.first_event in touched_events or pair.second_event in touched_events
        for pair in constraint.applies_to.event_pairs
    )


def delta_cost(
    instance: Instance,
    old_solution: Solution,
    old_cost: Cost,
    new_solution: Solution,
) -> Cost:
    """Cost of new_solution, computed incrementally from old_solution's
    already-known old_cost -- numerically identical to
    evaluate_cost(instance, new_solution), but only re-evaluates
    constraints whose scope touches something that changed (see
    _constraint_touches). Assumes new_solution was produced from
    old_solution by a move that preserves event_ref and SolutionEvent
    count at every position (every heuristic in
    src.heuristics.MANUAL_HEURISTICS satisfies this) -- raises
    ValueError if the resolved occurrence counts differ, which means that
    assumption was violated.

    Note: unlike a fresh evaluate_cost call, delta_cost never touches --
    and so can't raise on -- a constraint type evaluate_constraint doesn't
    implement, if that constraint isn't touched by this move."""
    old_occurrences = resolve_occurrences(instance, old_solution)
    new_occurrences = resolve_occurrences(instance, new_solution)
    if len(old_occurrences) != len(new_occurrences):
        raise ValueError(
            "old_solution and new_solution resolve to a different number of "
            "occurrences -- delta_cost only supports moves that preserve "
            "event/split structure (see src.heuristics.MANUAL_HEURISTICS)"
        )

    changed = [
        k
        for k in range(len(old_occurrences))
        if old_occurrences[k] != new_occurrences[k]
    ]
    if not changed:
        return old_cost

    touched_events = frozenset(
        o.event_ref for k in changed for o in (old_occurrences[k], new_occurrences[k])
    )
    touched_resources = frozenset(
        r
        for k in changed
        for r in _assigned_resource_ids(old_occurrences[k])
        + _assigned_resource_ids(new_occurrences[k])
        if r is not None
    )

    touched_constraints = [
        constraint
        for constraint in instance.constraints
        if _constraint_touches(instance, constraint, touched_events, touched_resources)
    ]

    old_index = new_index = None
    if any(c.type in _OCCUPANCY_INDEX_CONSTRAINT_TYPES for c in touched_constraints):
        old_index = _build_occupancy_index(instance, old_occurrences)
        new_index = _build_occupancy_index(instance, new_occurrences)

    infeasibility, objective = old_cost.infeasibility, old_cost.objective
    for constraint in touched_constraints:
        evaluator_ref._current_occupancy_index = old_index
        try:
            old_contribution = evaluate_constraint(
                instance, old_occurrences, constraint
            )
        finally:
            evaluator_ref._current_occupancy_index = None

        evaluator_ref._current_occupancy_index = new_index
        try:
            new_contribution = evaluate_constraint(
                instance, new_occurrences, constraint
            )
        finally:
            evaluator_ref._current_occupancy_index = None

        change = new_contribution - old_contribution
        if change == 0:
            continue
        if constraint.required:
            infeasibility += change
        else:
            objective += change

    return Cost(infeasibility, objective)
