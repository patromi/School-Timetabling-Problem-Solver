import json
from html import escape
from pathlib import Path

from src.evaluator_ref import Occurrence
from src.model import Instance

from src.html_report.evaluation import (
    ConstraintScore,
    build_constraint_scores,
    render_evaluation_section,
    violation_event_refs,
)
from src.html_report.grid import (
    DayColumn,
    TimetableCell,
    _render_resource_table,
    build_days,
    build_resource_grid,
)

_ASSETS_DIR = Path(__file__).parent.parent / "assets"
_PAGE_CSS = (_ASSETS_DIR / "timetable.css").read_text(encoding="utf-8")

def render_timetable_page(
    instance: Instance,
    occurrences: list[Occurrence],
    infeasibility: int,
    objective: int,
) -> str:
    """Builds a complete, self-contained HTML page: a resource-type/
    resource picker plus one day-by-period timetable grid per resource,
    switched client-side (no server, no external assets)."""
    days = build_days(instance)
    fonts_css = (_ASSETS_DIR / "fonts.css").read_text(encoding="utf-8")

    resources_by_type: dict[str, list] = {}
    for r in instance.resources:
        resources_by_type.setdefault(r.resource_type_ref, []).append(r)
    type_order = [rt.id for rt in instance.resource_types if rt.id in resources_by_type]

    type_tabs = "".join(
        f'<button class="type-tab" data-type="{escape(t)}">{escape(t)}</button>'
        for t in type_order
    )
    chips = "".join(
        f'<button class="chip" data-type="{escape(rt)}" data-resource="{escape(r.id)}">'
        f"{escape(r.name)}</button>"
        for rt in type_order
        for r in sorted(resources_by_type[rt], key=lambda r: r.name)
    )
    tables = "".join(
        _render_resource_table(
            instance,
            days,
            build_resource_grid(instance, occurrences, r.id),
            rt,
            r.id,
            r.name,
        )
        for rt in type_order
        for r in sorted(resources_by_type[rt], key=lambda r: r.name)
    )

    feasible = infeasibility == 0
    status_class = "ok" if feasible else "bad"
    status_text = "wykonalny" if feasible else "niewykonalny"

    scores = build_constraint_scores(instance, occurrences)
    eval_section = render_evaluation_section(scores, infeasibility, objective)

    constraints_by_id = {c.id: c for c in instance.constraints}
    violation_map = {
        s.id: violation_event_refs(instance, occurrences, constraints_by_id[s.id])
        for s in scores
        if s.cost > 0
    }
    violation_json = json.dumps(violation_map, ensure_ascii=False)

    return f"""<!doctype html>
<html lang="pl">
<head>
<meta charset="utf-8">
<title>{escape(instance.name)} — plan zajęć</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
{fonts_css}
{_PAGE_CSS}
</style>
</head>
<body>
<div class="page">
  <header class="masthead">
    <div class="identity">
      <p class="eyebrow">XHSTT · plan zajęć</p>
      <h1>{escape(instance.name)}</h1>
      <p class="instance-id">{escape(instance.id)}</p>
    </div>
    <dl class="readout">
      <div><dt>zdarzenia</dt><dd>{len(instance.events)}</dd></div>
      <div><dt>status</dt><dd class="{status_class}">{status_text}</dd></div>
      <div><dt>infeasibility</dt>
        <dd class="{"ok" if feasible else "bad"}">{infeasibility}</dd></div>
      <div><dt>objective</dt><dd>{objective}</dd></div>
    </dl>
  </header>

  <nav class="type-tabs" id="type-tabs">{type_tabs}</nav>
  <nav class="chip-row" id="chip-row">{chips}</nav>

  <main class="grid-wrap" id="grid-wrap">{tables}</main>

  {eval_section}
</div>
<script type="application/json" id="violation-map">{violation_json}</script>
<script>
{_PAGE_JS}
</script>
</body>
</html>"""


_PAGE_JS = """
(function () {
  var typeTabs = Array.prototype.slice.call(document.querySelectorAll('.type-tab'));
  var chips = Array.prototype.slice.call(document.querySelectorAll('.chip'));
  var tables = Array.prototype.slice.call(document.querySelectorAll('table.timetable'));

  function showType(type) {
    typeTabs.forEach(function (t) {
      t.classList.toggle('active', t.dataset.type === type);
    });
    chips.forEach(function (c) { c.hidden = c.dataset.type !== type; });
    var firstVisible = chips.filter(function (c) {
      return c.dataset.type === type;
    })[0];
    if (firstVisible) {
      showResource(firstVisible.dataset.type, firstVisible.dataset.resource);
    }
  }

  function showResource(type, resource) {
    chips.forEach(function (c) {
      c.classList.toggle(
        'active',
        c.dataset.type === type && c.dataset.resource === resource
      );
    });
    tables.forEach(function (tbl) {
      tbl.hidden = !(
        tbl.dataset.resourceType === type && tbl.dataset.resource === resource
      );
    });
  }

  typeTabs.forEach(function (t) {
    t.addEventListener('click', function () { showType(t.dataset.type); });
  });
  chips.forEach(function (c) {
    c.addEventListener('click', function () {
      showResource(c.dataset.type, c.dataset.resource);
    });
  });

  if (typeTabs.length) showType(typeTabs[0].dataset.type);

  var evalToggles = Array.prototype.slice.call(
    document.querySelectorAll('.eval-toggle')
  );
  evalToggles.forEach(function (btn) {
    btn.addEventListener('click', function () {
      var kind = btn.dataset.kind;
      var group = document.querySelector('.eval-group[data-kind="' + kind + '"]');
      var okRows = Array.prototype.slice.call(group.querySelectorAll('tr.row--ok'));
      var expanded = btn.dataset.expanded === 'true';
      okRows.forEach(function (row) { row.hidden = expanded; });
      btn.dataset.expanded = expanded ? 'false' : 'true';
      btn.textContent = expanded
        ? 'Pokaż wszystkie (' + btn.dataset.total + ')'
        : 'Pokaż tylko naruszone (' + btn.dataset.violated + ')';
    });
  });

  var violationMap = JSON.parse(
    document.getElementById('violation-map').textContent
  );

  function allCards() {
    return Array.prototype.slice.call(document.querySelectorAll('.cell-card[data-event]'));
  }

  function clearHighlight() {
    allCards().forEach(function (card) {
      card.classList.remove('cell-card--highlighted', 'cell-card--dimmed');
    });
  }

  Array.prototype.slice.call(
    document.querySelectorAll('.eval-table tr[data-constraint]')
  ).forEach(function (row) {
    row.addEventListener('mouseenter', function () {
      var events = new Set(violationMap[row.dataset.constraint] || []);
      if (!events.size) return;
      allCards().forEach(function (card) {
        var hit = events.has(card.dataset.event);
        card.classList.toggle('cell-card--highlighted', hit);
        card.classList.toggle('cell-card--dimmed', !hit);
      });
    });
    row.addEventListener('mouseleave', clearHighlight);
  });
})();
"""
