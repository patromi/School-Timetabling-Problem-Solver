from functools import cache

from src.model import Event, Instance

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
def _event_group_refs_cached(
    group_refs: tuple[str, ...], course_ref: str | None
) -> frozenset[str]:
    refs = set(group_refs)
    if course_ref is not None:
        refs.add(course_ref)
    return frozenset(refs)


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


@cache
def _event_group_members_cached(instance_key: int, group_ref: str) -> frozenset[str]:
    instance = _INSTANCE_REGISTRY[instance_key]
    return frozenset(e.id for e in instance.events if group_ref in _event_group_refs(e))


def _event_group_members(instance: Instance, group_ref: str) -> frozenset[str]:
    key = _register_instance(instance)
    return _event_group_members_cached(key, group_ref)


def _shortfall_or_excess(value: int, minimum: int, maximum: int) -> int:
    if value < minimum:
        return minimum - value
    if value > maximum:
        return value - maximum
    return 0


def _referenced_ids(entries: list[dict] | None) -> set[str]:  # type: ignore[type-arg]
    return {e["reference"] for e in (entries or [])}


def apply_cost_function(name: str, deviation: int) -> int:
    if name == "Linear":
        return deviation
    if name == "Quadratic":
        return deviation * deviation
    if name == "Step":
        return 1 if deviation != 0 else 0
    raise ValueError(f"unknown XHSTT cost function: {name!r}")


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


INFEASIBILITY_WEIGHT = 1_000_000
