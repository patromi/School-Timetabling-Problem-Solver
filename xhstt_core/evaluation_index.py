"""Static per-instance index for incremental evaluation.

`evaluator_ref` re-derives a constraint's *points of application* (the units
at which the spec says one deviation, and therefore one CostFunction call,
is produced) on every evaluation, then loops over all of them. An
incremental evaluator instead needs the inverse question answered fast:
"given that this event / this resource just changed, which (constraint,
point) pairs can possibly have a different cost now?" -- everything else is
provably unchanged and must not be recomputed.

This module answers that question with data that depends only on the
Instance, so it is built once per instance and never touched again during a
solve. Nothing here evaluates a constraint; see `xhstt_core.incremental`.
"""

from collections.abc import Iterator
from dataclasses import dataclass
from functools import cache

from xhstt_core.evaluator_ref import (
    _event_group_members,
    _events_in_applies_to,
    _preferred_resource_ids,
    _preferred_time_ids,
    _referenced_ids,
    _register_instance,
    _resources_in_applies_to,
    _time_ids_ordered,
)
from xhstt_core.model import Constraint, Event, Instance

EVENT = "event"
RESOURCE = "resource"
EVENT_GROUP = "event_group"
EVENT_PAIR = "event_pair"

# Which kind of object one point of application is, per constraint type --
# read straight off the XHSTT spec's "points of application" sentence for
# each type, and matching the loop `evaluator_ref._evaluate_*_constraint`
# already performs.
POINT_KIND: dict[str, str] = {
    "AssignTimeConstraint": EVENT,
    "AssignResourceConstraint": EVENT,
    "PreferResourcesConstraint": EVENT,
    "PreferTimesConstraint": EVENT,
    "SplitEventsConstraint": EVENT,
    "DistributeSplitEventsConstraint": EVENT,
    "AvoidClashesConstraint": RESOURCE,
    "ClusterBusyTimesConstraint": RESOURCE,
    "AvoidUnavailableTimesConstraint": RESOURCE,
    "LimitIdleTimesConstraint": RESOURCE,
    "LimitBusyTimesConstraint": RESOURCE,
    "LimitWorkloadConstraint": RESOURCE,
    "SpreadEventsConstraint": EVENT_GROUP,
    "AvoidSplitAssignmentsConstraint": EVENT_GROUP,
    "LinkEventsConstraint": EVENT_GROUP,
    "OrderEventsConstraint": EVENT_PAIR,
}

# A point id is an event id, a resource id, an event group ref, or -- for
# OrderEventsConstraint, whose points are pairs rather than named objects --
# an index into constraint.applies_to.event_pairs.
PointId = str | int
PointKey = tuple[int, PointId]


@dataclass(frozen=True)
class ConstraintPoints:
    """One constraint, with its points of application resolved once and the
    scope sets its deviation formula needs pre-derived (each of those is a
    full scan of instance.times/resources in `evaluator_ref`, repeated per
    evaluation)."""

    index: int
    constraint: Constraint
    kind: str
    points: tuple[PointId, ...]
    referenced_times: frozenset[str]
    referenced_time_groups: frozenset[str]
    preferred_times: frozenset[str]
    preferred_resources: frozenset[str]


def _event_points(instance: Instance, constraint: Constraint) -> tuple[PointId, ...]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    role = constraint.params.get("Role")
    if constraint.type == "AssignResourceConstraint":
        # Spec: the points are the *unpreassigned* event resources with this
        # Role, so an event with no such slot is not a point at all.
        keep = [
            e.id
            for e in instance.events
            if e.id in event_ids
            and any(er.role == role and er.resource_ref is None for er in e.resources)
        ]
    elif constraint.type == "PreferResourcesConstraint":
        keep = [
            e.id
            for e in instance.events
            if e.id in event_ids and any(er.role == role for er in e.resources)
        ]
    else:
        keep = [e.id for e in instance.events if e.id in event_ids]
    return tuple(keep)


def _build_constraint_points(instance: Instance) -> tuple[ConstraintPoints, ...]:
    entries = []
    for index, constraint in enumerate(instance.constraints):
        try:
            kind = POINT_KIND[constraint.type]
        except KeyError:
            raise NotImplementedError(
                f"no incremental point-of-application mapping for "
                f"{constraint.type!r} (constraint id={constraint.id!r})"
            ) from None

        if kind == EVENT:
            points: tuple[PointId, ...] = _event_points(instance, constraint)
        elif kind == RESOURCE:
            points = tuple(
                sorted(_resources_in_applies_to(instance, constraint.applies_to))
            )
        elif kind == EVENT_GROUP:
            points = tuple(constraint.applies_to.event_groups)
        else:
            points = tuple(range(len(constraint.applies_to.event_pairs)))

        entries.append(
            ConstraintPoints(
                index=index,
                constraint=constraint,
                kind=kind,
                points=points,
                referenced_times=frozenset(
                    _referenced_ids(constraint.params.get("Times"))
                ),
                referenced_time_groups=frozenset(
                    _referenced_ids(constraint.params.get("TimeGroups"))
                ),
                preferred_times=frozenset(_preferred_time_ids(instance, constraint))
                if constraint.type == "PreferTimesConstraint"
                else frozenset(),
                preferred_resources=frozenset(
                    _preferred_resource_ids(instance, constraint)
                )
                if constraint.type == "PreferResourcesConstraint"
                else frozenset(),
            )
        )
    return tuple(entries)


@dataclass(frozen=True)
class InstanceIndex:
    """Everything an incremental evaluator needs that never changes during a
    solve."""

    instance: Instance
    time_ids: tuple[str, ...]
    time_positions: dict[str, int]
    groups_of_time: dict[str, frozenset[str]]
    times_of_group: dict[str, tuple[str, ...]]
    events_by_id: dict[str, Event]
    constraint_points: tuple[ConstraintPoints, ...]
    points_for_event: dict[str, tuple[PointKey, ...]]
    points_for_resource: dict[str, tuple[PointKey, ...]]


def _event_triggers(
    instance: Instance, cp: ConstraintPoints
) -> Iterator[tuple[str, PointKey]]:
    """(event id, point) pairs meaning "if this event changes, that point has
    to be re-scored"."""
    if cp.kind == EVENT:
        for point in cp.points:
            yield str(point), (cp.index, point)
    elif cp.kind == EVENT_GROUP:
        for point in cp.points:
            # Any member event changing can move the whole group's
            # deviation -- and membership includes an event's Course, not
            # just explicit EventGroup references.
            for event_id in _event_group_members(instance, str(point)):
                yield event_id, (cp.index, point)
    elif cp.kind == EVENT_PAIR:
        for position, pair in enumerate(cp.constraint.applies_to.event_pairs):
            yield pair.first_event, (cp.index, position)
            yield pair.second_event, (cp.index, position)


def _build_reverse_maps(
    instance: Instance, constraint_points: tuple[ConstraintPoints, ...]
) -> tuple[dict[str, tuple[PointKey, ...]], dict[str, tuple[PointKey, ...]]]:
    by_event: dict[str, list[PointKey]] = {}
    by_resource: dict[str, list[PointKey]] = {}

    for cp in constraint_points:
        for event_id, key in _event_triggers(instance, cp):
            by_event.setdefault(event_id, []).append(key)
        if cp.kind == RESOURCE:
            for point in cp.points:
                by_resource.setdefault(str(point), []).append((cp.index, point))

    return (
        {k: tuple(v) for k, v in by_event.items()},
        {k: tuple(v) for k, v in by_resource.items()},
    )


@cache
def _instance_index_cached(instance_key: int) -> InstanceIndex:
    instance = _instance_from_key(instance_key)
    time_ids = _time_ids_ordered(instance)
    constraint_points = _build_constraint_points(instance)
    points_for_event, points_for_resource = _build_reverse_maps(
        instance, constraint_points
    )

    times_of_group: dict[str, list[str]] = {}
    for time in instance.times:
        for group_ref in time.group_refs:
            times_of_group.setdefault(group_ref, []).append(time.id)

    return InstanceIndex(
        instance=instance,
        time_ids=time_ids,
        time_positions={t: i for i, t in enumerate(time_ids)},
        groups_of_time={t.id: frozenset(t.group_refs) for t in instance.times},
        times_of_group={k: tuple(v) for k, v in times_of_group.items()},
        events_by_id={e.id: e for e in instance.events},
        constraint_points=constraint_points,
        points_for_event=points_for_event,
        points_for_resource=points_for_resource,
    )


def _instance_from_key(instance_key: int) -> Instance:
    from xhstt_core.evaluator_ref import _INSTANCE_REGISTRY

    return _INSTANCE_REGISTRY[instance_key]


def instance_index(instance: Instance) -> InstanceIndex:
    return _instance_index_cached(_register_instance(instance))
