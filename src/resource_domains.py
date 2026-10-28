"""Which resources may fill an assignable event resource (a role left
without a Reference in the instance), and which solution entries have to
change their resource together.

Shared by the constructors and the resource moves, so both draw from the
same candidates: drawing from every resource of the slot's ResourceType
ignores Required PreferResourcesConstraints and starts e.g. AU-BG-98 at
~1.4 million hard cost.
"""

from dataclasses import dataclass, replace
from functools import cache

from src.evaluator_ref import (
    _INSTANCE_REGISTRY,
    _event_group_members,
    _events_in_applies_to,
    _preferred_resource_ids,
    _register_instance,
)
from src.model import Instance, Solution, SolutionEventResource


def has_resource_choices(instance: Instance) -> bool:
    """True if some event resource is left for the solver to assign and
    has more than one resource in its domain."""
    return any(
        er.resource_ref is None and len(resource_domain(instance, e.id, er.role)) > 1
        for e in instance.events
        for er in e.resources
    )


@cache
def _domain_cached(instance_key: int, event_ref: str, role: str) -> tuple[str, ...]:
    instance = _INSTANCE_REGISTRY[instance_key]
    event = next(e for e in instance.events if e.id == event_ref)
    slot = next(
        (er for er in event.resources if er.role == role and er.resource_ref is None),
        None,
    )
    if slot is None:
        return ()
    of_type = [
        r.id
        for r in instance.resources
        if r.resource_type_ref == slot.resource_type_ref
    ]
    allowed = set(of_type)
    for c in instance.constraints:
        if (
            c.type == "PreferResourcesConstraint"
            and c.required
            and c.params.get("Role") == role
            and event_ref in _events_in_applies_to(instance, c.applies_to)
        ):
            allowed &= _preferred_resource_ids(instance, c)
    # Contradicting Required preferences (5 slots in DK-HG-12): every choice
    # costs the same there, so keep the whole type rather than nothing.
    return tuple(r for r in of_type if r in allowed) or tuple(of_type)


def resource_domain(instance: Instance, event_ref: str, role: str) -> tuple[str, ...]:
    """Resources of the slot's ResourceType allowed by every Required
    PreferResourcesConstraint on (event, role), in instance order. Empty if
    the event has no assignable slot for `role`."""
    return _domain_cached(_register_instance(instance), event_ref, role)


@cache
def _soft_preferred_cached(
    instance_key: int, event_ref: str, role: str
) -> frozenset[str]:
    instance = _INSTANCE_REGISTRY[instance_key]
    preferred = set(_domain_cached(instance_key, event_ref, role))
    for c in instance.constraints:
        if (
            c.type == "PreferResourcesConstraint"
            and not c.required
            and c.params.get("Role") == role
            and event_ref in _events_in_applies_to(instance, c.applies_to)
        ):
            narrowed = preferred & _preferred_resource_ids(instance, c)
            if narrowed:
                preferred = narrowed
    return frozenset(preferred)


def soft_preferred(instance: Instance, event_ref: str, role: str) -> frozenset[str]:
    """The part of resource_domain also favoured by the non-required
    PreferResourcesConstraints on (event, role) -- a tie-breaker for the
    constructor, not a restriction."""
    return _soft_preferred_cached(_register_instance(instance), event_ref, role)


@cache
def _assignment_groups_cached(
    instance_key: int, role: str
) -> dict[str, frozenset[str]]:
    instance = _INSTANCE_REGISTRY[instance_key]
    # Groups sharing an event must share the resource too: merge them.
    group_of: dict[str, frozenset[str]] = {}
    for c in instance.constraints:
        if c.type != "AvoidSplitAssignmentsConstraint" or c.params.get("Role") != role:
            continue
        for group_ref in c.applies_to.event_groups:
            merged = set(_event_group_members(instance, group_ref))
            for event_ref in list(merged):
                merged |= group_of.get(event_ref, frozenset())
            frozen = frozenset(merged)
            for event_ref in frozen:
                group_of[event_ref] = frozen
    return group_of


def assignment_group(instance: Instance, event_ref: str, role: str) -> frozenset[str]:
    """Events whose `role` an AvoidSplitAssignmentsConstraint wants filled
    by one resource, `event_ref` included; just {event_ref} if none does."""
    groups = _assignment_groups_cached(_register_instance(instance), role)
    return groups.get(event_ref, frozenset({event_ref}))


@cache
def _shared_domain_cached(
    instance_key: int, event_refs: frozenset[str], role: str
) -> tuple[str, ...]:
    domains = [
        d for e in sorted(event_refs) if (d := _domain_cached(instance_key, e, role))
    ]
    if not domains:
        return ()
    common = set(domains[0]).intersection(*domains[1:])
    return tuple(r for r in domains[0] if r in common) or domains[0]


def shared_domain(
    instance: Instance, event_refs: frozenset[str], role: str
) -> tuple[str, ...]:
    """Resources in the resource_domain of every event in `event_refs`;
    falls back to the first event's domain if they have none in common."""
    return _shared_domain_cached(_register_instance(instance), event_refs, role)


def is_split_together(instance: Instance, event_ref: str, role: str) -> bool:
    """Whether some AvoidSplitAssignmentsConstraint covers (event, role) --
    then all of the event's pieces move to a new resource together."""
    return event_ref in _assignment_groups_cached(_register_instance(instance), role)


@dataclass(frozen=True)
class ResourceUnit:
    """Solution resource entries that change resource as one: every piece of
    every event in one assignment group, or a single piece otherwise.
    `entries` are (index into solution.events, index into its resources)."""

    role: str
    entries: tuple[tuple[int, int], ...]
    domain: tuple[str, ...]
    resource: str


def resource_units(instance: Instance, solution: Solution) -> list[ResourceUnit]:
    """The units of the solution's assignable resources, in solution order."""
    # key: (role, assignment group) for grouped entries, (role, i, k) otherwise
    entries: dict[tuple[object, ...], list[tuple[int, int]]] = {}
    events_of: dict[tuple[object, ...], frozenset[str]] = {}
    for i, se in enumerate(solution.events):
        for k, sr in enumerate(se.resources):
            if is_split_together(instance, se.event_ref, sr.role):
                group = assignment_group(instance, se.event_ref, sr.role)
                key: tuple[object, ...] = (sr.role, group)
                events_of[key] = group
            else:
                key = (sr.role, i, k)
                events_of[key] = frozenset({se.event_ref})
            entries.setdefault(key, []).append((i, k))

    units = []
    for key, unit_entries in entries.items():
        first_i, first_k = unit_entries[0]
        role = solution.events[first_i].resources[first_k].role
        units.append(
            ResourceUnit(
                role=role,
                entries=tuple(unit_entries),
                domain=shared_domain(instance, events_of[key], role),
                resource=solution.events[first_i].resources[first_k].resource_ref,
            )
        )
    return units


def with_unit_resource(
    solution: Solution, unit: ResourceUnit, resource_ref: str
) -> Solution:
    """A new Solution with every entry of `unit` set to `resource_ref`;
    only the touched SolutionEvents are rebuilt."""
    events = list(solution.events)
    for i, k in unit.entries:
        resources = list(events[i].resources)
        resources[k] = SolutionEventResource(role=unit.role, resource_ref=resource_ref)
        events[i] = replace(events[i], resources=resources)
    return replace(solution, events=events)
