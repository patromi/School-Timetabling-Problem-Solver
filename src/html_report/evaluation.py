import math
from collections import Counter
from dataclasses import dataclass
from html import escape

from src.evaluator_ref import (
    Occurrence,
    _assigned_resource,
    _assigned_resource_ids,
    _event_group_members,
    _event_resource_workload,
    _events_in_applies_to,
    _preferred_resource_ids,
    _preferred_time_ids,
    _referenced_ids,
    _resources_in_applies_to,
    _shortfall_or_excess,
    evaluate_constraint,
)
from src.evaluator_ref.constraints import (
    _busy_count_in_group,
    _first_and_last_occupied_indices,
    _idle_count_in_group,
    _time_group_refs,
)
from src.evaluator_ref.occurrences import _full_span_busy_times, _occupied_time_ids
from src.model import Constraint, Instance

@dataclass
class ConstraintScore:
    id: str
    name: str
    type: str
    required: bool
    weight: int
    cost_function: str
    cost: int


def build_constraint_scores(
    instance: Instance, occurrences: list[Occurrence]
) -> list[ConstraintScore]:
    """One score per instance constraint, in file order -- callers sort
    and group for display as needed."""
    return [
        ConstraintScore(
            id=c.id,
            name=c.name,
            type=c.type,
            required=c.required,
            weight=c.weight,
            cost_function=c.cost_function,
            cost=evaluate_constraint(instance, occurrences, c),
        )
        for c in instance.constraints
    ]


def _events_for_resource_in_times(
    instance: Instance,
    occurrences: list[Occurrence],
    resource_id: str,
    target_times: set[str] | None = None,
) -> list[str]:
    """Events attended by `resource_id`, optionally filtered to those occupying `target_times`."""
    refs: list[str] = []
    for o in occurrences:
        if resource_id not in _assigned_resource_ids(o):
            continue
        if target_times is None:
            refs.append(o.event_ref)
            continue
        if o.time_ref is not None and (
            _occupied_time_ids(instance, o.time_ref, o.duration) & target_times
        ):
            refs.append(o.event_ref)
    return refs


def _violating_assign_time(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    return list(
        {
            o.event_ref
            for o in occurrences
            if o.event_ref in event_ids and o.time_ref is None
        }
    )


def _violating_avoid_clashes(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    result: list[str] = []
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        time_to_events: dict[str, list[str]] = {}
        for o in occurrences:
            if o.time_ref is None or resource_id not in _assigned_resource_ids(o):
                continue
            for t in _occupied_time_ids(instance, o.time_ref, o.duration):
                time_to_events.setdefault(t, []).append(o.event_ref)
        for events in time_to_events.values():
            if len(events) > 1:
                result.extend(events)
    return result


def _violating_assign_resource(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    role = constraint.params.get("Role")
    result: list[str] = []
    for event in instance.events:
        if event.id not in event_ids:
            continue
        if not any(er.role == role and er.resource_ref is None for er in event.resources):
            continue
        if any(
            o.event_ref == event.id and _assigned_resource(o, role) is None
            for o in occurrences
        ):
            result.append(event.id)
    return result


def _violating_prefer_times(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    preferred = _preferred_time_ids(instance, constraint)
    duration_filter = constraint.params.get("Duration")
    result: list[str] = []
    for o in occurrences:
        if o.event_ref not in event_ids:
            continue
        if duration_filter is not None and o.duration != int(duration_filter):
            continue
        if o.time_ref is not None and o.time_ref not in preferred:
            result.append(o.event_ref)
    return result


def _violating_prefer_resources(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    role = constraint.params.get("Role")
    preferred = _preferred_resource_ids(instance, constraint)
    result: list[str] = []
    for o in occurrences:
        if o.event_ref not in event_ids:
            continue
        assigned = _assigned_resource(o, role)
        if assigned is not None and assigned not in preferred:
            result.append(o.event_ref)
    return result


def _violating_avoid_unavailable(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    unavailable = _referenced_ids(constraint.params.get("Times"))
    result: list[str] = []
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        result.extend(
            _events_for_resource_in_times(instance, occurrences, resource_id, unavailable)
        )
    return result


def _violating_limit_idle_times(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    result: list[str] = []
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy = _full_span_busy_times(instance, occurrences, resource_id)
        idle_per_group = {
            g: _idle_count_in_group(instance, busy, g) for g in target_groups
        }
        idle_total = sum(idle_per_group.values())
        if _shortfall_or_excess(idle_total, minimum, maximum) == 0:
            continue
        if idle_total > maximum:
            bad_groups = {g for g, cnt in idle_per_group.items() if cnt > 0}
        else:
            bad_groups = target_groups
        target_times = {
            t.id for t in instance.times if bad_groups & set(t.group_refs)
        }
        result.extend(
            _events_for_resource_in_times(instance, occurrences, resource_id, target_times)
        )
    return result


def _violating_limit_busy_times(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    result: list[str] = []
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy = _full_span_busy_times(instance, occurrences, resource_id)
        bad_groups = {
            g
            for g in target_groups
            if (cnt := _busy_count_in_group(instance, busy, g)) != 0
            and _shortfall_or_excess(cnt, minimum, maximum) > 0
        }
        if not bad_groups:
            continue
        target_times = {
            t.id for t in instance.times if bad_groups & set(t.group_refs)
        }
        result.extend(
            _events_for_resource_in_times(instance, occurrences, resource_id, target_times)
        )
    return result


def _violating_cluster_busy_times(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    target_groups = _referenced_ids(constraint.params.get("TimeGroups"))
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    target_times = {t.id for t in instance.times if target_groups & set(t.group_refs)}
    result: list[str] = []
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        busy = _full_span_busy_times(instance, occurrences, resource_id)
        active_groups = {g for t in busy for g in _time_group_refs(instance, t)}
        if _shortfall_or_excess(len(active_groups & target_groups), minimum, maximum) > 0:
            result.extend(
                _events_for_resource_in_times(
                    instance, occurrences, resource_id, target_times or None
                )
            )
    return result


def _violating_limit_workload(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    events_by_id = {e.id: e for e in instance.events}
    result: list[str] = []
    for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
        workload_sum = 0
        resource_events: list[str] = []
        for o in occurrences:
            event_def = events_by_id[o.event_ref]
            for role, assigned in o.resource_assignments:
                if assigned != resource_id:
                    continue
                workload = _event_resource_workload(event_def, role)
                workload_sum += math.ceil(workload * (o.duration / event_def.duration))
                resource_events.append(o.event_ref)
        if _shortfall_or_excess(workload_sum, minimum, maximum) > 0:
            result.extend(resource_events)
    return result


def _violating_split_events(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    min_duration = int(constraint.params.get("MinimumDuration", 0))
    max_duration = int(constraint.params.get("MaximumDuration", 10**9))
    min_amount = int(constraint.params.get("MinimumAmount", 0))
    max_amount = int(constraint.params.get("MaximumAmount", 10**9))
    result: list[str] = []
    for event in instance.events:
        if event.id not in event_ids:
            continue
        subs = [o for o in occurrences if o.event_ref == event.id]
        if _shortfall_or_excess(len(subs), min_amount, max_amount) > 0 or any(
            not (min_duration <= o.duration <= max_duration) for o in subs
        ):
            result.append(event.id)
    return result


def _violating_distribute_split_events(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    event_ids = _events_in_applies_to(instance, constraint.applies_to)
    target_duration = int(constraint.params["Duration"])
    minimum = int(constraint.params.get("Minimum", 0))
    maximum = int(constraint.params.get("Maximum", 10**9))
    result: list[str] = []
    for event in instance.events:
        if event.id not in event_ids:
            continue
        count = sum(
            1
            for o in occurrences
            if o.event_ref == event.id and o.duration == target_duration
        )
        if _shortfall_or_excess(count, minimum, maximum) > 0:
            result.append(event.id)
    return result


def _violating_spread_events(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    result: list[str] = []
    for group_ref in constraint.applies_to.event_groups:
        member_ids = _event_group_members(instance, group_ref)
        subs = [o for o in occurrences if o.event_ref in member_ids]
        for entry in constraint.params.get("TimeGroups", []):
            tg_ref = entry["reference"]
            minimum = int(entry.get("Minimum", 0))
            maximum = int(entry.get("Maximum", 10**9))
            matching = [
                o
                for o in subs
                if o.time_ref is not None
                and tg_ref in _time_group_refs(instance, o.time_ref)
            ]
            count = len(matching)
            if count > maximum:
                result.extend(o.event_ref for o in matching)
            elif count < minimum:
                result.extend(member_ids)
    return result


def _violating_avoid_split_assignments(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    role = constraint.params.get("Role")
    result: list[str] = []
    for group_ref in constraint.applies_to.event_groups:
        member_ids = _event_group_members(instance, group_ref)
        group_occs = [
            o
            for o in occurrences
            if o.event_ref in member_ids and _assigned_resource(o, role) is not None
        ]
        assigned = {_assigned_resource(o, role) for o in group_occs}
        if len(assigned) > 1:
            result.extend(o.event_ref for o in group_occs)
    return result


def _violating_link_events(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    result: list[str] = []
    for group_ref in constraint.applies_to.event_groups:
        member_ids = _event_group_members(instance, group_ref)
        per_event_times = []
        for event_id in member_ids:
            times_for_event: set[str] = set()
            for o in occurrences:
                if o.event_ref == event_id:
                    times_for_event |= _occupied_time_ids(
                        instance, o.time_ref, o.duration
                    )
            per_event_times.append(times_for_event)
        all_times = set().union(*per_event_times) if per_event_times else set()
        if any(not all(t in s for s in per_event_times) for t in all_times):
            result.extend(member_ids)
    return result


def _violating_order_events(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    result: list[str] = []
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
        if _shortfall_or_excess(separation, pair.min_separation, maximum) > 0:
            result.extend([pair.first_event, pair.second_event])
    return result


_VIOLATION_FINDERS = {
    "AssignTimeConstraint": _violating_assign_time,
    "AvoidClashesConstraint": _violating_avoid_clashes,
    "AssignResourceConstraint": _violating_assign_resource,
    "PreferTimesConstraint": _violating_prefer_times,
    "PreferResourcesConstraint": _violating_prefer_resources,
    "AvoidUnavailableTimesConstraint": _violating_avoid_unavailable,
    "LimitIdleTimesConstraint": _violating_limit_idle_times,
    "LimitBusyTimesConstraint": _violating_limit_busy_times,
    "ClusterBusyTimesConstraint": _violating_cluster_busy_times,
    "LimitWorkloadConstraint": _violating_limit_workload,
    "SplitEventsConstraint": _violating_split_events,
    "DistributeSplitEventsConstraint": _violating_distribute_split_events,
    "SpreadEventsConstraint": _violating_spread_events,
    "AvoidSplitAssignmentsConstraint": _violating_avoid_split_assignments,
    "LinkEventsConstraint": _violating_link_events,
    "OrderEventsConstraint": _violating_order_events,
}


def violation_event_refs(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    """Returns deduplicated event_refs contributing to this constraint's violation."""
    finder = _VIOLATION_FINDERS.get(constraint.type)
    if finder is not None:
        return list(dict.fromkeys(finder(instance, occurrences, constraint)))
    return list(_events_in_applies_to(instance, constraint.applies_to))


def _render_eval_row(score: ConstraintScore) -> str:
    row_class = "row--bad" if score.cost > 0 else "row--ok"
    hidden_attr = "" if score.cost > 0 else " hidden"
    constraint_attr = (
        f' data-constraint="{escape(score.id)}" title="Najedź, aby zobaczyć naruszenia na planie"'
        if score.cost > 0 else ""
    )
    return (
        f'<tr class="{row_class}"{hidden_attr}{constraint_attr}>'
        f"<td>{escape(score.name)}</td>"
        f'<td class="mono">{escape(score.type)}</td>'
        f'<td class="mono num">{score.weight}</td>'
        f'<td class="mono">{escape(score.cost_function)}</td>'
        f'<td class="mono num">{score.cost}</td>'
        f"</tr>"
    )


def _render_eval_group(
    kind: str, title: str, scores: list[ConstraintScore], total: int
) -> str:
    if not scores:
        label = "wymaganych" if kind == "required" else "preferowanych"
        return (
            f'<div class="eval-group" data-kind="{kind}">'
            f"<h3>{escape(title)}</h3>"
            f'<p class="eval-empty">Brak ograniczeń {label} w tej instancji.</p>'
            f"</div>"
        )
    ordered = sorted(scores, key=lambda s: (-s.cost, s.name))
    violated = sum(1 for s in ordered if s.cost > 0)
    rows = "".join(_render_eval_row(s) for s in ordered)
    return f"""<div class="eval-group" data-kind="{kind}">
  <div class="eval-group-head">
    <h3>{escape(title)}</h3>
    <p class="eval-stat">{violated}/{len(ordered)} naruszonych &middot;
      suma = {total}</p>
    <button class="eval-toggle" type="button" data-kind="{kind}"
      data-total="{len(ordered)}" data-violated="{violated}"
      data-expanded="false">Pokaż wszystkie ({len(ordered)})</button>
  </div>
  <table class="eval-table">
    <thead><tr><th>Nazwa</th><th>Typ</th><th>Waga</th>
      <th>Funkcja kosztu</th><th>Koszt</th></tr></thead>
    <tbody>{rows}</tbody>
  </table>
</div>"""


def render_evaluation_section(
    scores: list[ConstraintScore], infeasibility: int, objective: int
) -> str:
    required = [s for s in scores if s.required]
    preferred = [s for s in scores if not s.required]
    return f"""<section class="evaluation">
  <h2>Ocena rozwiązania</h2>
  {_render_eval_group("required", "Ograniczenia wymagane", required, infeasibility)}
  {_render_eval_group("preferred", "Ograniczenia preferowane", preferred, objective)}
</section>"""


