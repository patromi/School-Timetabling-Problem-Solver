from dataclasses import dataclass
from html import escape

from src.evaluator_ref import Occurrence, evaluate_constraint
from src.model import Instance

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


def _render_eval_row(score: ConstraintScore) -> str:
    row_class = "row--bad" if score.cost > 0 else "row--ok"
    hidden_attr = "" if score.cost > 0 else " hidden"
    return (
        f'<tr class="{row_class}"{hidden_attr}>'
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


