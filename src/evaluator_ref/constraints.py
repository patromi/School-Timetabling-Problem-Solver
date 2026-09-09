import math
from collections import Counter
from collections.abc import Callable

from src.evaluator_ref._cache import (
    _event_group_members,
    _referenced_ids,
    _shortfall_or_excess,
    _time_positions,
    apply_cost_function,
)
from src.evaluator_ref.occurrences import (
    Occurrence,
    _assigned_resource,
    _assigned_resource_ids,
    _events_in_applies_to,
    _full_span_busy_times,
    _get_current_occupancy_index,
    _occupied_time_ids,
    _resources_in_applies_to,
)
from src.model import Constraint, Event, Instance


def _evaluate_assign_time_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Deviation is the summed *duration* of unassigned sub-events, not a
    # flat count of 1 per event (Kristiansen et al. 2015, §3.2.3).
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    deviation = sum(
        o.duration
        for o in occurrences
        if o.event_ref in event_ids and o.time_ref is None
    )
    return constraint.weight * apply_cost_function(constraint.cost_function, deviation)


def _evaluate_avoid_clashes_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = sum, over all times
    # the resource is preassigned/assigned to >=2 occurrences (counting
    # every time slot each occurrence's duration spans, not just its
    # start), of (count - 1).
    total = 0
    current_index = _get_current_occupancy_index()
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        counter: Counter[str]
        if current_index is not None:
            counter = current_index.get(resource_id, Counter())
        else:
            counter = Counter()
            for o in occurrences:
                if o.time_ref is None or resource_id not in _assigned_resource_ids(o):
                    continue
                for t in _occupied_time_ids(instance, o.time_ref, o.duration):
                    counter[t] += 1
        deviation = sum(count - 1 for count in counter.values() if count > 1)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _evaluate_assign_resource_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event resource (an event's unpreassigned
    # slot for the constraint's Role). Deviation = summed duration of that
    # event's sub-events still missing a resource for that role.
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    role = constraint.params.get("Role")
    total = 0
    for event in instance.events:
        if event.id not in event_ids:
            continue
        is_point_of_application = any(
            er.role == role and er.resource_ref is None for er in event.resources
        )
        if not is_point_of_application:
            continue
        deviation = sum(
            o.duration
            for o in occurrences
            if o.event_ref == event.id and _assigned_resource(o, role) is None
        )
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _preferred_resource_ids(instance: Instance, constraint: Constraint) -> set[str]:
    ids = {entry["reference"] for entry in constraint.params.get("Resources", [])}
    group_ids = {
        entry["reference"] for entry in constraint.params.get("ResourceGroups", [])
    }
    if group_ids:
        ids.update(r.id for r in instance.resources if group_ids & set(r.group_refs))
    return ids


def _evaluate_prefer_resources_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event resource (an event's slot for the
    # constraint's Role). Deviation = summed duration of sub-events whose
    # assigned resource for that role isn't among the preferred resources.
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    role = constraint.params.get("Role")
    preferred = _preferred_resource_ids(instance, constraint)
    total = 0
    for event in instance.events:
        if event.id not in event_ids:
            continue
        if not any(er.role == role for er in event.resources):
            continue
        deviation = sum(
            o.duration
            for o in occurrences
            if o.event_ref == event.id
            and _assigned_resource(o, role) is not None
            and _assigned_resource(o, role) not in preferred
        )
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _time_group_refs(instance: Instance, time_ref: str | None) -> set[str]:
    if time_ref is None:
        return set()
    time = next((t for t in instance.times if t.id == time_ref), None)
    return set(time.group_refs) if time else set()


def _evaluate_cluster_busy_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = shortfall/excess of
    # the number of the constraint's TimeGroups the resource is "active" in
    # (busy at >=1 time belonging to that group, over the full span of
    # every occurrence), summed first, thresholded once against
    # Minimum/Maximum.
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy = _full_span_busy_times(instance, occurrences, resource_id)
        active_groups = {g for t in busy for g in _time_group_refs(instance, t)}
        deviation = _shortfall_or_excess(
            len(active_groups & target_groups), minimum, maximum
        )
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _evaluate_avoid_unavailable_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = count of the
    # constraint's unavailable Times during which the resource attends
    # >=1 solution event (over the full span of every occurrence).
    unavailable_times = _referenced_ids(constraint.params.get("Times"))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy_times = _full_span_busy_times(instance, occurrences, resource_id)
        deviation = len(busy_times & unavailable_times)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _idle_count_in_group(instance: Instance, busy: set[str], group_ref: str) -> int:
    # Times are assumed consecutive in file order (XHSTT convention). A time
    # is idle if the resource is busy at an earlier AND a later time within
    # the same group -- i.e. it's a non-busy gap strictly between the
    # group's first and last busy time.
    group_times = [t.id for t in instance.times if group_ref in t.group_refs]
    busy_indices = [i for i, t in enumerate(group_times) if t in busy]
    if len(busy_indices) < 2:
        return 0
    first, last = min(busy_indices), max(busy_indices)
    return sum(1 for t in group_times[first + 1 : last] if t not in busy)


def _evaluate_limit_idle_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = shortfall/excess of
    # the summed idle-time count (across the constraint's TimeGroups)
    # against Minimum/Maximum.
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy = _full_span_busy_times(instance, occurrences, resource_id)
        idle_total = sum(_idle_count_in_group(instance, busy, g) for g in target_groups)
        deviation = _shortfall_or_excess(idle_total, minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _busy_count_in_group(instance: Instance, busy: set[str], group_ref: str) -> int:
    group_times = {t.id for t in instance.times if group_ref in t.group_refs}
    return len(busy & group_times)


def _evaluate_limit_busy_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = the SUM, over all the
    # constraint's TimeGroups, of the shortfall/excess of busy-count against
    # Minimum/Maximum in that group (a group with 0 busy times always
    # contributes 0, even if that's below Minimum) -- ONE deviation per
    # resource, CostFunction applied once to the sum (spec, verbatim).
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy = _full_span_busy_times(instance, occurrences, resource_id)
        deviation = 0
        for group_ref in target_groups:
            count = _busy_count_in_group(instance, busy, group_ref)
            if count != 0:
                deviation += _shortfall_or_excess(count, minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EvaluatorFn = Callable[["Instance", "list[Occurrence]", "Constraint"], int]
_EVALUATORS: dict[str, _EvaluatorFn] = {
    "AssignTimeConstraint": _evaluate_assign_time_constraint,
    "AvoidClashesConstraint": _evaluate_avoid_clashes_constraint,
    "AssignResourceConstraint": _evaluate_assign_resource_constraint,
    "PreferResourcesConstraint": _evaluate_prefer_resources_constraint,
    "ClusterBusyTimesConstraint": _evaluate_cluster_busy_times_constraint,
    "AvoidUnavailableTimesConstraint": _evaluate_avoid_unavailable_times_constraint,
    "LimitIdleTimesConstraint": _evaluate_limit_idle_times_constraint,
    "LimitBusyTimesConstraint": _evaluate_limit_busy_times_constraint,
}


def _event_resource_workload(event_def: Event, role: str) -> int:
    # Chain (spec, verbatim): EventResource.Workload if present, else the
    # enclosing Event's Workload if present, else the Event's Duration.
    er = next((x for x in event_def.resources if x.role == role), None)
    if er is not None and er.workload is not None:
        return er.workload
    if event_def.workload is not None:
        return event_def.workload
    return event_def.duration


def _evaluate_limit_workload_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = shortfall/excess of
    # total assigned workload against Minimum/Maximum. Workload(sr) =
    # Duration(se) * Workload(er) / Duration(e) for each solution resource
    # sr, EACH rounded up to an integer before being summed (spec,
    # verbatim: "Each amount is rounded up to the next integer before
    # being added to the sum").
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    events_by_id = {e.id: e for e in instance.events}
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        workload_sum = 0
        for o in occurrences:
            event_def = events_by_id[o.event_ref]
            for role, assigned in o.resource_assignments:
                if assigned != resource_id:
                    continue
                workload = _event_resource_workload(event_def, role)
                workload_sum += math.ceil(workload * (o.duration / event_def.duration))
        deviation = _shortfall_or_excess(workload_sum, minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["LimitWorkloadConstraint"] = _evaluate_limit_workload_constraint


def _evaluate_split_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one instance event. Deviation = (shortfall/
    # excess of sub-event count against Min/MaximumAmount) + (count of
    # sub-events whose duration falls outside [MinimumDuration,
    # MaximumDuration]).
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    min_duration = int(constraint.params.get("MinimumDuration", 0))
    max_duration = int(constraint.params.get("MaximumDuration", 10**9))
    min_amount = int(constraint.params.get("MinimumAmount", 0))
    max_amount = int(constraint.params.get("MaximumAmount", 10**9))
    total = 0
    for event in instance.events:
        if event.id not in event_ids:
            continue
        subs = [o for o in occurrences if o.event_ref == event.id]
        amount_deviation = _shortfall_or_excess(len(subs), min_amount, max_amount)
        duration_deviation = sum(
            1 for o in subs if not (min_duration <= o.duration <= max_duration)
        )
        deviation = amount_deviation + duration_deviation
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["SplitEventsConstraint"] = _evaluate_split_events_constraint


def _evaluate_distribute_split_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one instance event. Deviation = shortfall/excess
    # of (count of sub-events whose duration == Duration) against Min/Max.
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    target_duration = int(constraint.params["Duration"])
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    total = 0
    for event in instance.events:
        if event.id not in event_ids:
            continue
        count = sum(
            1
            for o in occurrences
            if o.event_ref == event.id and o.duration == target_duration
        )
        deviation = _shortfall_or_excess(count, minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["DistributeSplitEventsConstraint"] = (
    _evaluate_distribute_split_events_constraint
)


def _preferred_time_ids(instance: Instance, constraint: Constraint) -> set[str]:
    ids = _referenced_ids(constraint.params.get("Times"))
    group_ids = _referenced_ids(constraint.params.get("TimeGroups"))
    if group_ids:
        ids.update(t.id for t in instance.times if group_ids & set(t.group_refs))
    return ids


def _evaluate_prefer_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event (not event resource -- differs from
    # PreferResourcesConstraint's granularity). Deviation = summed duration
    # of sub-events assigned a time outside the preferred TimeGroups/Times.
    # If a `Duration` param is present, only sub-events of that exact
    # duration are considered.
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    preferred = _preferred_time_ids(instance, constraint)
    duration_filter = constraint.params.get("Duration")
    total = 0
    for event in instance.events:
        if event.id not in event_ids:
            continue
        subs = [o for o in occurrences if o.event_ref == event.id]
        if duration_filter is not None:
            subs = [o for o in subs if o.duration == int(duration_filter)]
        deviation = sum(
            o.duration
            for o in subs
            if o.time_ref is not None and o.time_ref not in preferred
        )
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["PreferTimesConstraint"] = _evaluate_prefer_times_constraint


def _evaluate_spread_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event group (individual events in AppliesTo
    # are not points of application for this constraint). Each TimeGroups
    # entry carries its own Minimum/Maximum. Deviation = sum, over those
    # time groups, of the shortfall/excess of sub-events *starting* in that
    # time group.
    total = 0
    for group_ref in constraint.applies_to.event_groups:
        member_ids = _event_group_members(instance, group_ref)
        subs = [o for o in occurrences if o.event_ref in member_ids]
        deviation = 0
        for entry in constraint.params.get("TimeGroups", []):
            tg_ref = entry["reference"]
            minimum = int(entry.get("Minimum", 0))
            maximum = int(entry.get("Maximum", 10**9))
            count = sum(
                1
                for o in subs
                if o.time_ref is not None
                and tg_ref in _time_group_refs(instance, o.time_ref)
            )
            deviation += _shortfall_or_excess(count, minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["SpreadEventsConstraint"] = _evaluate_spread_events_constraint


def _evaluate_avoid_split_assignments_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event group. Deviation = amount by which
    # the number of distinct resources assigned (matching Role) across the
    # group's events' sub-events exceeds 1.
    role = constraint.params.get("Role")
    total = 0
    for group_ref in constraint.applies_to.event_groups:
        member_ids = _event_group_members(instance, group_ref)
        assigned = {
            _assigned_resource(o, role)
            for o in occurrences
            if o.event_ref in member_ids and _assigned_resource(o, role) is not None
        }
        deviation = max(0, len(assigned) - 1)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["AvoidSplitAssignmentsConstraint"] = (
    _evaluate_avoid_split_assignments_constraint
)


def _evaluate_link_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event group (individual events are not
    # points of application). For each instance event in the group, build
    # the set of all times its sub-events occupy (not just start times).
    # Deviation = count of times appearing in >=1 of those sets but not all.
    total = 0
    for group_ref in constraint.applies_to.event_groups:
        member_ids = _event_group_members(instance, group_ref)
        per_event_times = []
        for event_id in member_ids:
            times_for_event = set()
            for o in occurrences:
                if o.event_ref == event_id:
                    times_for_event |= _occupied_time_ids(
                        instance, o.time_ref, o.duration
                    )
            per_event_times.append(times_for_event)
        all_times = set().union(*per_event_times) if per_event_times else set()
        deviation = sum(
            1 for t in all_times if not all(t in s for s in per_event_times)
        )
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["LinkEventsConstraint"] = _evaluate_link_events_constraint


def _first_and_last_occupied_indices(
    instance: Instance, occurrences: list[Occurrence], event_id: str
) -> tuple[int | None, int | None]:
    # Spec: deviation is 0 if the event has NO solution events, OR if ANY
    # of its solution events has an unassigned time -- so a mix of
    # assigned/unassigned sub-events must also yield (None, None), not be
    # silently computed from only the assigned ones.
    positions = _time_positions(instance)
    matching = [o for o in occurrences if o.event_ref == event_id]
    if not matching or any(o.time_ref is None for o in matching):
        return None, None
    starts = [positions[o.time_ref] for o in matching]
    ends = [positions[o.time_ref] + o.duration - 1 for o in matching]
    return min(starts), max(ends)


def _evaluate_order_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event pair. Deviation = shortfall/excess of
    # the gap between the end of the first event's latest sub-event and the
    # start of the second event's earliest sub-event, against
    # MinSeparation/MaxSeparation; 0 if either event has zero solution
    # events or any solution event with an unassigned time.
    total = 0
    for pair in constraint.applies_to.event_pairs:
        _, first_last = _first_and_last_occupied_indices(
            instance, occurrences, pair.first_event
        )
        second_first, _ = _first_and_last_occupied_indices(
            instance, occurrences, pair.second_event
        )
        if first_last is None or second_first is None:
            continue
        separation = second_first - first_last - 1
        maximum = pair.max_separation if pair.max_separation is not None else 10**9
        deviation = _shortfall_or_excess(separation, pair.min_separation, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["OrderEventsConstraint"] = _evaluate_order_events_constraint


def evaluate_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    try:
        evaluator = _EVALUATORS[constraint.type]
    except KeyError:
        raise NotImplementedError(
            f"no evaluator implemented yet for {constraint.type!r} "
            f"(constraint id={constraint.id!r}) — see task to verify its "
            f"exact HSEval deviation formula before implementing"
        ) from None
    return evaluator(instance, occurrences, constraint)
