from collections import Counter
from dataclasses import dataclass
from html import escape

from src.evaluator_ref import (
    Occurrence,
    _assigned_resource,
    _assigned_resource_ids,
    _events_in_applies_to,
    _preferred_resource_ids,
    _preferred_time_ids,
    _referenced_ids,
    _resources_in_applies_to,
    evaluate_constraint,
)
from src.evaluator_ref.occurrences import _occupied_time_ids
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


def violation_event_refs(
    instance: Instance, occurrences: list[Occurrence], constraint: Constraint
) -> list[str]:
    """Returns event_refs that contribute to this constraint's violation.
    Precise for the most common types; falls back to applies_to scope for others."""
    ctype = constraint.type

    if ctype == "AssignTimeConstraint":
        event_ids = _events_in_applies_to(instance, constraint.applies_to)
        return list({o.event_ref for o in occurrences
                     if o.event_ref in event_ids and o.time_ref is None})

    if ctype == "AvoidClashesConstraint":
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
        return list(dict.fromkeys(result))

    if ctype == "PreferTimesConstraint":
        event_ids = _events_in_applies_to(instance, constraint.applies_to)
        preferred = _preferred_time_ids(instance, constraint)
        duration_filter = constraint.params.get("Duration")
        result = []
        for o in occurrences:
            if o.event_ref not in event_ids:
                continue
            if duration_filter is not None and o.duration != int(duration_filter):
                continue
            if o.time_ref is not None and o.time_ref not in preferred:
                result.append(o.event_ref)
        return list(dict.fromkeys(result))

    if ctype == "PreferResourcesConstraint":
        event_ids = _events_in_applies_to(instance, constraint.applies_to)
        role = constraint.params.get("Role")
        preferred = _preferred_resource_ids(instance, constraint)
        result = []
        for o in occurrences:
            if o.event_ref not in event_ids:
                continue
            assigned = _assigned_resource(o, role)
            if assigned is not None and assigned not in preferred:
                result.append(o.event_ref)
        return list(dict.fromkeys(result))

    if ctype == "AvoidUnavailableTimesConstraint":
        unavailable = _referenced_ids(constraint.params.get("Times"))
        result = []
        for resource_id in _resources_in_applies_to(instance, constraint.applies_to):
            for o in occurrences:
                if o.time_ref is None or resource_id not in _assigned_resource_ids(o):
                    continue
                if _occupied_time_ids(instance, o.time_ref, o.duration) & unavailable:
                    result.append(o.event_ref)
        return list(dict.fromkeys(result))

    # Fallback: return events in applies_to scope (best-effort for resource-centric types)
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


