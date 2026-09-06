import random

from xhstt_core.evaluator_ref import _events_in_applies_to, valid_start_time_ids
from xhstt_core.model import (
    Event,
    Instance,
    Solution,
    SolutionEvent,
    SolutionEventResource,
)


def _split_duration_bounds(instance: Instance, event: Event) -> tuple[int, int]:
    """Finds MinimumDuration/MaximumDuration from a SplitEventsConstraint
    applying to this event, if any. Without one, the event isn't meant to
    be split -- (1, event.duration) yields a single piece covering the
    whole duration, matching the pre-split-aware behavior."""
    for c in instance.constraints:
        if c.type != "SplitEventsConstraint":
            continue
        if event.id not in _events_in_applies_to(instance, c.applies_to):
            continue
        min_d = int(c.params.get("MinimumDuration", 1))
        max_d = int(c.params.get("MaximumDuration", event.duration))
        return min_d, max_d
    return 1, event.duration


def _split_durations(total: int, min_duration: int, max_duration: int) -> list[int]:
    """Breaks `total` into pieces each within [min_duration, max_duration]
    summing back to `total`, preferring fewer/larger pieces (e.g. double
    periods over singles) -- the pattern real reference solutions use
    (confirmed against BrazilInstance1's HSEval-validated solutions)."""
    if total <= max_duration:
        return [total]
    pieces = []
    remaining = total
    while remaining > max_duration:
        pieces.append(max_duration)
        remaining -= max_duration
    if remaining < min_duration and pieces:
        # Top up the too-small remainder by borrowing from the last full
        # piece, so every piece respects MinimumDuration.
        deficit = min_duration - remaining
        pieces[-1] -= deficit
        remaining += deficit
    pieces.append(remaining)
    return pieces


def build_initial(instance: Instance, rng: random.Random) -> Solution:
    """Naive greedy constructor: every event slot (time or event resource)
    left unassigned by the Instance gets an arbitrary, structurally-valid
    (matching ResourceType, non-overflowing time span, split according to
    any applicable SplitEventsConstraint) but constraint-blind assignment.
    Produces a *complete* solution (no None left anywhere) with no regard
    for constraint cost -- the starting point for local-search improvement
    (LAHC etc.), not a solution in its own right.

    Events that are already fully preassigned (time and every resource)
    are skipped entirely, matching the real-file convention (confirmed via
    HSEval reference solutions) that such events need no explicit
    SolutionEvent entry."""
    resources_by_type: dict[str, list[str]] = {}
    for r in instance.resources:
        resources_by_type.setdefault(r.resource_type_ref, []).append(r.id)

    events = []
    for event in instance.events:
        needs_time = event.time_ref is None
        unassigned_roles = [er for er in event.resources if er.resource_ref is None]
        if not needs_time and not unassigned_roles:
            continue

        # One resource choice per unassigned role, reused across every
        # split piece of this event -- keeps a course's teacher/room
        # stable by default, a better starting point for
        # AvoidSplitAssignmentsConstraint than re-rolling per piece.
        role_choices = {
            er.role: rng.choice(resources_by_type[er.resource_type_ref])
            for er in unassigned_roles
        }

        def _fresh_resources(
            role_choices: dict[str, str] = role_choices,
        ) -> list[SolutionEventResource]:
            # A new list of new SolutionEventResource instances each call,
            # so sub-events sharing the same role->resource choice don't
            # share mutable objects a later in-place edit could corrupt.
            # role_choices is a default arg (not a closure over the loop
            # variable) so its value is bound at definition time -- see
            # ruff B023.
            return [
                SolutionEventResource(role=role, resource_ref=ref)
                for role, ref in role_choices.items()
            ]

        if not needs_time:
            events.append(
                SolutionEvent(
                    event_ref=event.id, time_ref=None, resources=_fresh_resources()
                )
            )
            continue

        min_d, max_d = _split_duration_bounds(instance, event)
        for duration in _split_durations(event.duration, min_d, max_d):
            candidates = valid_start_time_ids(instance, duration)
            time_ref = (
                rng.choice(candidates) if candidates else rng.choice(instance.times).id
            )
            events.append(
                SolutionEvent(
                    event_ref=event.id,
                    time_ref=time_ref,
                    duration=duration,
                    resources=_fresh_resources(),
                )
            )

    return Solution(instance_ref=instance.id, events=events)
