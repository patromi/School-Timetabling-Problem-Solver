import math
from collections import Counter
from dataclasses import dataclass, field
from functools import cache

from xhstt_core.model import AppliesTo, Constraint, Event, Instance, Solution


@cache
def _event_group_refs_cached(
    group_refs: tuple[str, ...], course_ref: str | None
) -> frozenset[str]:
    refs = set(group_refs)
    if course_ref is not None:
        refs.add(course_ref)
    return frozenset(refs)


_INSTANCE_REGISTRY: dict[int, Instance] = {}


def _register_instance(instance: Instance) -> int:
    # Keyed by id(instance): safe because the registry keeps a strong
    # reference to every Instance it has ever seen, so that id() can never
    # be reused for a *different* live Instance for the rest of the
    # process (this project always solves one, occasionally a handful, of
    # Instance objects per run -- unbounded growth here is a non-issue).
    key = id(instance)
    _INSTANCE_REGISTRY[key] = instance
    return key


@cache
def _event_group_members_cached(instance_key: int, group_ref: str) -> frozenset[str]:
    instance = _INSTANCE_REGISTRY[instance_key]
    return frozenset(e.id for e in instance.events if group_ref in _event_group_refs(e))


def _event_group_members(instance: Instance, group_ref: str) -> frozenset[str]:
    # Cached for the same reason as _event_group_refs: several evaluators
    # (SpreadEvents, AvoidSplitAssignments, LinkEvents) re-scan *every*
    # instance event to resolve one event group's membership, once per
    # constraint per cost evaluation -- static data, recomputed anyway.
    key = _register_instance(instance)
    return _event_group_members_cached(key, group_ref)


def _event_group_refs(event: Event) -> frozenset[str]:
    # Spec: "Course is an alternative form for EventGroup" -- an event's
    # Course reference counts as membership in that event group too,
    # exactly like an explicit <EventGroups><EventGroup Reference=.../>
    # entry. Confirmed as a real gap via direct HSEval comparison on
    # BrazilInstance1 (events whose only group membership route was via
    # <Course> were invisible to AppliesTo.EventGroups matching).
    #
    # Cached: an event's group membership is static instance data -- it
    # never changes during a solve, but this function was being called
    # ~350,000 times per LAHC iteration on a mid-size real instance
    # (AU-BG-98, profiled), because every constraint evaluation re-derives
    # it for every event from scratch. Caching on the *value* of
    # (group_refs, course_ref) rather than object identity avoids any
    # id()-reuse/GC hazard.
    return _event_group_refs_cached(tuple(event.group_refs), event.course_ref)


@dataclass
class Occurrence:
    """One concrete meeting of an Event — one per SolutionEvent entry, since
    a split event occurs multiple times under the same event_ref.

    resource_assignments is a LIST of (role, resource_ref) pairs, not a
    dict -- Role is only required to be unique within an event when it
    identifies an assignable slot; the spec explicitly allows omitting
    Role for preassigned resources, and real instances (e.g. a staff
    meeting listing dozens of attendees) commonly have many preassigned
    resources sharing the same (often empty) role. A role-keyed dict would
    silently collapse all but one of them."""

    event_ref: str
    duration: int
    time_ref: str | None
    resource_assignments: list[tuple[str, str | None]] = field(default_factory=list)


def _assigned_resource(occurrence: "Occurrence", role: str) -> str | None:
    """Looks up the (single) resource assigned to a given role -- correct
    for the constraint types that reference a specific Role, since the
    spec guarantees Role uniqueness for genuine assignable slots (the only
    slots those constraints ever target). Returns None if the role isn't
    present at all, or if present but unassigned."""
    return next(
        (r for role_, r in occurrence.resource_assignments if role_ == role), None
    )


def _assigned_resource_ids(occurrence: "Occurrence") -> list[str | None]:
    return [r for _, r in occurrence.resource_assignments]


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
    """Merges each Event's fixed resource/time preassignments (declared in
    the Instance) with what the Solution provides for roles/times left
    unassigned. An instance event that never appears in the Solution's
    Events at all (confirmed real pattern: a fully preassigned event, e.g.
    a staff meeting with fixed time and all resources fixed, is never
    listed explicitly) still "happens" once, at its full duration, and
    must be visible to the evaluator -- so it gets one synthesized
    occurrence rather than zero."""
    events_by_id = {e.id: e for e in instance.events}
    solution_events_by_ref: dict[str, list] = {}
    for se in solution.events:
        solution_events_by_ref.setdefault(se.event_ref, []).append(se)

    occurrences = []
    for event_def in instance.events:
        fixed_assignments = [(er.role, er.resource_ref) for er in event_def.resources]
        matching = solution_events_by_ref.get(event_def.id)
        if not matching:
            occurrences.append(
                Occurrence(
                    event_ref=event_def.id,
                    duration=event_def.duration,
                    time_ref=event_def.time_ref,
                    resource_assignments=list(fixed_assignments),
                )
            )
            continue
        for se in matching:
            assignments = list(fixed_assignments)
            for sr in se.resources:
                # A solution override fills in the first still-unassigned
                # slot with a matching role (the "assignable slot" the
                # spec's Role-uniqueness guarantee refers to); if none is
                # found, add it as a new entry.
                for i, (role, ref) in enumerate(assignments):
                    if role == sr.role and ref is None:
                        assignments[i] = (role, sr.resource_ref)
                        break
                else:
                    assignments.append((sr.role, sr.resource_ref))
            occurrences.append(
                Occurrence(
                    event_ref=se.event_ref,
                    duration=se.duration
                    if se.duration is not None
                    else event_def.duration,
                    time_ref=se.time_ref
                    if se.time_ref is not None
                    else event_def.time_ref,
                    resource_assignments=assignments,
                )
            )
    return occurrences


@cache
def _events_in_applies_to_cached(
    instance_key: int, event_groups: tuple[str, ...], events: tuple[str, ...]
) -> frozenset[str]:
    instance = _INSTANCE_REGISTRY[instance_key]
    ids = set(events)
    if event_groups:
        groups = set(event_groups)
        ids.update(e.id for e in instance.events if groups & _event_group_refs(e))
    return frozenset(ids)


def _events_in_applies_to(instance: Instance, applies_to: AppliesTo) -> frozenset[str]:
    # Cached like _event_group_refs: which events a constraint's AppliesTo
    # resolves to is static for the whole solve (depends only on instance
    # data), but was being recomputed -- a full scan of instance.events --
    # on every single cost evaluation of every constraint that uses it.
    key = _register_instance(instance)
    return _events_in_applies_to_cached(
        key, tuple(applies_to.event_groups), tuple(applies_to.events)
    )


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


@cache
def _resources_in_applies_to_cached(
    instance_key: int, resource_groups: tuple[str, ...], resources: tuple[str, ...]
) -> frozenset[str]:
    instance = _INSTANCE_REGISTRY[instance_key]
    ids = set(resources)
    if resource_groups:
        groups = set(resource_groups)
        ids.update(r.id for r in instance.resources if groups & set(r.group_refs))
    return frozenset(ids)


def _resources_in_applies_to(
    instance: Instance, applies_to: AppliesTo
) -> frozenset[str]:
    # Cached for the same reason as _events_in_applies_to above.
    key = _register_instance(instance)
    return _resources_in_applies_to_cached(
        key, tuple(applies_to.resource_groups), tuple(applies_to.resources)
    )


@cache
def _time_ids_ordered_cached(instance_key: int) -> tuple[str, ...]:
    instance = _INSTANCE_REGISTRY[instance_key]
    return tuple(t.id for t in instance.times)


@cache
def _time_positions_cached(instance_key: int) -> dict[str, int]:
    return {t: i for i, t in enumerate(_time_ids_ordered_cached(instance_key))}


def _time_ids_ordered(instance: Instance) -> tuple[str, ...]:
    return _time_ids_ordered_cached(_register_instance(instance))


def _time_positions(instance: Instance) -> dict[str, int]:
    # instance.times' order (and thus each time's position) is static --
    # both the ordered id list and the position-lookup dict were being
    # rebuilt from scratch (with a linear .index() search into the bargain)
    # on every single call from multiple hot paths.
    return _time_positions_cached(_register_instance(instance))


def _occupied_time_ids(
    instance: Instance, time_ref: str | None, duration: int
) -> set[str]:
    if time_ref is None:
        return set()
    all_ids = _time_ids_ordered(instance)
    start = _time_positions(instance)[time_ref]
    return set(all_ids[start : start + duration])


# Set (not None) only during a total_cost() call, to a resource_id ->
# Counter[time_id] map built in ONE pass over occurrences -- see
# _build_occupancy_index. AvoidClashesConstraint, ClusterBusyTimesConstraint,
# LimitBusyTimesConstraint, LimitIdleTimesConstraint and
# AvoidUnavailableTimesConstraint all previously re-derived a resource's
# occupied times independently (once per resource, per constraint,
# rescanning every occurrence each time); profiling a real ~400-event
# instance (AU-BG-98) showed this redundant rescanning was the largest
# single cost after the earlier group-membership caching fix. Module-level
# and not thread-safe by design -- this solver is single-threaded.
_current_occupancy_index: dict[str, "Counter[str]"] | None = None


def _build_occupancy_index(
    instance: Instance, occurrences: list[Occurrence]
) -> dict[str, "Counter[str]"]:
    index: dict[str, Counter] = {}
    for o in occurrences:
        if o.time_ref is None:
            continue
        span = None
        for _, ref in o.resource_assignments:
            if ref is None:
                continue
            if span is None:
                span = _occupied_time_ids(instance, o.time_ref, o.duration)
            counter = index.setdefault(ref, Counter())
            for t in span:
                counter[t] += 1
    return index


def _full_span_busy_times(
    instance: Instance, occurrences: list[Occurrence], resource_id: str
) -> set[str]:
    """All time ids a resource is occupied at, expanding each occurrence's
    full duration span rather than just its start time -- confirmed
    required by spec (explicit for AvoidClashesConstraint: "All times when
    the solution resources' solution events are running are included, not
    just their starting times") and, via direct comparison against a real
    HSEval report for BrazilInstance1.xml, required for "busy" in general
    (LimitIdleTimesConstraint, ClusterBusyTimesConstraint,
    LimitBusyTimesConstraint, AvoidUnavailableTimesConstraint all define
    "busy" the same way, off the same general definition)."""
    if _current_occupancy_index is not None:
        counter = _current_occupancy_index.get(resource_id)
        return set(counter.keys()) if counter else set()
    busy = set()
    for o in occurrences:
        if o.time_ref is None or resource_id not in _assigned_resource_ids(o):
            continue
        busy |= _occupied_time_ids(instance, o.time_ref, o.duration)
    return busy


def _evaluate_avoid_clashes_constraint(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> int:
    # Point of application: one resource. Deviation = sum, over all times
    # the resource is preassigned/assigned to >=2 occurrences (counting
    # every time slot each occurrence's duration spans, not just its
    # start), of (count - 1).
    total = 0
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        if _current_occupancy_index is not None:
            counter = _current_occupancy_index.get(resource_id, Counter())
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
    # Point of application: one event pair. Verified against the
    # authoritative spec (fetched directly via curl over plain HTTP, since
    # HTTPS/WebFetch to jeffreykingston.id.au is refused from this
    # environment). Deviation = shortfall/excess of the gap between (end of
    # first event's latest sub-event) and (start of second event's
    # earliest sub-event) against MinSeparation/MaxSeparation; 0 if either
    # event has zero solution events or any solution event with an
    # unassigned time.
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


@cache
def _day_group_ids_cached(instance_key: int) -> frozenset[str]:
    instance = _INSTANCE_REGISTRY[instance_key]
    return frozenset(g.id for g in instance.time_groups if g.kind == "Day")


def _day_group_ref(instance: Instance, time_id: str) -> str | None:
    day_group_ids = _day_group_ids_cached(_register_instance(instance))
    time = next(t for t in instance.times if t.id == time_id)
    return next((ref for ref in time.group_refs if ref in day_group_ids), None)


@cache
def _valid_start_time_ids(instance_key: int, duration: int) -> tuple[str, ...]:
    """Start times for which a `duration`-slot span neither overflows past
    the last defined Time (the exact structural error HSEval reports:
    "<Time> not assignable to <Event>") nor crosses into a different Day
    group, when the times involved belong to one. Shared (cached) by both
    construct.py's initial solution builder and moves.py's time-changing
    moves -- moves.py had the exact same overflow bug construct.py was
    fixed for (real HSEval rejection: "'Fr_5' not assignable to Event
    'T10-S1'", surfaced after a longer LAHC run on a real multi-day-period
    instance), because it picked *any* instance time with no regard for
    whether the event's duration would still fit there."""
    instance = _INSTANCE_REGISTRY[instance_key]
    all_times = instance.times
    n = len(all_times)
    valid = []
    for i, t in enumerate(all_times):
        if i + duration > n:
            continue
        day_ref = _day_group_ref(instance, t.id)
        if day_ref is not None and any(
            _day_group_ref(instance, all_times[j].id) != day_ref
            for j in range(i, i + duration)
        ):
            continue
        valid.append(t.id)
    return tuple(valid)


def valid_start_time_ids(instance: Instance, duration: int) -> tuple[str, ...]:
    return _valid_start_time_ids(_register_instance(instance), duration)


_INFEASIBILITY_WEIGHT = 1_000_000


def total_cost(instance: Instance, solution: Solution) -> int:
    """Lexicographic total: infeasibility (sum of Required=true constraint
    costs) dominates objective (sum of Required=false costs), matching the
    Env sketch in the thesis plan (`infeas * 1_000_000 + obj`) -- a single
    point of infeasibility always outweighs any amount of objective cost,
    so a solver comparing this scalar naturally prioritizes feasibility
    first."""
    global _current_occupancy_index
    occurrences = resolve_occurrences(instance, solution)
    _current_occupancy_index = _build_occupancy_index(instance, occurrences)
    try:
        infeasibility = 0
        objective = 0
        for c in instance.constraints:
            cost = evaluate_constraint(instance, occurrences, c)
            if c.required:
                infeasibility += cost
            else:
                objective += cost
        return infeasibility * _INFEASIBILITY_WEIGHT + objective
    finally:
        _current_occupancy_index = None
