import random
from dataclasses import replace

from src.evaluator_ref import (
    Occurrence,
    _events_in_applies_to,
    _preferred_time_ids,
    resolve_occurrence,
    resolve_occurrences,
    valid_start_time_ids,
)
from src.model import (
    Event,
    Instance,
    Solution,
    SolutionEvent,
    SolutionEventResource,
)
from src.resource_domains import assignment_group, shared_domain, soft_preferred


def _split_duration_bounds(instance: Instance, event: Event) -> tuple[int, int]:
    """Returns (min_duration, max_duration) for splitting this event's total
    duration into sub-event pieces.

    Priority order:
    1. SplitEventsConstraint — explicit structural split rule; takes precedence.
    2. PreferTimesConstraint with Duration filter — if a Required constraint
       says a given sub-event duration has *no* preferred times (empty set),
       treat that duration as forbidden and step down to the next valid size.
       Handles instances like AU-SA-96 where splitting is implied by
       PreferTimesConstraint rather than stated directly.
    3. Fallback: single piece covering the whole duration."""
    for c in instance.constraints:
        if c.type != "SplitEventsConstraint":
            continue
        if event.id not in _events_in_applies_to(instance, c.applies_to):
            continue
        min_d = int(c.params.get("MinimumDuration", 1))
        max_d = int(c.params.get("MaximumDuration", event.duration))
        return min_d, max_d

    forbidden: set[int] = set()
    for c in instance.constraints:
        if c.type != "PreferTimesConstraint" or not c.required:
            continue
        dur_param = c.params.get("Duration")
        if dur_param is None:
            continue
        if event.id not in _events_in_applies_to(instance, c.applies_to):
            continue
        if not _preferred_time_ids(instance, c):
            forbidden.add(int(dur_param))

    max_d = event.duration
    while max_d in forbidden and max_d > 1:
        max_d -= 1
    return 1, max_d


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
        deficit = min_duration - remaining
        pieces[-1] -= deficit
        remaining += deficit
    pieces.append(remaining)
    return pieces


def build_initial(instance: Instance, rng: random.Random) -> Solution:
    """Naive greedy constructor: every event slot (time or event resource)
    left unassigned by the Instance gets an arbitrary, structurally-valid
    (resource from resource_domains.resource_domain, non-overflowing time
    span, split according to any applicable SplitEventsConstraint) but
    otherwise constraint-blind assignment.
    Produces a *complete* solution (no None left anywhere) with no regard
    for constraint cost -- the starting point for local-search improvement
    (LAHC etc.), not a solution in its own right.

    Events that are already fully preassigned (time and every resource)
    are skipped entirely, matching the real-file convention (confirmed via
    HSEval reference solutions) that such events need no explicit
    SolutionEvent entry."""
    group_choices: dict[tuple[str, frozenset[str]], str] = {}

    events = []
    for event in instance.events:
        needs_time = event.time_ref is None
        unassigned_roles = [er for er in event.resources if er.resource_ref is None]
        if not needs_time and not unassigned_roles:
            continue

        # One choice per role and assignment group, reused across splits and
        # across the group's events -- the start AvoidSplitAssignments wants.
        role_choices = {}
        for er in unassigned_roles:
            group = assignment_group(instance, event.id, er.role)
            key = (er.role, group)
            if key not in group_choices:
                group_choices[key] = rng.choice(shared_domain(instance, group, er.role))
            role_choices[er.role] = group_choices[key]

        def _fresh_resources(
            # Default arg (not closure): binds role_choices at definition time (ruff B023).
            # New list each call so split pieces don't share mutable SolutionEventResource objects.
            role_choices: dict[str, str] = role_choices,
        ) -> list[SolutionEventResource]:
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


def build_greedy(instance: Instance, rng: random.Random) -> Solution:
    """Clash-aware constructor: the same event pieces and resources as
    `build_initial`, but each piece is re-timed to avoid clashes.

    Starts from `build_initial` (so splitting and resource choices stay
    identical), then places the pieces one by one, hardest first -- longest
    duration, then most resources, the classic largest-degree-first order of
    graph colouring -- each on the valid start time where its resources
    overlap the fewest pieces already placed (ties broken at random).
    Events with a preassigned time are in the occupancy table from the
    start, since they block their slots for everyone else.

    Only clashes guide the choice: diagnostics on the in-scope instances
    show AvoidClashesConstraint as the dominant hard violation, and every
    other constraint is left to local search.

    Assignable resources are chosen here too, when the first piece of their
    assignment group (resource_domains.assignment_group) is placed: the
    piece is timed by its preassigned resources, then each open role gets
    the domain resource with the fewest overlaps at that time, preferring
    soft-preferred and then less loaded resources."""
    base = build_initial(instance, rng)
    table = _Bookings(instance, rng)
    event_defs = table.event_defs

    movable = [
        i
        for i, se in enumerate(base.events)
        if se.time_ref is not None and se.duration is not None
    ]
    movable_refs = {base.events[i].event_ref for i in movable}

    for occurrence in resolve_occurrences(instance, base):
        if occurrence.event_ref not in movable_refs and occurrence.time_ref:
            table.book(
                table.preassigned(occurrence.event_ref),
                occurrence.time_ref,
                occurrence.duration,
            )

    events = list(base.events)
    # Fixed-time events that only need resources: their time is known upfront.
    for i, se in enumerate(base.events):
        fixed_time = event_defs[se.event_ref].time_ref
        if se.time_ref is None and se.resources and fixed_time is not None:
            events[i] = table.place(se, fixed_time, event_defs[se.event_ref].duration)

    def difficulty(i: int) -> tuple[int, int]:
        se = base.events[i]
        occurrence = resolve_occurrence(event_defs[se.event_ref], se)
        return (se.duration or 0, len(_resources_of(occurrence)))

    order = list(movable)
    rng.shuffle(order)  # random order among equally hard pieces
    order.sort(key=difficulty, reverse=True)

    for i in order:
        se = events[i]
        assert se.duration is not None and se.time_ref is not None
        time_ref = se.time_ref
        resources = table.known_resources(se)
        candidates = list(valid_start_time_ids(instance, se.duration))
        if candidates:
            counts = {t: table.overlaps(resources, t, se.duration) for t in candidates}
            fewest = min(counts.values())
            time_ref = rng.choice([t for t in candidates if counts[t] == fewest])
        events[i] = table.place(se, time_ref, se.duration)

    return replace(base, events=events)


def _resources_of(occurrence: Occurrence) -> list[str]:
    return [r for _, r in occurrence.resource_assignments if r is not None]


class _Bookings:
    """build_greedy's state: how many placed pieces use each (resource,
    period), each resource's booked periods, and the resource chosen for
    each (role, assignment group) once its first piece is placed."""

    def __init__(self, instance: Instance, rng: random.Random) -> None:
        self.instance = instance
        self.rng = rng
        self.position = {t.id: k for k, t in enumerate(instance.times)}
        self.event_defs = {e.id: e for e in instance.events}
        self.busy: dict[tuple[str, int], int] = {}
        self.load: dict[str, int] = {}
        self.chosen: dict[tuple[str, frozenset[str]], str] = {}

    def book(self, resources: list[str], time_ref: str, duration: int) -> None:
        start = self.position[time_ref]
        for r in resources:
            self.load[r] = self.load.get(r, 0) + duration
            for p in range(start, start + duration):
                self.busy[(r, p)] = self.busy.get((r, p), 0) + 1

    def overlaps(self, resources: list[str], time_ref: str, duration: int) -> int:
        return _overlaps(self.busy, resources, self.position[time_ref], duration)

    def preassigned(self, event_ref: str) -> list[str]:
        return [
            er.resource_ref
            for er in self.event_defs[event_ref].resources
            if er.resource_ref is not None
        ]

    def _key(self, event_ref: str, role: str) -> tuple[str, frozenset[str]]:
        return (role, assignment_group(self.instance, event_ref, role))

    def known_resources(self, se: SolutionEvent) -> list[str]:
        """Preassigned resources plus those already chosen for its roles."""
        known = self.preassigned(se.event_ref)
        for sr in se.resources:
            key = self._key(se.event_ref, sr.role)
            if key in self.chosen:
                known.append(self.chosen[key])
        return known

    def place(self, se: SolutionEvent, time_ref: str, duration: int) -> SolutionEvent:
        """`se` at `time_ref`, its still open roles filled with the domain
        resource that overlaps the fewest placed pieces there; booked."""
        taken = self.preassigned(se.event_ref)
        resources = []
        for sr in se.resources:
            key = self._key(se.event_ref, sr.role)
            if key not in self.chosen:
                self.chosen[key] = self._pick(
                    se.event_ref, sr.role, taken, time_ref, duration
                )
            taken.append(self.chosen[key])
            resources.append(
                SolutionEventResource(role=sr.role, resource_ref=self.chosen[key])
            )
        self.book(taken, time_ref, duration)
        if se.time_ref is None:
            return replace(se, resources=resources) if resources else se
        return replace(se, time_ref=time_ref, resources=resources)

    def _pick(
        self, event_ref: str, role: str, taken: list[str], time_ref: str, duration: int
    ) -> str:
        preferred = soft_preferred(self.instance, event_ref, role)
        group = assignment_group(self.instance, event_ref, role)
        scores = {
            r: (
                self.overlaps([r], time_ref, duration) + taken.count(r),
                r not in preferred,
                self.load.get(r, 0),
            )
            for r in shared_domain(self.instance, group, role)
        }
        best = min(scores.values())
        return self.rng.choice([r for r, s in scores.items() if s == best])


def _overlaps(
    busy: dict[tuple[str, int], int], resources: list[str], start: int, duration: int
) -> int:
    """Pieces already booked on any of `resources` during [start, start +
    duration) -- the clashes a new piece placed there would cause."""
    return sum(
        busy.get((r, p), 0) for r in resources for p in range(start, start + duration)
    )
