import random
from dataclasses import replace

from xhstt_core.evaluator_ref import valid_start_time_ids
from xhstt_core.model import Instance, Solution, SolutionEventResource

# time_swap_move retries a bounded number of random pairs looking for one
# where swapping keeps BOTH events within a valid (non-overflowing,
# same-day) time span, rather than enumerating all O(n^2) pairs.
_MAX_SWAP_ATTEMPTS = 20


def time_reassign_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution:
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

    Uses structural sharing rather than a deep copy: only the touched
    SolutionEvent is rebuilt (via dataclasses.replace), every other event
    keeps the exact same object reference. Confirmed as a real hot spot by
    profiling on a large real instance (AU-BG-98): deepcopy of the whole
    Solution -- events, their resource lists, everything -- on every
    single move accounted for ~4.6s of every 23.8s spent on 100 LAHC
    iterations."""
    if not solution.events:
        raise ValueError("cannot apply a move to a solution with no solution events")
    # rng.randrange(len(seq)) draws the same way rng.choice(seq) would
    # (both bottom out in Random._randbelow), so this keeps the exact same
    # random-number consumption sequence as picking via rng.choice.
    index = rng.randrange(len(solution.events))
    event = solution.events[index]
    other_times = [
        t for t in valid_start_time_ids(instance, event.duration) if t != event.time_ref
    ]
    if not other_times:
        raise ValueError("instance has no alternative valid time to reassign to")
    new_event = replace(event, time_ref=rng.choice(other_times))
    new_events = list(solution.events)
    new_events[index] = new_event
    return replace(solution, events=new_events)


def time_swap_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution:
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
        if b.time_ref not in valid_start_time_ids(instance, a.duration):
            continue
        if a.time_ref not in valid_start_time_ids(instance, b.duration):
            continue
        new_events = list(solution.events)
        new_events[i] = replace(a, time_ref=b.time_ref)
        new_events[j] = replace(b, time_ref=a.time_ref)
        return replace(solution, events=new_events)
    raise ValueError("no valid time swap found within the attempt budget")


def resource_reassign_move(instance: Instance, solution: Solution, rng: random.Random) -> Solution:
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
    alternatives = [r for r in resources_by_type[type_ref] if r != target_resource.resource_ref]
    new_resource = SolutionEventResource(role=target_resource.role, resource_ref=rng.choice(alternatives))

    new_resources = list(target_event.resources)
    new_resources[r_index] = new_resource
    new_events = list(solution.events)
    new_events[se_index] = replace(target_event, resources=new_resources)
    return replace(solution, events=new_events)
