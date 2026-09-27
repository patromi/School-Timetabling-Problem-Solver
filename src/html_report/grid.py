from dataclasses import dataclass, field
from html import escape

from src.evaluator_ref import Occurrence
from src.model import Instance, Time

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
        f'<div class="{tag_class}" data-event="{escape(cell.event_ref)}">'
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
        f"<caption>{escape(resource_name)}</caption>"
        f"<thead><tr><th></th>{header_cells}</tr></thead>"
        f"<tbody>{''.join(rows)}</tbody>"
        f"</table>"
    )


