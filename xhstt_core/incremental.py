"""Incremental (delta) cost evaluation with a persistent, transactional state.

`xhstt_core.delta.delta_cost` is a *filtered full re-evaluation*: it decides
which constraints a move can possibly have affected, then re-runs each of
them over its entire scope -- twice, once for the old solution and once for
the new. On real instances that loses to plain full evaluation, because the
few constraints that dominate evaluation time (one AvoidClashesConstraint
over every teacher, one SpreadEventsConstraint over every event group) are
also the ones almost any move touches.

This module fixes that by going one level finer. The spec evaluates a
constraint at each of its *points of application* independently -- one
resource, one event, one event group, one event pair -- and the total cost
is the sum over points. So the per-point costs can be cached, and a move
only invalidates the handful of points that actually see different data.
Moving one event re-scores 2-4 teachers instead of all of them.

Two consequences worth stating explicitly:

- The cost function is applied per point, to that point's *deviation*, never
  to a change in deviation: `Quadratic`/`Step` make `f(a+b) != f(a)+f(b)`,
  so the update is always `f(new deviation) - f(old deviation)` at one
  point.
- State is transactional. `evaluate()` mutates in place and returns a
  `Transaction`; the caller then `accept()`s it or `rollback()`s it. A
  rejected LAHC candidate therefore costs one delta and one journal replay,
  not a second evaluation -- and a heuristic scoring every valid time for
  one event pays one full evaluation plus N deltas instead of N full
  evaluations.

`evaluator_ref` stays the independent, HSEval-verified oracle: it is not
used by the hot path here, and the invariant tests compare this module's
per-constraint vector against it.
"""

from collections.abc import Hashable
from dataclasses import dataclass, field
from math import ceil
from typing import Any

from xhstt_core.cost import Cost
from xhstt_core.evaluation_index import (
    ConstraintPoints,
    InstanceIndex,
    PointId,
    PointKey,
    instance_index,
)
from xhstt_core.evaluator_ref import (
    Occurrence,
    _assigned_resource,
    _event_group_members,
    _event_resource_workload,
    _shortfall_or_excess,
    apply_cost_function,
    resolve_occurrence,
    resolve_occurrences,
)
from xhstt_core.model import Instance, Solution

_UNBOUNDED = 10**9


class StructuralChangeError(Exception):
    """Raised when a candidate solution cannot be reached from the current
    state by re-resolving individual entries -- a different number of
    solution events, or a different event at some position. Incremental
    updates are undefined there; the caller must fall back to a full
    rebuild."""


# Every incrementally maintained aggregate is a dict of ints, which is what
# lets one undo journal cover all of them -- and means a journalled `None`
# unambiguously records "this key was absent", with no sentinel needed.
Table = dict[Any, int]
JournalEntry = tuple[Table, Hashable, int | None]


@dataclass
class Transaction:
    """An applied-but-not-yet-final state change, with everything needed to
    undo it. Produced by `IncrementalEvaluator.evaluate`; the state already
    reflects the candidate when it is returned."""

    cost: Cost
    previous_solution: Solution
    previous_infeasibility: int
    previous_objective: int
    journal: list[JournalEntry] = field(default_factory=list)
    occurrence_journal: list[tuple[int, Occurrence]] = field(default_factory=list)
    open: bool = True


def _span(index: InstanceIndex, time_ref: str | None, duration: int) -> tuple[str, ...]:
    """The time ids an occurrence occupies. Deliberately reproduces
    `evaluator_ref._occupied_time_ids`, including its truncation at the end
    of the time list (a preassigned time near the end can legitimately
    overflow)."""
    if time_ref is None:
        return ()
    start = index.time_positions[time_ref]
    return index.time_ids[start : start + duration]


class IncrementalEvaluator:
    """Maintains the cost of one solution and updates it in place as moves
    are applied. Single-threaded by design, like the reference evaluator."""

    def __init__(self, instance: Instance, solution: Solution) -> None:
        self._instance = instance
        self._index = instance_index(instance)
        self.rebuild(solution)

    # ---------------------------------------------------------------- state

    def rebuild(self, solution: Solution) -> None:
        """Discards all incremental state and recomputes everything from
        scratch. The safety hatch for structural changes and for heuristics
        that aren't known to preserve the solution's shape."""
        index = self._index
        self._solution = solution
        self._occurrences = resolve_occurrences(self._instance, solution)

        occs_by_event: dict[str, list[int]] = {}
        for k, occurrence in enumerate(self._occurrences):
            occs_by_event.setdefault(occurrence.event_ref, []).append(k)
        self._occs_by_event: dict[str, tuple[int, ...]] = {
            k: tuple(v) for k, v in occs_by_event.items()
        }
        self._occ_of_solution_event = self._map_solution_events(solution)

        self._busy: dict[str, dict[str, int]] = {}
        self._occs_by_resource: dict[str, dict[int, int]] = {}
        self._journal: list[JournalEntry] = []
        for k in range(len(self._occurrences)):
            self._apply_occupancy(k, +1)
        self._journal = []

        self._point_cost: dict[PointKey, int] = {}
        self._infeasibility = 0
        self._objective = 0
        for cp in index.constraint_points:
            for point in cp.points:
                cost = self._point_cost_of(cp, point)
                if cost == 0:
                    continue
                self._point_cost[(cp.index, point)] = cost
                if cp.constraint.required:
                    self._infeasibility += cost
                else:
                    self._objective += cost

    def _map_solution_events(self, solution: Solution) -> tuple[int, ...]:
        """Occurrence index for each position in `solution.events`. Mirrors
        `resolve_occurrences`' ordering: instance events in instance order,
        each followed by its solution events in solution order (or one
        synthesized occurrence when it has none)."""
        positions_by_ref: dict[str, list[int]] = {}
        for position, se in enumerate(solution.events):
            positions_by_ref.setdefault(se.event_ref, []).append(position)

        mapping = [0] * len(solution.events)
        occurrence = 0
        for event_def in self._instance.events:
            matching = positions_by_ref.get(event_def.id)
            if not matching:
                occurrence += 1
                continue
            for position in matching:
                mapping[position] = occurrence
                occurrence += 1
        return tuple(mapping)

    @property
    def solution(self) -> Solution:
        return self._solution

    @property
    def occurrences(self) -> list[Occurrence]:
        return self._occurrences

    @property
    def cost(self) -> Cost:
        return Cost(self._infeasibility, self._objective)

    def constraint_costs(self) -> tuple[int, ...]:
        """Per-constraint cost vector, comparable with
        `evaluator_ref.evaluate_constraint_costs`. Diagnostic only -- it
        rebuilds the per-constraint sums from the per-point cache."""
        totals = [0] * len(self._instance.constraints)
        for (constraint_index, _), cost in self._point_cost.items():
            totals[constraint_index] += cost
        return tuple(totals)

    # ------------------------------------------------------------ mutation

    def _write(self, table: Table, key: Hashable, value: int) -> None:
        self._journal.append((table, key, table.get(key)))
        table[key] = value

    def _bump(self, table: Table, key: Hashable, delta: int) -> None:
        previous = table.get(key)
        self._journal.append((table, key, previous))
        table[key] = (0 if previous is None else previous) + delta

    def _apply_occupancy(self, k: int, sign: int) -> None:
        occurrence = self._occurrences[k]
        span = _span(self._index, occurrence.time_ref, occurrence.duration)
        for _, resource_ref in occurrence.resource_assignments:
            if resource_ref is None:
                continue
            self._bump(self._occs_by_resource.setdefault(resource_ref, {}), k, sign)
            busy = self._busy.setdefault(resource_ref, {})
            for time_id in span:
                self._bump(busy, time_id, sign)

    def busy_times(self, resource_ref: str) -> set[str]:
        return {t for t, count in self._busy.get(resource_ref, {}).items() if count > 0}

    # ---------------------------------------------------------- transaction

    def evaluate(self, candidate: Solution) -> Transaction:
        """Applies `candidate` to the state and returns its cost together
        with the means to undo it. The caller MUST finish the transaction
        with `accept` or `rollback` before starting another one."""
        transaction = Transaction(
            cost=self.cost,
            previous_solution=self._solution,
            previous_infeasibility=self._infeasibility,
            previous_objective=self._objective,
        )
        self._journal = transaction.journal

        try:
            changed = self._changed_occurrences(candidate)
        except StructuralChangeError:
            self._journal = []
            raise

        touched_events: set[str] = set()
        touched_resources: set[str] = set()
        for k, new_occurrence in changed:
            old_occurrence = self._occurrences[k]
            touched_events.add(old_occurrence.event_ref)
            touched_resources.update(
                r for _, r in old_occurrence.resource_assignments if r is not None
            )
            self._apply_occupancy(k, -1)
            transaction.occurrence_journal.append((k, old_occurrence))
            self._occurrences[k] = new_occurrence
            self._apply_occupancy(k, +1)
            touched_resources.update(
                r for _, r in new_occurrence.resource_assignments if r is not None
            )

        self._rescore(touched_events, touched_resources)
        self._solution = candidate
        transaction.cost = self.cost
        self._journal = []
        return transaction

    def _changed_occurrences(self, candidate: Solution) -> list[tuple[int, Occurrence]]:
        current = self._solution.events
        if len(candidate.events) != len(current):
            raise StructuralChangeError(
                "candidate has a different number of solution events"
            )

        changed: list[tuple[int, Occurrence]] = []
        for position, new_event in enumerate(candidate.events):
            old_event = current[position]
            # Manual moves rebuild only the entries they touch
            # (dataclasses.replace + structural sharing), so identity rules
            # out the overwhelming majority in one cheap pass.
            if old_event is new_event:
                continue
            if old_event.event_ref != new_event.event_ref:
                raise StructuralChangeError(
                    f"candidate changes the event at position {position}"
                )
            if old_event == new_event:
                continue
            event_def = self._index.events_by_id[new_event.event_ref]
            occurrence = resolve_occurrence(event_def, new_event)
            k = self._occ_of_solution_event[position]
            if occurrence != self._occurrences[k]:
                changed.append((k, occurrence))
        return changed

    def _rescore(self, touched_events: set[str], touched_resources: set[str]) -> None:
        index = self._index
        dirty: set[PointKey] = set()
        for event_id in touched_events:
            dirty.update(index.points_for_event.get(event_id, ()))
        for resource_ref in touched_resources:
            dirty.update(index.points_for_resource.get(resource_ref, ()))

        for key in dirty:
            constraint_index, point = key
            cp = index.constraint_points[constraint_index]
            new_cost = self._point_cost_of(cp, point)
            old_cost = self._point_cost.get(key, 0)
            if new_cost == old_cost:
                continue
            self._write(self._point_cost, key, new_cost)
            if cp.constraint.required:
                self._infeasibility += new_cost - old_cost
            else:
                self._objective += new_cost - old_cost

    def _point_cost_of(self, cp: ConstraintPoints, point: PointId) -> int:
        deviation = _DEVIATIONS[cp.constraint.type](self, cp, point)
        return cp.constraint.weight * apply_cost_function(
            cp.constraint.cost_function, deviation
        )

    def accept(self, transaction: Transaction) -> None:
        transaction.open = False

    def rollback(self, transaction: Transaction) -> None:
        if not transaction.open:
            raise ValueError("transaction is already finished")
        for k, occurrence in reversed(transaction.occurrence_journal):
            self._occurrences[k] = occurrence
        for table, key, previous in reversed(transaction.journal):
            if previous is None:
                table.pop(key, None)
            else:
                table[key] = previous
        self._solution = transaction.previous_solution
        self._infeasibility = transaction.previous_infeasibility
        self._objective = transaction.previous_objective
        transaction.open = False

    # ------------------------------------------------------ convenience API

    def probe(self, candidate: Solution) -> Cost:
        """Cost of `candidate`, leaving the state untouched. Falls back to a
        throwaway full evaluation if the candidate isn't structurally
        compatible."""
        try:
            transaction = self.evaluate(candidate)
        except StructuralChangeError:
            return IncrementalEvaluator(self._instance, candidate).cost
        cost = transaction.cost
        self.rollback(transaction)
        return cost

    def commit(self, candidate: Solution) -> Cost:
        """Cost of `candidate`, keeping it as the new current state."""
        try:
            transaction = self.evaluate(candidate)
        except StructuralChangeError:
            self.rebuild(candidate)
            return self.cost
        self.accept(transaction)
        return transaction.cost


# --------------------------------------------------------------- deviations
#
# One function per constraint type, each computing the deviation at ONE point
# of application. Every one of them is a restriction of the corresponding
# `evaluator_ref._evaluate_*_constraint` loop body to a single point, and the
# invariant tests compare the resulting per-constraint vector against that
# reference over long chains of real moves.


def _event_occurrences(ev: IncrementalEvaluator, event_id: str) -> list[Occurrence]:
    return [ev._occurrences[k] for k in ev._occs_by_event.get(event_id, ())]


def _group_occurrences(ev: IncrementalEvaluator, group_ref: str) -> list[Occurrence]:
    members = _event_group_members(ev._instance, group_ref)
    return [o for event_id in members for o in _event_occurrences(ev, event_id)]


def _bounds(cp: ConstraintPoints) -> tuple[int, int]:
    params = cp.constraint.params
    return int(params.get("Minimum", 0)), int(params.get("Maximum", _UNBOUNDED))


def _role(cp: ConstraintPoints) -> str | None:
    role = cp.constraint.params.get("Role")
    return role if isinstance(role, str) else None


def _assign_time(ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId) -> int:
    return sum(
        o.duration for o in _event_occurrences(ev, str(point)) if o.time_ref is None
    )


def _avoid_clashes(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    return sum(
        count - 1 for count in ev._busy.get(str(point), {}).values() if count > 1
    )


def _assign_resource(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    role = _role(cp)
    return sum(
        o.duration
        for o in _event_occurrences(ev, str(point))
        if _assigned_resource(o, role) is None
    )


def _prefer_resources(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    role = _role(cp)
    deviation = 0
    for o in _event_occurrences(ev, str(point)):
        assigned = _assigned_resource(o, role)
        if assigned is not None and assigned not in cp.preferred_resources:
            deviation += o.duration
    return deviation


def _cluster_busy_times(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    minimum, maximum = _bounds(cp)
    busy = ev.busy_times(str(point))
    active = {g for t in busy for g in ev._index.groups_of_time.get(t, frozenset())}
    return _shortfall_or_excess(
        len(active & cp.referenced_time_groups), minimum, maximum
    )


def _avoid_unavailable_times(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    return len(ev.busy_times(str(point)) & cp.referenced_times)


def _idle_count_in_group(
    ev: IncrementalEvaluator, busy: set[str], group_ref: str
) -> int:
    group_times = ev._index.times_of_group.get(group_ref, ())
    busy_indices = [i for i, t in enumerate(group_times) if t in busy]
    if len(busy_indices) < 2:
        return 0
    first, last = min(busy_indices), max(busy_indices)
    return sum(1 for t in group_times[first + 1 : last] if t not in busy)


def _limit_idle_times(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    minimum, maximum = _bounds(cp)
    busy = ev.busy_times(str(point))
    idle_total = sum(
        _idle_count_in_group(ev, busy, g) for g in cp.referenced_time_groups
    )
    return _shortfall_or_excess(idle_total, minimum, maximum)


def _limit_busy_times(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    minimum, maximum = _bounds(cp)
    busy = ev.busy_times(str(point))
    deviation = 0
    for group_ref in cp.referenced_time_groups:
        # A group the resource is not busy in at all contributes 0 even when
        # that is below Minimum (spec) -- so this must stay a `!= 0` guard,
        # not a max/min clamp.
        count = len(busy.intersection(ev._index.times_of_group.get(group_ref, ())))
        if count != 0:
            deviation += _shortfall_or_excess(count, minimum, maximum)
    return deviation


def _limit_workload(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    minimum, maximum = _bounds(cp)
    resource_ref = str(point)
    workload_sum = 0
    for k in ev._occs_by_resource.get(resource_ref, {}):
        o = ev._occurrences[k]
        event_def = ev._index.events_by_id[o.event_ref]
        for role, assigned in o.resource_assignments:
            if assigned != resource_ref:
                continue
            workload = _event_resource_workload(event_def, role)
            # Rounded per term before summing (spec, verbatim).
            workload_sum += ceil(workload * (o.duration / event_def.duration))
    return _shortfall_or_excess(workload_sum, minimum, maximum)


def _split_events(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    params = cp.constraint.params
    min_duration = int(params.get("MinimumDuration", 0))
    max_duration = int(params.get("MaximumDuration", _UNBOUNDED))
    min_amount = int(params.get("MinimumAmount", 0))
    max_amount = int(params.get("MaximumAmount", _UNBOUNDED))
    subs = _event_occurrences(ev, str(point))
    amount_deviation = _shortfall_or_excess(len(subs), min_amount, max_amount)
    duration_deviation = sum(
        1 for o in subs if not (min_duration <= o.duration <= max_duration)
    )
    return amount_deviation + duration_deviation


def _distribute_split_events(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    minimum, maximum = _bounds(cp)
    target_duration = int(cp.constraint.params["Duration"])
    count = sum(
        1 for o in _event_occurrences(ev, str(point)) if o.duration == target_duration
    )
    return _shortfall_or_excess(count, minimum, maximum)


def _prefer_times(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    duration_filter = cp.constraint.params.get("Duration")
    subs = _event_occurrences(ev, str(point))
    if duration_filter is not None:
        subs = [o for o in subs if o.duration == int(duration_filter)]
    return sum(
        o.duration
        for o in subs
        if o.time_ref is not None and o.time_ref not in cp.preferred_times
    )


def _spread_events(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    subs = _group_occurrences(ev, str(point))
    groups_of_time = ev._index.groups_of_time
    deviation = 0
    for entry in cp.constraint.params.get("TimeGroups", []):
        group_ref = entry["reference"]
        minimum = int(entry.get("Minimum", 0))
        maximum = int(entry.get("Maximum", _UNBOUNDED))
        count = sum(
            1
            for o in subs
            if o.time_ref is not None
            and group_ref in groups_of_time.get(o.time_ref, frozenset())
        )
        deviation += _shortfall_or_excess(count, minimum, maximum)
    return deviation


def _avoid_split_assignments(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    role = _role(cp)
    assigned = {
        _assigned_resource(o, role)
        for o in _group_occurrences(ev, str(point))
        if _assigned_resource(o, role) is not None
    }
    return max(0, len(assigned) - 1)


def _link_events(ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId) -> int:
    per_event_times = []
    for event_id in _event_group_members(ev._instance, str(point)):
        times_for_event: set[str] = set()
        for o in _event_occurrences(ev, event_id):
            times_for_event.update(_span(ev._index, o.time_ref, o.duration))
        per_event_times.append(times_for_event)
    if not per_event_times:
        return 0
    all_times = set().union(*per_event_times)
    return sum(1 for t in all_times if not all(t in s for s in per_event_times))


def _first_and_last(
    ev: IncrementalEvaluator, event_id: str
) -> tuple[int | None, int | None]:
    positions = ev._index.time_positions
    matching = _event_occurrences(ev, event_id)
    if not matching or any(o.time_ref is None for o in matching):
        return None, None
    starts = [positions[o.time_ref] for o in matching if o.time_ref is not None]
    ends = [
        positions[o.time_ref] + o.duration - 1
        for o in matching
        if o.time_ref is not None
    ]
    return min(starts), max(ends)


def _order_events(
    ev: IncrementalEvaluator, cp: ConstraintPoints, point: PointId
) -> int:
    pair = cp.constraint.applies_to.event_pairs[int(point)]
    _, first_last = _first_and_last(ev, pair.first_event)
    second_first, _ = _first_and_last(ev, pair.second_event)
    if first_last is None or second_first is None:
        return 0
    separation = second_first - first_last - 1
    maximum = pair.max_separation if pair.max_separation is not None else _UNBOUNDED
    return _shortfall_or_excess(separation, pair.min_separation, maximum)


_DEVIATIONS: dict[str, Any] = {
    "AssignTimeConstraint": _assign_time,
    "AvoidClashesConstraint": _avoid_clashes,
    "AssignResourceConstraint": _assign_resource,
    "PreferResourcesConstraint": _prefer_resources,
    "ClusterBusyTimesConstraint": _cluster_busy_times,
    "AvoidUnavailableTimesConstraint": _avoid_unavailable_times,
    "LimitIdleTimesConstraint": _limit_idle_times,
    "LimitBusyTimesConstraint": _limit_busy_times,
    "LimitWorkloadConstraint": _limit_workload,
    "SplitEventsConstraint": _split_events,
    "DistributeSplitEventsConstraint": _distribute_split_events,
    "PreferTimesConstraint": _prefer_times,
    "SpreadEventsConstraint": _spread_events,
    "AvoidSplitAssignmentsConstraint": _avoid_split_assignments,
    "LinkEventsConstraint": _link_events,
    "OrderEventsConstraint": _order_events,
}
