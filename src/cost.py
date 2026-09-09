"""Cost VECTOR representation on top of evaluator_ref's scalar total_cost.

CLAUDE.md's target design: "Koszt rozwiazania to para (infeasibility,
objective) porownywana leksykograficznie... Dla selektora RL koszt
splaszczamy do skalaru: HARD_MULTIPLIER * infeasibility + objective, ale w
logach i raportach zawsze trzymamy pare osobno." `evaluator_ref.total_cost`
only ever returns the already-flattened scalar; `Cost` here is the pair
itself, so callers that need to log/compare the two components separately
(the RL selector's reward, experiment logs) don't have to reconstruct them
by re-running the evaluator.

Also home to two pieces that are deliberately NOT part of the XHSTT spec and
therefore never contribute to `Cost`/`total_cost`:

- `min_working_days_cost` -- a custom, instance-agnostic diagnostic metric
  (XHSTT's 16 official constraint types have no "MinWorkingDays"; the
  closest spec constraint, ClusterBusyTimesConstraint, penalizes a
  *resource* -- typically a teacher -- for being active on too many/few
  days, but only if the instance's XML happens to declare it for that
  resource). This one is opt-in: call it explicitly with the resource ids
  and threshold you care about.
- `AdaGenSchedule` -- an ADAGEN-style (Adaptive Genetic Penalty) growing
  infeasibility multiplier, for a *future* stagnation-aware acceptance loop
  (Etap 5 in CLAUDE.md; the current LAHC loop in lahc.py has no such
  mechanism yet). Not wired into anything -- it is the multiplier a future
  on_progress/acceptance callback would read.
"""

from collections.abc import Iterable
from dataclasses import dataclass

from src.evaluator_ref import (
    INFEASIBILITY_WEIGHT,
    Occurrence,
    apply_cost_function,
    evaluate_cost_components,
    resolve_occurrences,
)
from src.model import Instance, Solution


@dataclass(frozen=True, order=True)
class Cost:
    """(infeasibility, objective) pair. `order=True` makes dataclass compare
    instances field-by-field in declaration order -- infeasibility first,
    objective second -- which IS lexicographic comparison: any
    infeasibility difference decides the result outright, objective only
    breaks ties. So `Cost(1, 0) > Cost(0, 10**9)` holds, matching "a single
    point of infeasibility always outweighs any amount of objective cost"."""

    infeasibility: int
    objective: int

    def as_scalar(self, hard_multiplier: int = INFEASIBILITY_WEIGHT) -> int:
        """Flattens to the single number the RL selector's reward and the
        LAHC acceptance rule compare -- must stay consistent with
        evaluator_ref.total_cost, which is why the default multiplier is
        imported from there rather than redefined here."""
        return self.infeasibility * hard_multiplier + self.objective


def evaluate_cost(instance: Instance, solution: Solution) -> Cost:
    """The vector counterpart of evaluator_ref.total_cost -- same numbers,
    kept apart instead of pre-combined."""
    infeasibility, objective = evaluate_cost_components(instance, solution)
    return Cost(infeasibility, objective)


def resources_of_type(instance: Instance, resource_type_ref: str) -> list[str]:
    """Ids of every resource of a given ResourceType (e.g. whichever type an
    instance uses for student groups/classes) -- the usual way to build the
    `resource_ids` argument for `min_working_days_cost` below, since XHSTT
    has no fixed name for "the student-group resource type" across
    instances."""
    return [
        r.id for r in instance.resources if r.resource_type_ref == resource_type_ref
    ]


def _busy_time_ids(
    instance: Instance, occurrences: list[Occurrence], resource_id: str
) -> set[str]:
    # Deliberately self-contained rather than reusing evaluator_ref's
    # (private, cache-heavy) _full_span_busy_times: that helper is tuned for
    # being called hundreds of thousands of times per LAHC iteration across
    # every XHSTT constraint evaluator, which this non-spec, opt-in metric
    # is not.
    time_ids = [t.id for t in instance.times]
    positions = {t: i for i, t in enumerate(time_ids)}
    busy: set[str] = set()
    for o in occurrences:
        if o.time_ref is None:
            continue
        if resource_id not in (ref for _, ref in o.resource_assignments):
            continue
        start = positions[o.time_ref]
        busy |= set(time_ids[start : start + o.duration])
    return busy


def _day_group_of(instance: Instance, time_id: str) -> str | None:
    day_group_ids = {g.id for g in instance.time_groups if g.kind == "Day"}
    time = next(t for t in instance.times if t.id == time_id)
    return next((ref for ref in time.group_refs if ref in day_group_ids), None)


def min_working_days_cost(
    instance: Instance,
    solution: Solution,
    resource_ids: Iterable[str],
    minimum_days: int,
    cost_function: str = "Linear",
) -> int:
    """Non-XHSTT diagnostic metric: for each given resource, penalizes a
    shortfall of distinct Day-groups it has >=1 occurrence in, below
    `minimum_days` (e.g. flagging a class that only comes in on 2 days a
    week instead of spreading across 5). Symmetric to
    ClusterBusyTimesConstraint's Minimum bound, but resource-id-driven and
    always active rather than gated on the instance's XML declaring it.

    Uses the same Linear/Quadratic/Step convention (`apply_cost_function`)
    as every XHSTT constraint evaluator, for consistency -- but this value
    is NOT part of Cost/total_cost. Callers must add it in explicitly
    wherever they want it (e.g. into the RL reward), the same way they must
    pick which resources and threshold apply.
    """
    occurrences = resolve_occurrences(instance, solution)
    total = 0
    for resource_id in resource_ids:
        busy = _busy_time_ids(instance, occurrences, resource_id)
        days = {_day_group_of(instance, t) for t in busy} - {None}
        deviation = max(0, minimum_days - len(days))
        total += apply_cost_function(cost_function, deviation)
    return total


@dataclass(frozen=True)
class AdaGenSchedule:
    """ADAGEN (Adaptive Genetic Penalty): grows the infeasibility multiplier
    with the number of non-improving ("stagnant") iterations, so a search
    stuck unable to reduce infeasibility to 0 -- even though it's reachable
    -- increasingly ignores objective cost when comparing candidates as a
    scalar. Cost's own lexicographic ordering (via `order=True`) already
    ignores objective whenever infeasibility differs; ADAGEN instead helps
    when infeasibility is *tied* and objective improvements are pulling the
    search sideways into a local minimum it can't escape by chasing
    Cost.as_scalar() with a fixed multiplier alone.

    Not wired into lahc.py -- there's no stagnation-tracking acceptance loop
    yet (Etap 5). This is the multiplier such a loop would call
    `.multiplier(stagnant_iterations)` on every step, or `.scalar(cost, n)`
    as a drop-in replacement for `cost.as_scalar()`.
    """

    base_multiplier: int = INFEASIBILITY_WEIGHT
    growth_per_stagnant_step: float = 0.05
    max_multiplier: int = INFEASIBILITY_WEIGHT * 1_000

    def multiplier(self, stagnant_iterations: int) -> int:
        growth = 1.0 + self.growth_per_stagnant_step
        grown = self.base_multiplier * growth**stagnant_iterations
        return min(int(grown), self.max_multiplier)

    def scalar(self, cost: Cost, stagnant_iterations: int) -> int:
        return cost.as_scalar(self.multiplier(stagnant_iterations))
