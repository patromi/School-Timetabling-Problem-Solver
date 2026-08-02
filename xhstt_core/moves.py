import itertools
import random
from dataclasses import replace

from xhstt_core.evaluator_ref import valid_start_time_ids
from xhstt_core.model import (
    Event,
    Instance,
    Solution,
    SolutionEvent,
    SolutionEventResource,
)

# time_swap_move retries a bounded number of random pairs looking for one
# where swapping keeps BOTH events within a valid (non-overflowing,
# same-day) time span, rather than enumerating all O(n^2) pairs.
_MAX_SWAP_ATTEMPTS = 20


def time_reassign_move(
    instance: Instance, solution: Solution, rng: random.Random
) -> Solution:
    """Picks one solution event at random and reassigns it to a different
    (uniformly random) VALID time -- one where the event's duration still
    fits without overflowing past the last defined Time or crossing into
    a different Day group (see valid_start_time_ids; same check
    construct.py's initial builder uses). Returns a new Solution; the
    input is untouched.

    Confirmed as a real bug via a genuine HSEval rejection ("'Fr_5' not
    assignable to Event 'T10-S1'") surfaced after a longer LAHC run on a
    real multi-period instance: the old version picked *any* instance
    time with no regard for whether the event's duration would still fit
    there.

    Only considers solution events with a time to reassign -- a
    resources-only entry (time_ref=None, duration=None; see
    construct.build_initial, an event whose time is already fixed on the
    Instance but needed a resource assignment) has no duration, so
    valid_start_time_ids(instance, event.duration) would crash on
    None+int. Confirmed as a real crash on AU-BG-98, whose initial
    solution has 8 such entries among 765 -- rare enough that a
    fixed-seed smoke run can go hundreds of iterations before drawing one.

    Uses structural sharing rather than a deep copy: only the touched
    SolutionEvent is rebuilt (via dataclasses.replace), every other event
    keeps the exact same object reference. Confirmed as a real hot spot by
    profiling on a large real instance (AU-BG-98): deepcopy of the whole
    Solution -- events, their resource lists, everything -- on every
    single move accounted for ~4.6s of every 23.8s spent on 100 LAHC
    iterations."""
    candidates = [
        i
        for i, se in enumerate(solution.events)
        if se.time_ref is not None and se.duration is not None
    ]
    if not candidates:
        raise ValueError(
            "cannot apply a move: no solution event has a time to reassign"
        )
    index = rng.choice(candidates)
    event = solution.events[index]
    assert event.duration is not None
    other_times = [
        t for t in valid_start_time_ids(instance, event.duration) if t != event.time_ref
    ]
    if not other_times:
        raise ValueError("instance has no alternative valid time to reassign to")
    new_event = replace(event, time_ref=rng.choice(other_times))
    new_events = list(solution.events)
    new_events[index] = new_event
    return replace(solution, events=new_events)


def time_swap_move(
    instance: Instance, solution: Solution, rng: random.Random
) -> Solution:
    """Picks two distinct solution events at random and swaps their times.
    Returns a new Solution; the input is untouched. Structural sharing as
    in time_reassign_move.

    Swapping is only valid if BOTH events' durations still fit at their
    new (the other's old) time -- with different durations, a swap that
    was fine for a same-duration pair can silently overflow one side (the
    same class of bug time_reassign_move had). Retries a bounded number of
    random pairs looking for a valid one rather than enumerating all
    pairs; raises if none is found in the budget."""
    if len(solution.events) < 2:
        raise ValueError("time_swap_move needs at least 2 solution events")
    n = len(solution.events)
    for _ in range(min(_MAX_SWAP_ATTEMPTS, n * (n - 1) // 2)):
        i, j = rng.sample(range(n), 2)
        a, b = solution.events[i], solution.events[j]
        if a.time_ref is None or b.time_ref is None:
            continue
        assert a.duration is not None and b.duration is not None
        if b.time_ref not in valid_start_time_ids(instance, a.duration):
            continue
        if a.time_ref not in valid_start_time_ids(instance, b.duration):
            continue
        new_events = list(solution.events)
        new_events[i] = replace(a, time_ref=b.time_ref)
        new_events[j] = replace(b, time_ref=a.time_ref)
        return replace(solution, events=new_events)
    raise ValueError("no valid time swap found within the attempt budget")


def resource_reassign_move(
    instance: Instance, solution: Solution, rng: random.Random
) -> Solution:
    """Picks one (solution event, event resource) pair at random and
    reassigns it to a different resource of the same ResourceType. Returns
    a new Solution; the input is untouched. Structural sharing as in
    time_reassign_move -- only the touched SolutionEvent's resources list,
    and the one changed SolutionEventResource in it, are rebuilt."""
    resources_by_type: dict[str, list[str]] = {}
    for r in instance.resources:
        resources_by_type.setdefault(r.resource_type_ref, []).append(r.id)
    events_by_id = {e.id: e for e in instance.events}

    candidates = []  # (event index, resource index, ResourceType id)
    for se_index, se in enumerate(solution.events):
        event_def = events_by_id[se.event_ref]
        for r_index, sr in enumerate(se.resources):
            er = next((x for x in event_def.resources if x.role == sr.role), None)
            type_ref = er.resource_type_ref if er else None
            if type_ref and len(resources_by_type.get(type_ref, [])) > 1:
                candidates.append((se_index, r_index, type_ref))
    if not candidates:
        raise ValueError("no reassignable event resource with a same-type alternative")

    se_index, r_index, type_ref = rng.choice(candidates)
    target_event = solution.events[se_index]
    target_resource = target_event.resources[r_index]
    alternatives = [
        r for r in resources_by_type[type_ref] if r != target_resource.resource_ref
    ]
    new_resource = SolutionEventResource(
        role=target_resource.role, resource_ref=rng.choice(alternatives)
    )

    new_resources = list(target_event.resources)
    new_resources[r_index] = new_resource
    new_events = list(solution.events)
    new_events[se_index] = replace(target_event, resources=new_resources)
    return replace(solution, events=new_events)


def _kempe_resource_ids(event_def: Event, se: SolutionEvent) -> frozenset[str]:
    # Union of resources fixed on the Event definition and resources chosen
    # in the Solution -- either kind makes two events clash if they ever
    # land on the same time, so both count toward the conflict graph.
    ids = {er.resource_ref for er in event_def.resources if er.resource_ref is not None}
    ids |= {sr.resource_ref for sr in se.resources}
    return frozenset(ids)


def _kempe_candidate_nodes(
    instance: Instance, solution: Solution, t1: str, t2: str
) -> dict[int, frozenset[str]]:
    # Solution-event indices currently at t1 or t2, restricted to ones whose
    # duration fits (non-overflowing, same-day) at BOTH times -- same guard
    # as time_swap_move -- mapped to the resource ids that make them clash
    # with another event at the same time.
    events_by_id = {e.id: e for e in instance.events}
    resource_ids: dict[int, frozenset[str]] = {}
    for i, se in enumerate(solution.events):
        if se.time_ref not in (t1, t2) or se.duration is None:
            continue
        valid = valid_start_time_ids(instance, se.duration)
        if t1 not in valid or t2 not in valid:
            continue
        resource_ids[i] = _kempe_resource_ids(events_by_id[se.event_ref], se)
    return resource_ids


def _kempe_component(
    resource_ids: dict[int, frozenset[str]], rng: random.Random
) -> set[int]:
    # Conflict graph: an edge connects two candidate nodes that share a
    # resource. Picks a random node with at least one edge and returns its
    # whole connected component (BFS/DFS) -- the set of events that flip
    # t1<->t2 together in one Kempe chain move.
    adjacency: dict[int, list[int]] = {i: [] for i in resource_ids}
    for a, b in itertools.combinations(resource_ids, 2):
        if resource_ids[a] & resource_ids[b]:
            adjacency[a].append(b)
            adjacency[b].append(a)

    connected = [i for i in resource_ids if adjacency[i]]
    if not connected:
        raise ValueError("no two events at the chosen times share a resource to chain")

    component: set[int] = set()
    stack = [rng.choice(connected)]
    while stack:
        node = stack.pop()
        if node in component:
            continue
        component.add(node)
        stack.extend(n for n in adjacency[node] if n not in component)
    return component


def kempe_chain_move(
    instance: Instance, solution: Solution, rng: random.Random
) -> Solution:
    """Kempe chain interchange, borrowed from graph-colouring and exam/course
    timetabling literature (e.g. Burke, Elliman & Weare 1994): picks two
    times t1/t2, builds the conflict graph over solution events currently
    placed at either one -- an edge connects two events that share an
    assigned resource (fixed on the Event definition or chosen in the
    Solution), since placing both at the same time would clash -- then
    flips every event in ONE connected component from t1<->t2 in a single
    move. Unlike time_swap_move (always exactly 2 events), this can move an
    entire linked chain at once, useful for reaching states a sequence of
    single-event moves can't reach without passing through a worse
    intermediate step. Returns a new Solution; the input is untouched.

    Only considers events whose duration fits (non-overflowing, same-day)
    at BOTH t1 and t2 -- same guard as time_swap_move -- so every flip in
    the chosen component is guaranteed structurally valid.

    Raises ValueError if the instance has fewer than 2 times, fewer than 2
    solution events are both eligible at the two chosen times, or none of
    those events share a resource with another (nothing to chain)."""
    if len(instance.times) < 2:
        raise ValueError("instance has fewer than 2 times to build a Kempe chain")
    t1, t2 = rng.sample([t.id for t in instance.times], 2)

    resource_ids = _kempe_candidate_nodes(instance, solution, t1, t2)
    if len(resource_ids) < 2:
        raise ValueError("fewer than 2 events at the chosen times are eligible to swap")
    component = _kempe_component(resource_ids, rng)

    new_events = list(solution.events)
    for i in component:
        se = new_events[i]
        new_events[i] = replace(se, time_ref=t2 if se.time_ref == t1 else t1)
    return replace(solution, events=new_events)


_LARGE_PERTURBATION_FRACTION = 0.3
_LARGE_PERTURBATION_MIN_EVENTS = 4


def large_perturbation_move(
    instance: Instance, solution: Solution, rng: random.Random
) -> Solution:
    """Large random perturbation meant to kick LAHC out of a local minimum,
    as opposed to ruin_and_recreate's small, greedily-rebuilt portion:
    picks about _LARGE_PERTURBATION_FRACTION of the solution's movable
    events (floored at _LARGE_PERTURBATION_MIN_EVENTS, capped at however
    many movable events actually exist -- unlike ruin_and_recreate there
    is no upper cap otherwise, which is what makes this "large") and
    reassigns each, independently, to a uniformly random valid start time.
    No cost-minimizing recreate step -- O(1) evaluator-free work per
    reassigned event, instead of ruin_and_recreate's O(valid times)
    total_cost calls -- which is what makes this operator cheap enough to
    touch a large fraction of the solution in one call. Returns a new
    Solution; the input is untouched.

    Because it operates on whatever Solution it's given, the exact same
    function can later be pointed at the solver's best-known solution
    instead of its current one, to implement a stagnation-triggered
    "restart from best" step once the solver loop grows one (Etap 5/6) --
    no separate code path needed here, just a different caller.

    Raises ValueError if the solution has no movable event (a time_ref and
    a duration set -- see time_reassign_move) to perturb, or if one of the
    chosen movable events has no valid start time at all."""
    movable = [
        i
        for i, se in enumerate(solution.events)
        if se.time_ref is not None and se.duration is not None
    ]
    if not movable:
        raise ValueError(
            "cannot apply a move: no solution event has a time to reassign"
        )

    k = min(
        len(movable),
        max(
            _LARGE_PERTURBATION_MIN_EVENTS,
            round(len(movable) * _LARGE_PERTURBATION_FRACTION),
        ),
    )
    chosen = rng.sample(movable, k)

    new_events = list(solution.events)
    for i in chosen:
        se = new_events[i]
        assert se.duration is not None
        candidates = valid_start_time_ids(instance, se.duration)
        if not candidates:
            raise ValueError(f"event {se.event_ref!r} has no valid start time")
        other_times = [t for t in candidates if t != se.time_ref]
        if not other_times:
            raise ValueError(
                f"event {se.event_ref!r} has no alternative valid start time"
            )
        new_events[i] = replace(se, time_ref=rng.choice(other_times))
    return replace(solution, events=new_events)
