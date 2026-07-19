from dataclasses import dataclass, field
from html import escape
from pathlib import Path

from xhstt_core.evaluator_ref import Occurrence
from xhstt_core.model import Instance, Time

_ASSETS_DIR = Path(__file__).parent / "assets"
_ROLE_LABELS = {"Teacher": "Naucz.", "Room": "Sala", "Class": "Klasa"}


@dataclass
class DayColumn:
    id: str
    name: str
    periods: list[Time]


@dataclass
class TimetableCell:
    event_ref: str
    event_name: str
    duration: int
    other_resources: list[tuple[str, str]] = field(default_factory=list)


def build_days(instance: Instance) -> list[DayColumn]:
    """One column per "Day" time group (in file order), each carrying its
    periods in file order -- the axes of the school-timetable grid."""
    day_groups = [g for g in instance.time_groups if g.kind == "Day"]
    columns = []
    for group in day_groups:
        periods = [t for t in instance.times if group.id in t.group_refs]
        columns.append(DayColumn(id=group.id, name=group.name, periods=periods))
    return columns


def build_resource_grid(
    instance: Instance, occurrences: list[Occurrence], resource_id: str
) -> dict[tuple[str, int], list[TimetableCell]]:
    """Maps (day_group_id, period_index_within_day) -> lessons for
    `resource_id`, for every occurrence assigning it to any role. A
    duration>1 lesson gets one entry at its starting period only -- the
    renderer spans it across rows using `duration`. Multiple entries at
    the same key means a clash (the resource is double-booked there)."""
    events_by_id = {e.id: e for e in instance.events}
    resource_names = {r.id: r.name for r in instance.resources}
    day_columns = build_days(instance)
    period_position: dict[str, tuple[str, int]] = {}
    for day in day_columns:
        for i, t in enumerate(day.periods):
            period_position[t.id] = (day.id, i)

    grid: dict[tuple[str, int], list[TimetableCell]] = {}
    for o in occurrences:
        if o.time_ref is None or o.time_ref not in period_position:
            continue
        if resource_id not in [r for _, r in o.resource_assignments]:
            continue
        event_def = events_by_id[o.event_ref]
        other_resources = [
            (role, resource_names.get(ref, ref))
            for role, ref in o.resource_assignments
            if ref is not None and ref != resource_id
        ]
        key = period_position[o.time_ref]
        grid.setdefault(key, []).append(
            TimetableCell(
                event_ref=o.event_ref,
                event_name=event_def.name,
                duration=o.duration,
                other_resources=other_resources,
            )
        )
    return grid


def _render_cell_card(cell: TimetableCell, is_clash: bool) -> str:
    tag_class = "cell-card cell-card--clash" if is_clash else "cell-card"
    other = "".join(
        f'<li><span class="role">{escape(_ROLE_LABELS.get(role, role) or "")}</span>'
        f'<span class="who">{escape(name)}</span></li>'
        for role, name in cell.other_resources
    )
    return (
        f'<div class="{tag_class}">'
        f'<p class="lesson">{escape(cell.event_name)}</p>'
        f'<ul class="who-list">{other}</ul>'
        f"</div>"
    )


def _render_resource_table(
    instance: Instance,
    days: list[DayColumn],
    grid: dict[tuple[str, int], list[TimetableCell]],
    resource_type: str,
    resource_id: str,
    resource_name: str,
) -> str:
    max_periods = max((len(d.periods) for d in days), default=0)
    header_cells = "".join(f"<th>{escape(d.name)}</th>" for d in days)
    # Per-day-column count of remaining rows to skip because a longer
    # lesson above is still spanning them via rowspan.
    skip_until = {d.id: 0 for d in days}

    rows = []
    for period_index in range(max_periods):
        cells = [f'<th class="period-no">{period_index + 1}</th>']
        for day in days:
            if period_index >= len(day.periods):
                continue  # this day has fewer periods than the grid's max
            if skip_until[day.id] > period_index:
                continue  # covered by a rowspan from an earlier row
            entries = grid.get((day.id, period_index), [])
            if not entries:
                cells.append('<td class="cell cell--empty"></td>')
                continue
            span = max(e.duration for e in entries)
            skip_until[day.id] = period_index + span
            is_clash = len(entries) > 1
            cards = "".join(_render_cell_card(e, is_clash) for e in entries)
            row_attr = f' rowspan="{span}"' if span > 1 else ""
            clash_class = " cell--clash" if is_clash else ""
            cells.append(f'<td class="cell{clash_class}"{row_attr}>{cards}</td>')
        rows.append(f"<tr>{''.join(cells)}</tr>")

    return (
        f'<table class="timetable" data-resource-type="{escape(resource_type)}" '
        f'data-resource="{escape(resource_id)}" hidden>'
        f'<caption>{escape(resource_name)}</caption>'
        f"<thead><tr><th></th>{header_cells}</tr></thead>"
        f"<tbody>{''.join(rows)}</tbody>"
        f"</table>"
    )


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
            instance, days, build_resource_grid(instance, occurrences, r.id), rt, r.id, r.name
        )
        for rt in type_order
        for r in sorted(resources_by_type[rt], key=lambda r: r.name)
    )

    feasible = infeasibility == 0
    status_class = "ok" if feasible else "bad"
    status_text = "wykonalny" if feasible else "niewykonalny"

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
      <div><dt>infeasibility</dt><dd class="{'ok' if feasible else 'bad'}">{infeasibility}</dd></div>
      <div><dt>objective</dt><dd>{objective}</dd></div>
    </dl>
  </header>

  <nav class="type-tabs" id="type-tabs">{type_tabs}</nav>
  <nav class="chip-row" id="chip-row">{chips}</nav>

  <main class="grid-wrap" id="grid-wrap">{tables}</main>
</div>
<script>
{_PAGE_JS}
</script>
</body>
</html>"""


_PAGE_CSS = """
:root {
  --bg: #eef1ef;
  --surface: #ffffff;
  --ink: #16232b;
  --ink-muted: #5b6b70;
  --line: #c7d1cd;
  --accent: #d9722c;
  --accent-ink: #7a3c14;
  --accent-wash: #f7e2cf;
  --ok: #3f7d6b;
  --ok-wash: #dfeee9;
  --bad: #b23b3b;
  --bad-wash: #f6dede;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #10161a;
    --surface: #1a2329;
    --ink: #e7ece9;
    --ink-muted: #93a3a1;
    --line: #2e3a3d;
    --accent: #e8935a;
    --accent-ink: #2a1608;
    --accent-wash: #3a2a1c;
    --ok: #5fae97;
    --ok-wash: #1c332c;
    --bad: #d9716b;
    --bad-wash: #3a2020;
  }
}
:root[data-theme="dark"] {
  --bg: #10161a; --surface: #1a2329; --ink: #e7ece9; --ink-muted: #93a3a1;
  --line: #2e3a3d; --accent: #e8935a; --accent-ink: #2a1608; --accent-wash: #3a2a1c;
  --ok: #5fae97; --ok-wash: #1c332c; --bad: #d9716b; --bad-wash: #3a2020;
}
:root[data-theme="light"] {
  --bg: #eef1ef; --surface: #ffffff; --ink: #16232b; --ink-muted: #5b6b70;
  --line: #c7d1cd; --accent: #d9722c; --accent-ink: #7a3c14; --accent-wash: #f7e2cf;
  --ok: #3f7d6b; --ok-wash: #dfeee9; --bad: #b23b3b; --bad-wash: #f6dede;
}
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; }
body {
  background: var(--bg);
  color: var(--ink);
  font-family: 'IBM Plex Sans', system-ui, sans-serif;
  -webkit-font-smoothing: antialiased;
}
.page {
  max-width: 1180px;
  margin: 0 auto;
  padding: 2.5rem 1.5rem 4rem;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}
.masthead {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-between;
  align-items: flex-end;
  gap: 1.5rem;
  border-bottom: 2px solid var(--ink);
  padding-bottom: 1.25rem;
}
.eyebrow {
  margin: 0 0 0.35rem;
  font-family: 'IBM Plex Mono', monospace;
  font-size: 0.72rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--accent);
  font-weight: 600;
}
.identity h1 {
  margin: 0;
  font-size: clamp(1.6rem, 3vw, 2.35rem);
  font-weight: 700;
  letter-spacing: -0.01em;
  text-wrap: balance;
}
.instance-id {
  margin: 0.3rem 0 0;
  font-family: 'IBM Plex Mono', monospace;
  font-size: 0.85rem;
  color: var(--ink-muted);
}
.readout {
  display: flex;
  gap: 1.5rem;
  margin: 0;
  font-family: 'IBM Plex Mono', monospace;
}
.readout > div { display: flex; flex-direction: column; gap: 0.2rem; }
.readout dt {
  font-size: 0.68rem;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: var(--ink-muted);
}
.readout dd {
  margin: 0;
  font-size: 1.15rem;
  font-weight: 600;
  font-variant-numeric: tabular-nums;
}
.readout dd.ok { color: var(--ok); }
.readout dd.bad { color: var(--bad); }

.type-tabs, .chip-row {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}
.type-tab, .chip {
  font-family: 'IBM Plex Sans', sans-serif;
  font-size: 0.85rem;
  font-weight: 600;
  border: 1px solid var(--line);
  background: var(--surface);
  color: var(--ink);
  padding: 0.4rem 0.85rem;
  border-radius: 999px;
  cursor: pointer;
  transition: border-color 0.15s ease, color 0.15s ease, background 0.15s ease;
}
.type-tab:hover, .chip:hover { border-color: var(--accent); }
.type-tab:focus-visible, .chip:focus-visible {
  outline: 2px solid var(--accent);
  outline-offset: 2px;
}
.type-tab.active {
  background: var(--ink);
  color: var(--surface);
  border-color: var(--ink);
}
.chip.active {
  background: var(--accent-wash);
  color: var(--accent-ink);
  border-color: var(--accent);
}
.chip { font-weight: 400; font-size: 0.8rem; }

.grid-wrap { overflow-x: auto; }
table.timetable {
  width: 100%;
  border-collapse: collapse;
  background: var(--surface);
  border: 1px solid var(--line);
}
table.timetable caption {
  text-align: left;
  font-weight: 700;
  font-size: 1.05rem;
  padding: 0.85rem 1rem;
  border-bottom: 1px solid var(--line);
}
table.timetable th, table.timetable td {
  border: 1px solid var(--line);
  vertical-align: top;
}
table.timetable thead th {
  font-family: 'IBM Plex Sans', sans-serif;
  font-size: 0.78rem;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--ink-muted);
  padding: 0.6rem 0.75rem;
  text-align: left;
  background: var(--bg);
}
th.period-no {
  font-family: 'IBM Plex Mono', monospace;
  font-size: 0.8rem;
  color: var(--ink-muted);
  text-align: center;
  width: 2.75rem;
  background: var(--bg);
  font-variant-numeric: tabular-nums;
}
td.cell { padding: 0; min-width: 8.5rem; }
td.cell--empty {
  background:
    repeating-linear-gradient(135deg, var(--line) 0 1px, transparent 1px 12px);
  opacity: 0.35;
}
.cell-card {
  height: 100%;
  padding: 0.5rem 0.65rem;
  border-left: 3px solid var(--accent);
  background: var(--accent-wash);
}
.cell-card + .cell-card { border-top: 1px dashed var(--bad); }
.cell-card--clash { border-left-color: var(--bad); background: var(--bad-wash); }
.lesson {
  margin: 0 0 0.3rem;
  font-weight: 600;
  font-size: 0.88rem;
  line-height: 1.25;
}
.who-list {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 0.15rem;
  font-family: 'IBM Plex Mono', monospace;
  font-size: 0.72rem;
  color: var(--ink-muted);
}
.who-list li { display: flex; gap: 0.4rem; }
.who-list .role {
  text-transform: uppercase;
  letter-spacing: 0.04em;
  min-width: 3.6rem;
}
.who-list .who { color: var(--ink); }

@media (prefers-reduced-motion: reduce) {
  .type-tab, .chip { transition: none; }
}
"""

_PAGE_JS = """
(function () {
  var typeTabs = Array.prototype.slice.call(document.querySelectorAll('.type-tab'));
  var chips = Array.prototype.slice.call(document.querySelectorAll('.chip'));
  var tables = Array.prototype.slice.call(document.querySelectorAll('table.timetable'));

  function showType(type) {
    typeTabs.forEach(function (t) { t.classList.toggle('active', t.dataset.type === type); });
    chips.forEach(function (c) { c.hidden = c.dataset.type !== type; });
    var firstVisible = chips.filter(function (c) { return c.dataset.type === type; })[0];
    if (firstVisible) showResource(firstVisible.dataset.type, firstVisible.dataset.resource);
  }

  function showResource(type, resource) {
    chips.forEach(function (c) {
      c.classList.toggle('active', c.dataset.type === type && c.dataset.resource === resource);
    });
    tables.forEach(function (tbl) {
      tbl.hidden = !(tbl.dataset.resourceType === type && tbl.dataset.resource === resource);
    });
  }

  typeTabs.forEach(function (t) {
    t.addEventListener('click', function () { showType(t.dataset.type); });
  });
  chips.forEach(function (c) {
    c.addEventListener('click', function () { showResource(c.dataset.type, c.dataset.resource); });
  });

  if (typeTabs.length) showType(typeTabs[0].dataset.type);
})();
"""
