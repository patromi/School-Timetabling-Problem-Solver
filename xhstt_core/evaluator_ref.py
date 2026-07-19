import math
from collections import Counter
from dataclasses import dataclass, field

from xhstt_core.model import AppliesTo, Constraint, Instance, Solution


@dataclass
class Occurrence:
    """One concrete meeting of an Event — one per SolutionEvent entry, since
    a split event occurs multiple times under the same event_ref."""

    event_ref: str
    duration: int
    time_ref: str | None
    resource_assignments: dict[str, str | None] = field(default_factory=dict)


def _shortfall_or_excess(value: int, minimum: int, maximum: int) -> int:
    if value < minimum:
        return minimum - value
    if value > maximum:
        return value - maximum
    return 0


def _referenced_ids(entries: list[dict] | None) -> set[str]:
    return {e["reference"] for e in (entries or [])}


def apply_cost_function(name: str, deviation: int) -> int:
    if name == "Linear":
        return deviation
    if name == "Quadratic":
        return deviation * deviation
    if name == "Step":
        return 1 if deviation != 0 else 0
    raise ValueError(f"unknown XHSTT cost function: {name!r}")


def resolve_occurrences(instance: Instance, solution: Solution) -> list[Occurrence]:
    """Merges each Event's fixed resource assignments (declared in the
    Instance) with the resources chosen by the Solution for roles that were
    left unassigned — the two are complementary, never overlapping, per the
    XHSTT format."""
    events_by_id = {e.id: e for e in instance.events}
    occurrences = []
    for se in solution.events:
        event_def = events_by_id[se.event_ref]
        assignments = {er.role: er.resource_ref for er in event_def.resources}
        assignments.update({sr.role: sr.resource_ref for sr in se.resources})
        occurrences.append(
            Occurrence(
                event_ref=se.event_ref,
                duration=se.duration if se.duration is not None else event_def.duration,
                time_ref=se.time_ref,
                resource_assignments=assignments,
            )
        )
    return occurrences


def _events_in_applies_to(instance: Instance, applies_to: AppliesTo) -> set[str]:
    ids = set(applies_to.events)
    if applies_to.event_groups:
        groups = set(applies_to.event_groups)
        ids.update(e.id for e in instance.events if groups & set(e.group_refs))
    return ids


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


def _resources_in_applies_to(instance: Instance, applies_to: AppliesTo) -> set[str]:
    ids = set(applies_to.resources)
    if applies_to.resource_groups:
        groups = set(applies_to.resource_groups)
        ids.update(r.id for r in instance.resources if groups & set(r.group_refs))
    return ids


def _evaluate_avoid_clashes_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = sum, over all times
    # the resource is assigned to >=2 occurrences, of (count - 1).
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        times_used = [
            o.time_ref
            for o in occurrences
            if o.time_ref is not None
            and resource_id in o.resource_assignments.values()
        ]
        deviation = sum(count - 1 for count in Counter(times_used).values() if count > 1)
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
            if o.event_ref == event.id and o.resource_assignments.get(role) is None
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
            and o.resource_assignments.get(role) is not None
            and o.resource_assignments.get(role) not in preferred
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
    # (busy at >=1 time belonging to that group), summed first, thresholded
    # once against Minimum/Maximum.
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        active_groups = set()
        for o in occurrences:
            if resource_id not in o.resource_assignments.values():
                continue
            active_groups |= target_groups & _time_group_refs(instance, o.time_ref)
        deviation = _shortfall_or_excess(len(active_groups), minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _evaluate_avoid_unavailable_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = count of the
    # constraint's unavailable Times during which the resource attends
    # >=1 solution event.
    unavailable_times = _referenced_ids(constraint.params.get("Times"))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy_times = {
            o.time_ref
            for o in occurrences
            if o.time_ref is not None
            and resource_id in o.resource_assignments.values()
        }
        deviation = len(busy_times & unavailable_times)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _idle_count_in_group(
    instance: Instance, resource_id: str, occurrences: list[Occurrence], group_ref: str
) -> int:
    # Times are assumed consecutive in file order (XHSTT convention). A time
    # is idle if the resource is busy at an earlier AND a later time within
    # the same group -- i.e. it's a non-busy gap strictly between the
    # group's first and last busy time.
    group_times = [t.id for t in instance.times if group_ref in t.group_refs]
    busy = {
        o.time_ref
        for o in occurrences
        if o.time_ref is not None and resource_id in o.resource_assignments.values()
    }
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
        idle_total = sum(
            _idle_count_in_group(instance, resource_id, occurrences, g)
            for g in target_groups
        )
        deviation = _shortfall_or_excess(idle_total, minimum, maximum)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


def _busy_count_in_group(
    instance: Instance, resource_id: str, occurrences: list[Occurrence], group_ref: str
) -> int:
    group_times = {t.id for t in instance.times if group_ref in t.group_refs}
    return sum(
        1
        for o in occurrences
        if o.time_ref in group_times and resource_id in o.resource_assignments.values()
    )


def _evaluate_limit_busy_times_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource, but unlike ClusterBusyTimes/
    # LimitIdleTimes this produces one *independent* deviation per time
    # group (not pre-summed), and a group with 0 busy times is never
    # penalized even if that's below Minimum (built-in AllowZero carve-out).
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        for group_ref in target_groups:
            count = _busy_count_in_group(instance, resource_id, occurrences, group_ref)
            deviation = 0 if count == 0 else _shortfall_or_excess(count, minimum, maximum)
            total += constraint.weight * apply_cost_function(
                constraint.cost_function, deviation
            )
    return total


_EVALUATORS = {
    "AssignTimeConstraint": _evaluate_assign_time_constraint,
    "AvoidClashesConstraint": _evaluate_avoid_clashes_constraint,
    "AssignResourceConstraint": _evaluate_assign_resource_constraint,
    "PreferResourcesConstraint": _evaluate_prefer_resources_constraint,
    "ClusterBusyTimesConstraint": _evaluate_cluster_busy_times_constraint,
    "AvoidUnavailableTimesConstraint": _evaluate_avoid_unavailable_times_constraint,
    "LimitIdleTimesConstraint": _evaluate_limit_idle_times_constraint,
    "LimitBusyTimesConstraint": _evaluate_limit_busy_times_constraint,
}


def _evaluate_limit_workload_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = shortfall/excess
    # (rounded up) of total assigned workload against Minimum/Maximum.
    # Workload(event resource) x (sub-event duration / event duration),
    # summed over all solution resources assigned to that resource.
    # NOTE: Workload defaults to 1.0 when unspecified on the event
    # resource -- this default isn't spec-confirmed (rare constraint, no
    # real sample found using it alongside an unspecified Workload);
    # revisit if a G1 reference solution exercises this path.
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    events_by_id = {e.id: e for e in instance.events}
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        workload_sum = 0.0
        for o in occurrences:
            event_def = events_by_id[o.event_ref]
            for role, assigned in o.resource_assignments.items():
                if assigned != resource_id:
                    continue
                er = next((x for x in event_def.resources if x.role == role), None)
                workload = er.workload if er and er.workload is not None else 1.0
                workload_sum += workload * (o.duration / event_def.duration)
        deviation = _shortfall_or_excess(math.ceil(workload_sum), minimum, maximum)
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


_EVALUATORS["DistributeSplitEventsConstraint"] = _evaluate_distribute_split_events_constraint


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
        member_ids = {e.id for e in instance.events if group_ref in e.group_refs}
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
        member_ids = {e.id for e in instance.events if group_ref in e.group_refs}
        assigned = {
            o.resource_assignments.get(role)
            for o in occurrences
            if o.event_ref in member_ids
            and o.resource_assignments.get(role) is not None
        }
        deviation = max(0, len(assigned) - 1)
        total += constraint.weight * apply_cost_function(
            constraint.cost_function, deviation
        )
    return total


_EVALUATORS["AvoidSplitAssignmentsConstraint"] = _evaluate_avoid_split_assignments_constraint


def _occupied_time_ids(instance: Instance, time_ref: str | None, duration: int) -> set[str]:
    if time_ref is None:
        return set()
    all_ids = [t.id for t in instance.times]
    start = all_ids.index(time_ref)
    return set(all_ids[start : start + duration])


def _evaluate_link_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event group (individual events are not
    # points of application). For each instance event in the group, build
    # the set of all times its sub-events occupy (not just start times).
    # Deviation = count of times appearing in >=1 of those sets but not all.
    total = 0
    for group_ref in constraint.applies_to.event_groups:
        member_events = [e for e in instance.events if group_ref in e.group_refs]
        per_event_times = []
        for event in member_events:
            times_for_event = set()
            for o in occurrences:
                if o.event_ref == event.id:
                    times_for_event |= _occupied_time_ids(instance, o.time_ref, o.duration)
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
    all_ids = [t.id for t in instance.times]
    starts, ends = [], []
    for o in occurrences:
        if o.event_ref == event_id and o.time_ref is not None:
            start = all_ids.index(o.time_ref)
            starts.append(start)
            ends.append(start + o.duration - 1)
    if not starts:
        return None, None
    return min(starts), max(ends)


def _evaluate_order_events_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one event pair. UNVERIFIED AGAINST HSEval -- no
    # real XHSTT-2014 sample using this constraint was found anywhere in
    # the archive during research; the ordinal-subtraction direction below
    # is a best-effort reading of a spec summary the research itself
    # flagged as ambiguous. Re-derive from a real instance before trusting
    # this for gate G1. Deviation = shortfall/excess of the gap between
    # (end of first event's latest sub-event) and (start of second event's
    # earliest sub-event) against MinSeparation/MaxSeparation; 0 if either
    # event has no time-assigned sub-events.
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
