"""Wykresy raportu statycznego liczone z logów JSONL (wyciągi `RunLog`).

  fig03  zbieżność: najlepszy koszt twardy w funkcji iteracji
  fig06  udział wywołań heurystyk w trakcie przebiegu
  figS1  rozkład długości okresów bez poprawy najlepszego wyniku
  figS2  czas spędzony w każdym z 6 stanów selektora UCB
  figS3  odsetek iteracji w wysokiej stagnacji w trakcie przebiegu
  figS4  moment ostatniej poprawy jako ułamek budżetu
  figS5  wybory UCB w zależności od stanu (wykonalność x stagnacja)
  figS6  skuteczność perturbacji
  figS7  anatomia jednego przebiegu

Licznik stagnacji jest odtwarzany dokładnie tak jak w src/lahc.py: rośnie
o 1 na iterację i zeruje się przy każdym nowym najlepszym wyniku i każdej
perturbacji. Stan selektora: (1 = plan wykonalny / 0 = niewykonalny,
stagnacja 0/1/2 = niska/średnia/wysoka wg progów z run_config).
"""

import statistics
from collections import defaultdict
from collections.abc import Iterable
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import plot_heuristic_usage
from matplotlib.colors import LinearSegmentedColormap, ListedColormap
from matplotlib.patches import Rectangle
from static_report_data import RunLog, best_inf_at, order_instances, stagnation_resets
from static_report_style import (
    BLUE_RAMP,
    NEUTRAL,
    ORANGE_RAMP,
    TEXT_MUTED,
    TEXT_PRIMARY,
    TEXT_SECONDARY,
    axis_label,
    data_path,
    figure_paths,
    footnote,
    legend_top,
    order_selectors,
    panel_title,
    save,
    selector_color,
    small_multiples,
    style_axes,
    write_csv,
)

STAGNATION_NAMES = ["niska", "średnia", "wysoka"]
STATE_KEYS = ["0,0", "0,1", "0,2", "1,0", "1,1", "1,2"]
STATE_LABELS = [
    f"{'wykonalny' if k[0] == '1' else 'niewykonalny'} · {STAGNATION_NAMES[int(k[2])]}"
    for k in STATE_KEYS
]
# niewykonalne = rampa pomarańczowa, wykonalne = niebieska; jaśniej = mniejsza stagnacja
STATE_COLORS = [
    ORANGE_RAMP[0],
    ORANGE_RAMP[1],
    ORANGE_RAMP[3],
    BLUE_RAMP[1],
    BLUE_RAMP[3],
    BLUE_RAMP[5],
]
STAGNATION_COLORS = ["#e9e8e4", BLUE_RAMP[2], BLUE_RAMP[5]]


def _group(logs: Iterable[RunLog]) -> dict[tuple[str, str], list[RunLog]]:
    groups: dict[tuple[str, str], list[RunLog]] = defaultdict(list)
    for log in logs:
        groups[(log["instance"], log["selector"])].append(log)
    return groups


def _selector_legend(fig: plt.Figure, selectors: list[str], band: bool = False) -> None:
    handles = [
        plt.Line2D([], [], color=selector_color(s), linewidth=2) for s in selectors
    ]
    legend_top(fig, handles, selectors)


# ── fig03: zbieżność ──────────────────────────────────────────────────────


def fig03_convergence(logs: list[RunLog], out: Path, points: int = 101) -> None:
    groups = _group(logs)
    instances = order_instances(log["instance"] for log in logs)
    selectors = order_selectors(log["selector"] for log in logs)
    max_it = max(log["max_iterations"] for log in logs)
    grid = [round(max_it * i / (points - 1)) for i in range(points)]
    fig, axes = small_multiples(len(instances), 5, 2.7, 2.2)
    table = []
    for ax, inst in zip(axes, instances, strict=True):
        for sel in selectors:
            runs = groups.get((inst, sel), [])
            if not runs:
                continue
            curves = np.array([[best_inf_at(log, s) for s in grid] for log in runs])
            q1, med, q3 = np.percentile(curves, [25, 50, 75], axis=0)
            color = selector_color(sel)
            ax.fill_between(grid, q1, q3, color=color, alpha=0.15, linewidth=0)
            ax.plot(grid, med, color=color, linewidth=1.6)
            table += [
                [inst, sel, s, m, a, b]
                for s, m, a, b in zip(grid, med, q1, q3, strict=True)
            ]
        ax.set_yscale("symlog", linthresh=1)
        ax.set_ylim(bottom=0)
        ax.set_xlim(0, max_it)
        ax.xaxis.set_major_formatter(
            plt.FuncFormatter(lambda v, _p: f"{v / 1000:.0f}k")
        )
        panel_title(ax, inst)
        style_axes(ax)
    axis_label(axes[0], y="najlepszy koszt twardy")
    _selector_legend(fig, selectors)
    footnote(
        fig,
        "Linia = mediana po seedach, pasmo = rozstęp międzykwartylowy. Oś X: iteracja. Oś Y: symlog (0 = plan wykonalny).",
    )
    save(fig, out, top=0.95)
    write_csv(
        out, ["instance_id", "selector", "iteration", "median", "q1", "q3"], table
    )


# ── fig06: udział heurystyk ───────────────────────────────────────────────


def fig06_heuristic_usage(logs: list[RunLog], out: Path, bins: int = 10) -> None:
    """Rysuje istniejącym plot_heuristic_usage.plot, dzieląc instancje na
    dwie strony, żeby wykres zmieścił się na stronie A4."""
    usage: plot_heuristic_usage.Usage = defaultdict(
        lambda: [defaultdict(int) for _ in range(bins)]
    )
    for log in logs:
        key = (log["instance"], log["selector"])
        run_length = log["iterations_done"]
        for start, end, heuristics in log["windows"]:
            b = min(bins - 1, int(bins * (start + end) / 2 / run_length))
            for h, (calls, _better) in heuristics.items():
                usage[key][b][plot_heuristic_usage.GROUP_OF.get(h, "inne")] += calls
    instances = order_instances(k[0] for k in usage)
    half = (len(instances) + 1) // 2
    for part, chunk in enumerate((instances[:half], instances[half:]), start=1):
        sub = {k: v for k, v in usage.items() if k[0] in chunk}
        path = out.with_name(f"{out.name}_{part}")
        png, pdf = figure_paths(path)
        # plot_heuristic_usage.plot zapisuje PDF obok PNG -- przenosimy go do pdf/
        plot_heuristic_usage.plot(sub, bins, png)
        png.with_suffix(".pdf").replace(pdf)
        data = data_path(path)
        data.parent.mkdir(parents=True, exist_ok=True)
        plot_heuristic_usage.write_table(sub, bins, data)


# ── figS1: okresy bez poprawy ─────────────────────────────────────────────


def _gaps(logs: Iterable[RunLog]) -> list[int]:
    gaps: list[int] = []
    for log in logs:
        steps = [s for s, _, _ in log["bests"]]
        gaps += [b - a for a, b in zip(steps, steps[1:], strict=False)]
    return gaps


def _ccdf(
    ax: plt.Axes, values: list[int], color: str, label: str, dashed: bool = False
) -> list[tuple[int, float]]:
    xs = np.sort(np.array(values))
    ys = 1.0 - np.arange(1, len(xs) + 1) / len(xs)
    ax.step(
        xs,
        ys,
        where="post",
        color=color,
        linewidth=1.8,
        label=label,
        linestyle="--" if dashed else "-",
    )
    return [(int(t), float(np.mean(xs > t))) for t in (100, 500, 1000, 2000, 3000)]


def figS1_gaps(sa_logs: list[RunLog], lahc_logs: list[RunLog], out: Path) -> None:
    selectors = order_selectors(log["selector"] for log in sa_logs)
    thresholds = sa_logs[0]["thresholds"] + [sa_logs[0]["idle_limit"]]
    fig, axes = plt.subplots(1, 2, figsize=(9.4, 3.6), sharey=True, squeeze=False)
    table = []
    ax = axes[0][0]
    for sel in selectors:
        pts = _ccdf(
            ax,
            _gaps(x for x in sa_logs if x["selector"] == sel),
            selector_color(sel),
            sel,
        )
        table += [["SA", sel, t, round(p, 5)] for t, p in pts]
    panel_title(ax, "selektory (SA)")
    ax = axes[0][1]
    for sel in selectors:
        color = selector_color(sel)
        _ccdf(
            ax, _gaps(x for x in sa_logs if x["selector"] == sel), color, f"{sel} · SA"
        )
        pts = _ccdf(
            ax,
            _gaps(x for x in lahc_logs if x["selector"] == sel),
            color,
            f"{sel} · LAHC",
            dashed=True,
        )
        table += [["LAHC", sel, t, round(p, 5)] for t, p in pts]
    panel_title(ax, "SA (linia ciągła) vs LAHC (przerywana)")
    for ax in axes[0]:
        ax.set_xscale("log")
        ax.set_yscale("log")
        for t in thresholds:
            ax.axvline(t, color=TEXT_MUTED, linewidth=0.7, linestyle=":")
        axis_label(ax, x="długość okresu bez nowego najlepszego wyniku [iteracje]")
        style_axes(ax, grid_axis="both")
    axis_label(axes[0][0], y="P(okres dłuższy niż x)")
    _selector_legend(fig, selectors)
    footnote(
        fig,
        "Odstępy między kolejnymi nowymi najlepszymi wynikami, wszystkie instancje i seedy (100k iteracji). "
        f"Kropkowane linie: progi stagnacji „średnia” ({thresholds[0]}), „wysoka” ({thresholds[1]}) i perturbacja ({thresholds[2]}).",
    )
    save(fig, out, top=0.9, bottom=0.05)
    write_csv(out, ["acceptance", "selector", "longer_than", "share"], table)


# ── figS2: czas w stanach selektora ───────────────────────────────────────


def _state_shares(logs: Iterable[RunLog]) -> list[float]:
    totals = dict.fromkeys(STATE_KEYS, 0)
    for log in logs:
        for key, s in (log["final_states"] or {}).items():
            if key in totals:
                totals[key] += s["N"]
    n = sum(totals.values()) or 1
    return [100 * totals[k] / n for k in STATE_KEYS]


def figS2_state_occupancy(
    logs: list[RunLog],
    out: Path,
    selectors: tuple[str, ...] = ("ucb", "ucb-preprocessing"),
) -> None:
    groups = _group(logs)
    instances = order_instances(log["instance"] for log in logs)
    present = [s for s in selectors if any(k[1] == s for k in groups)]
    fig, axes = plt.subplots(
        1,
        len(present),
        figsize=(4.6 * len(present), 0.3 * len(instances) + 1.6),
        sharey=True,
        squeeze=False,
    )
    table = []
    for ax, sel in zip(axes[0], present, strict=True):
        for y, inst in enumerate(instances):
            shares = _state_shares(groups.get((inst, sel), []))
            table.append([sel, inst, *[round(s, 2) for s in shares]])
            left = 0.0
            for share, color in zip(shares, STATE_COLORS, strict=True):
                ax.barh(
                    y,
                    share,
                    left=left,
                    color=color,
                    edgecolor="white",
                    linewidth=0.8,
                    height=0.75,
                )
                left += share
        ax.set_xlim(0, 100)
        ax.set_yticks(range(len(instances)), instances, fontsize=8)
        panel_title(ax, sel)
        axis_label(ax, x="% decyzji selektora")
        style_axes(ax, grid_axis="")
    axes[0][0].invert_yaxis()
    handles = [Rectangle((0, 0), 1, 1, color=c) for c in STATE_COLORS]
    legend_top(fig, handles, STATE_LABELS, ncol=3)
    footnote(
        fig,
        "Udział decyzji selektora podjętych w każdym stanie (licznik N stanu na końcu przebiegu, suma po seedach).",
    )
    save(fig, out, top=0.84, bottom=0.05)
    write_csv(out, ["selector", "instance_id", *STATE_LABELS], table)


# ── figS3: wysoka stagnacja w trakcie przebiegu ───────────────────────────


def _high_stagnation_share(log: RunLog, bins: int) -> np.ndarray:
    """Ułamek iteracji każdego z `bins` odcinków przebiegu, w których licznik
    stagnacji jest >= progu „wysoka”. Dokładnie, odcinkami między resetami:
    po resecie w kroku r licznik osiąga próg w kroku r + T."""
    length = log["iterations_done"]
    threshold = log["thresholds"][1]
    edges = np.linspace(0, length, bins + 1)
    covered = np.zeros(bins)
    resets = [0, *stagnation_resets(log), length]
    for start, end in zip(resets, resets[1:], strict=False):
        lo, hi = start + threshold, end
        if hi <= lo:
            continue
        covered += np.clip(
            np.minimum(edges[1:], hi) - np.maximum(edges[:-1], lo), 0, None
        )
    return covered / np.diff(edges)


def figS3_high_stagnation(logs: list[RunLog], out: Path, bins: int = 50) -> None:
    groups = _group(logs)
    instances = order_instances(log["instance"] for log in logs)
    selectors = order_selectors(log["selector"] for log in logs)
    xs = [100 * (b + 0.5) / bins for b in range(bins)]
    fig, axes = small_multiples(len(instances), 5, 2.7, 2.2, sharey=True)
    table = []
    for ax, inst in zip(axes, instances, strict=True):
        for sel in selectors:
            runs = groups.get((inst, sel), [])
            if not runs:
                continue
            share = 100 * np.mean(
                [_high_stagnation_share(log, bins) for log in runs], axis=0
            )
            ax.plot(xs, share, color=selector_color(sel), linewidth=1.5)
            table += [
                [inst, sel, round(x, 1), round(v, 2)]
                for x, v in zip(xs, share, strict=True)
            ]
        ax.set_xlim(0, 100)
        ax.set_ylim(0, 100)
        panel_title(ax, inst)
        style_axes(ax)
    axis_label(axes[0], y="% iteracji w stagnacji „wysokiej”")
    _selector_legend(fig, selectors)
    footnote(
        fig,
        "Oś X: postęp przebiegu [%], średnia po seedach. Licznik zeruje się przy nowym najlepszym wyniku i perturbacji "
        "(także nieudanej), więc przy trwałym zastoju „wysoka” zajmuje ok. 1/3 cyklu: iteracje 2000–3000 z ~3000.",
    )
    save(fig, out, top=0.95)
    write_csv(
        out, ["instance_id", "selector", "progress_pct", "high_stagnation_pct"], table
    )


# ── figS4: ostatnia poprawa ───────────────────────────────────────────────


def _last_improvement(log: RunLog) -> float:
    return log["bests"][-1][0] / log["max_iterations"] if log["bests"] else 0.0


def figS4_last_improvement(budget_logs: dict[int, list[RunLog]], out: Path) -> None:
    budgets = sorted(budget_logs)
    selectors = order_selectors(
        log["selector"] for logs in budget_logs.values() for log in logs
    )
    fig, axes = plt.subplots(
        1,
        len(budgets),
        figsize=(3.4 * len(budgets) + 0.6, 3.4),
        sharey=True,
        squeeze=False,
    )
    table = []
    for ax, it in zip(axes[0], budgets, strict=True):
        for sel in selectors:
            vals = np.sort(
                [
                    _last_improvement(log)
                    for log in budget_logs[it]
                    if log["selector"] == sel
                ]
            )
            if not len(vals):
                continue
            ys = np.arange(1, len(vals) + 1) / len(vals)
            ax.step(
                100 * vals,
                100 * ys,
                where="post",
                color=selector_color(sel),
                linewidth=1.8,
            )
            table.append(
                [
                    it,
                    sel,
                    len(vals),
                    round(100 * float(np.median(vals)), 1),
                    round(100 * float(np.mean(vals > 0.9)), 1),
                ]
            )
        ax.set_xlim(0, 100)
        ax.set_ylim(0, 100)
        panel_title(ax, f"budżet {it // 1000}k iteracji")
        axis_label(ax, x="ostatnia poprawa [% budżetu]")
        style_axes(ax, grid_axis="both")
    axis_label(axes[0][0], y="% przebiegów")
    _selector_legend(fig, selectors)
    footnote(
        fig,
        "Dystrybuanta: jaki odsetek przebiegów przestał się poprawiać przed danym punktem budżetu. "
        "Krzywa przy prawej krawędzi = przebiegi wciąż się poprawiają.",
    )
    save(fig, out, top=0.86, bottom=0.06)
    write_csv(
        out,
        [
            "iterations",
            "selector",
            "n_runs",
            "median_last_improvement_pct",
            "improving_after_90pct",
        ],
        table,
    )


# ── figS5: wybory UCB wg stanu ────────────────────────────────────────────


def figS5_choices_by_state(
    logs: list[RunLog],
    out: Path,
    selectors: tuple[str, ...] = ("ucb", "ucb-preprocessing"),
) -> None:
    present = [s for s in selectors if any(log["selector"] == s for log in logs)]
    heuristics = sorted(
        {
            h
            for log in logs
            for s in (log["final_states"] or {}).values()
            for h in s["n"]
        }
    )
    cmap = LinearSegmentedColormap.from_list("blue", ["#f4f8fd", *BLUE_RAMP])
    fig, axes = plt.subplots(
        1,
        len(present),
        figsize=(0.62 * len(heuristics) * len(present) + 2.6, 4.0),
        squeeze=False,
    )
    table = []
    for ax, sel in zip(axes[0], present, strict=True):
        counts = {k: dict.fromkeys(heuristics, 0) for k in STATE_KEYS}
        for log in logs:
            if log["selector"] != sel:
                continue
            for key, s in (log["final_states"] or {}).items():
                if key in counts:
                    for h, n in s["n"].items():
                        counts[key][h] += n
        matrix = []
        labels = []
        for key, label in zip(STATE_KEYS, STATE_LABELS, strict=True):
            total = sum(counts[key].values())
            matrix.append(
                [100 * counts[key][h] / total if total else np.nan for h in heuristics]
            )
            labels.append(f"{label} (N={total:,})".replace(",", " "))
            table += [
                [sel, label, h, counts[key][h], round(v, 2)]
                for h, v in zip(heuristics, matrix[-1], strict=True)
            ]
        ax.imshow(matrix, cmap=cmap, vmin=0, vmax=np.nanmax(matrix), aspect="auto")
        for i, row in enumerate(matrix):
            for j, v in enumerate(row):
                if not np.isnan(v):
                    color = "white" if v >= 0.6 * np.nanmax(matrix) else TEXT_PRIMARY
                    ax.text(
                        j,
                        i,
                        f"{v:.0f}",
                        ha="center",
                        va="center",
                        fontsize=7,
                        color=color,
                    )
        ax.set_xticks(
            range(len(heuristics)), heuristics, rotation=40, ha="right", fontsize=7
        )
        ax.set_yticks(range(len(labels)), labels, fontsize=7)
        ax.axhline(2.5, color="white", linewidth=3)
        ax.tick_params(colors=TEXT_SECONDARY, length=0)
        for spine in ax.spines.values():
            spine.set_visible(False)
        panel_title(ax, sel)
    footnote(
        fig,
        "Komórka = % wyborów heurystyki w danym stanie (wiersz sumuje się do 100). Wszystkie instancje i seedy, 100k iteracji.",
    )
    save(fig, out, top=0.95, bottom=0.04)
    write_csv(out, ["selector", "state", "heuristic", "pulls", "share_pct"], table)


# ── figS6: perturbacje ────────────────────────────────────────────────────


def figS6_perturbations(logs: list[RunLog], out: Path) -> None:
    instances = order_instances(log["instance"] for log in logs)
    kinds = [
        ("small_perturbation", "mała", BLUE_RAMP[3]),
        ("large_perturbation", "duża", ORANGE_RAMP[2]),
    ]
    stats: dict[tuple[str, str], dict[str, float]] = {}
    runs_per_inst: dict[str, int] = defaultdict(int)
    for log in logs:
        runs_per_inst[log["instance"]] += 1
    for inst in instances:
        for kind, _, _ in kinds:
            outs = [
                o
                for log in logs
                if log["instance"] == inst
                for o in log["outcomes"]
                if o[0] == kind
            ]
            rec = [o[2] for o in outs if o[1] and o[2] is not None]
            stats[(inst, kind)] = {
                "per_run": len(outs) / runs_per_inst[inst],
                "new_best": 100 * sum(o[3] for o in outs) / len(outs)
                if outs
                else np.nan,
                "recovered": 100 * sum(o[1] for o in outs) / len(outs)
                if outs
                else np.nan,
                "median_steps": statistics.median(rec) if rec else np.nan,
            }
    metrics = [
        ("per_run", "perturbacji na przebieg"),
        ("new_best", "% kończy się nowym najlepszym"),
        ("recovered", "% powrót do kosztu sprzed"),
    ]
    fig, axes = plt.subplots(
        1, 3, figsize=(10.5, 0.32 * len(instances) + 1.5), sharey=True, squeeze=False
    )
    ys = np.arange(len(instances))
    for ax, (metric, title) in zip(axes[0], metrics, strict=True):
        for k, (kind, _, color) in enumerate(kinds):
            vals = [stats[(inst, kind)][metric] for inst in instances]
            ax.barh(
                ys + (k - 0.5) * 0.38,
                vals,
                height=0.36,
                color=color,
                edgecolor="white",
                linewidth=0.6,
            )
        ax.set_yticks(ys, instances, fontsize=8)
        if metric != "per_run":
            ax.set_xlim(0, 100)
        panel_title(ax, title)
        style_axes(ax, grid_axis="x")
    axes[0][0].invert_yaxis()
    legend_top(
        fig,
        [Rectangle((0, 0), 1, 1, color=c) for _, _, c in kinds],
        [f"perturbacja {n}" for _, n, _ in kinds],
    )
    footnote(
        fig,
        "Wszystkie selektory razem (perturbacje odpala solver po 3000 iteracjach bez poprawy, nie selektor). "
        "„Powrót” = koszt bieżący wrócił do najlepszego sprzed perturbacji.",
    )
    save(fig, out, top=0.88, bottom=0.05)
    write_csv(
        out,
        [
            "instance_id",
            "perturbation",
            "per_run",
            "new_best_pct",
            "recovered_pct",
            "median_steps_to_recover",
        ],
        [
            [inst, kind, *[round(v, 2) for v in stats[(inst, kind)].values()]]
            for inst in instances
            for kind, _, _ in kinds
        ],
    )


# ── figS7: anatomia przebiegu ─────────────────────────────────────────────


def _stagnation_buckets(log: RunLog) -> np.ndarray:
    """Poziom stagnacji (0/1/2) w każdej iteracji przebiegu."""
    length = log["iterations_done"]
    low, high = log["thresholds"]
    buckets = np.zeros(length, dtype=np.int8)
    resets = [0, *stagnation_resets(log), length]
    for start, end in zip(resets, resets[1:], strict=False):
        buckets[min(start + low, end) : end] = 1
        buckets[min(start + high, end) : end] = 2
    return buckets


def figS7_anatomy(log: RunLog, out: Path) -> None:
    length = log["iterations_done"]
    bests = [[0, log["inf_before"], log["obj_before"]], *log["bests"]]
    snaps = log["snapshots"]
    weight = 1_000_000  # INFEASIBILITY_WEIGHT z src/
    fig, axes = plt.subplots(
        4,
        1,
        figsize=(9.0, 6.8),
        sharex=True,
        gridspec_kw={"height_ratios": [3, 3, 2, 0.5]},
        squeeze=False,
    )
    ax_h, ax_s, ax_t, ax_g = (row[0] for row in axes)
    steps = [b[0] for b in bests] + [length]
    ax_h.step(
        steps,
        [b[1] for b in bests] + [bests[-1][1]],
        where="post",
        color=TEXT_PRIMARY,
        linewidth=1.6,
        label="najlepszy",
    )
    ax_h.plot(
        [s[0] for s in snaps],
        [s[1] // weight for s in snaps],
        color=BLUE_RAMP[3],
        linewidth=0.9,
        marker="o",
        markersize=3,
        label="bieżący",
    )
    ax_s.step(
        steps,
        [b[2] for b in bests] + [bests[-1][2]],
        where="post",
        color=TEXT_PRIMARY,
        linewidth=1.6,
    )
    ax_s.plot(
        [s[0] for s in snaps],
        [s[1] % weight for s in snaps],
        color=BLUE_RAMP[3],
        linewidth=0.9,
        marker="o",
        markersize=3,
    )
    temps = [(s[0], s[3]) for s in snaps if s[3] is not None]
    if temps:
        ax_t.plot(
            [t[0] for t in temps],
            [t[1] for t in temps],
            color=ORANGE_RAMP[2],
            linewidth=1.2,
            marker="o",
            markersize=2.5,
        )
        ax_t.set_yscale("log")
        ax_t.yaxis.set_major_formatter(
            plt.FuncFormatter(lambda v, _p: f"{v:,.0f}".replace(",", " "))
        )
        ax_t.yaxis.set_minor_formatter(plt.NullFormatter())
    for step, kind in log["perturbations"]:
        for ax in (ax_h, ax_s, ax_t):
            ax.axvline(
                step,
                color=NEUTRAL,
                linewidth=0.6,
                linestyle="--" if kind == "large_perturbation" else ":",
                zorder=0,
            )
    ax_g.imshow(
        _stagnation_buckets(log)[None, :],
        aspect="auto",
        cmap=ListedColormap(STAGNATION_COLORS),
        vmin=0,
        vmax=2,
        extent=(0, length, 0, 1),
        interpolation="nearest",
    )
    ax_g.set_yticks([])
    ax_h.set_yscale("symlog", linthresh=1)
    ax_h.set_ylim(bottom=0)
    for ax, title in (
        (ax_h, "koszt twardy"),
        (ax_s, "koszt miękki"),
        (ax_t, "temperatura SA"),
        (ax_g, "stagnacja"),
    ):
        panel_title(ax, title)
        style_axes(ax)
    ax_g.set_xlim(0, length)
    ax_g.xaxis.set_major_formatter(plt.FuncFormatter(lambda v, _p: f"{v / 1000:.0f}k"))
    axis_label(ax_g, x="iteracja")
    handles = [
        plt.Line2D([], [], color=TEXT_PRIMARY, linewidth=1.6),
        plt.Line2D([], [], color=BLUE_RAMP[3], linewidth=0.9, marker="o", markersize=3),
        plt.Line2D([], [], color=NEUTRAL, linestyle=":"),
        plt.Line2D([], [], color=NEUTRAL, linestyle="--"),
        *[Rectangle((0, 0), 1, 1, color=c) for c in STAGNATION_COLORS],
    ]
    legend_top(
        fig,
        handles,
        [
            "najlepszy",
            "bieżący (migawki)",
            "perturbacja mała",
            "perturbacja duża",
            *[f"stagnacja {n}" for n in STAGNATION_NAMES],
        ],
    )
    footnote(
        fig,
        f"{log['instance']}, selektor {log['selector']}, seed {log['seed']}. "
        "Koszt bieżący i temperatura: migawki co 5000 iteracji i przy każdej perturbacji.",
    )
    save(fig, out, top=0.9, bottom=0.03)
    write_csv(
        out,
        [
            "step",
            "current_cost",
            "best_cost",
            "temperature",
            "state",
            "stagnant_iterations",
        ],
        snaps,
    )
