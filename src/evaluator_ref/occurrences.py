from collections import Counter
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass, field
from functools import cache

from src.evaluator_ref._cache import (
    _INSTANCE_REGISTRY,
    _event_group_refs,
    _register_instance,
    _time_ids_ordered,
    _time_positions,
)
from src.model import AppliesTo, Event, Instance, Solution, SolutionEvent


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


def _assigned_resource(occurrence: Occurrence, role: str | None) -> str | None:
    """Looks up the (single) resource assigned to a given role -- correct
    for the constraint types that reference a specific Role, since the
    spec guarantees Role uniqueness for genuine assignable slots (the only
    slots those constraints ever target). Returns None if the role isn't
    present at all, or if present but unassigned."""
    return next(
        (r for role_, r in occurrence.resource_assignments if role_ == role), None
    )


def _assigned_resource_ids(occurrence: Occurrence) -> list[str | None]:
    return [r for _, r in occurrence.resource_assignments]


def resolve_occurrence(event_def: Event, solution_event: SolutionEvent) -> Occurrence:
    """Merges one SolutionEvent with its Event definition's preassignments.
    Extracted so an incremental evaluator can re-resolve a single changed
    entry without duplicating (and drifting from) the merge rules."""
    assignments = [(er.role, er.resource_ref) for er in event_def.resources]
    for sr in solution_event.resources:
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
    return Occurrence(
        event_ref=solution_event.event_ref,
        duration=solution_event.duration
        if solution_event.duration is not None
        else event_def.duration,
        time_ref=solution_event.time_ref
        if solution_event.time_ref is not None
        else event_def.time_ref,
        resource_assignments=assignments,
    )


def resolve_occurrences(instance: Instance, solution: Solution) -> list[Occurrence]:
    """Merges each Event's fixed resource/time preassignments (declared in
    the Instance) with what the Solution provides for roles/times left
    unassigned. An instance event that never appears in the Solution's
    Events at all (confirmed real pattern: a fully preassigned event, e.g.
    a staff meeting with fixed time and all resources fixed, is never
    listed explicitly) still "happens" once, at its full duration, and
    must be visible to the evaluator -- so it gets one synthesized
    occurrence rather than zero."""
    solution_events_by_ref: dict[str, list] = {}  # type: ignore[type-arg]
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
    key = _register_instance(instance)
    return _resources_in_applies_to_cached(
        key, tuple(applies_to.resource_groups), tuple(applies_to.resources)
    )


def _occupied_time_ids(
    instance: Instance, time_ref: str | None, duration: int
) -> set[str]:
    if time_ref is None:
        return set()
    all_ids = _time_ids_ordered(instance)
    start = _time_positions(instance)[time_ref]
    return set(all_ids[start : start + duration])


# Set (not None) only during an evaluate_cost_components() call, to a resource_id ->
# Counter[time_id] map built in ONE pass over occurrences -- see
# _build_occupancy_index. AvoidClashesConstraint, ClusterBusyTimesConstraint,
# LimitBusyTimesConstraint, LimitIdleTimesConstraint and
# AvoidUnavailableTimesConstraint all previously re-derived a resource's
# occupied times independently (once per resource, per constraint,
# rescanning every occurrence each time); profiling a real ~400-event
# instance (AU-BG-98) showed this redundant rescanning was the largest
# single cost after the earlier group-membership caching fix. Module-level
# and not thread-safe by design -- this solver is single-threaded.
_current_occupancy_index: dict[str, Counter[str]] | None = None


@contextmanager
def occupancy_index(index: dict[str, Counter[str]] | None) -> Iterator[None]:
    """Sets _current_occupancy_index for the duration of the `with` block,
    then restores the previous value (not always None) -- unlike the old
    pattern in delta.py that hardwired `finally: ... = None`, this is
    safe under nesting (which doesn't occur today but is correct if it
    ever does)."""
    global _current_occupancy_index
    previous = _current_occupancy_index
    _current_occupancy_index = index
    try:
        yield
    finally:
        _current_occupancy_index = previous


occupancy_index_scope = occupancy_index


def _get_current_occupancy_index() -> dict[str, Counter[str]] | None:
    """Accessor for constraints.py -- avoids the frozen-binding problem
    that would arise from `from occurrences import _current_occupancy_index`."""
    return _current_occupancy_index


def _build_occupancy_index(
    instance: Instance, occurrences: list[Occurrence]
) -> dict[str, Counter[str]]:
    index: dict[str, Counter[str]] = {}
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
