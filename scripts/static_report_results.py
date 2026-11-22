"""Wykresy raportu statycznego liczone z samych CSV (bez logów JSONL).

fig01  odsetek planów wykonalnych, instancja x selektor (heatmapa)
fig02  koszt miękki planów wykonalnych, per instancja (box ploty)
fig10  SA vs LAHC: wygrane / remisy / przegrane w parach po seedzie
fig12  preset bazowy vs konfiguracja domyślna: odsetek wykonalnych
fig13  wrażliwość UCB na c i alfa (odsetek wykonalnych, średnia ranga)
fig15  UCB z 2 stanami vs 6 stanów (pary po seedzie)
fig16  jakość w funkcji budżetu iteracji
fig17  UCB: wykonalność a liczba iteracji, SA i LAHC (+ logi LAHC)
fig18  cztery selektory: wykonalność a liczba iteracji
"""

import math
import statistics
from collections import defaultdict
from dataclasses import replace
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.patches import Rectangle
from scipy.stats import rankdata
from static_report_data import (
    Run,
    RunLog,
    best_inf_at,
    by_instance_selector,
    feasible_pct,
    gap_pct,
    order_instances,
    pair_by_seed,
    wilcoxon_p,
)
from static_report_style import (
    BLUE_RAMP,
    NEUTRAL,
    SELECTOR_ORDER,
    TEXT_MUTED,
    TEXT_PRIMARY,
    TEXT_SECONDARY,
    WIN_A,
    WIN_B,
    axis_label,
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

ALL = "wszystkie"


# ── fig01: odsetek wykonalnych ────────────────────────────────────────────


def fig01_feasible_heatmap(runs: list[Run], out: Path) -> None:
    groups = by_instance_selector(runs)
    instances = order_instances(r.instance for r in runs)
    selectors = order_selectors(r.selector for r in runs)
    rows = instances + [ALL]
    values: list[list[float]] = []
    counts: list[list[str]] = []
    for inst in rows:
        vals, cnts = [], []
        for sel in selectors:
            g = (
                [r for r in runs if r.selector == sel]
                if inst == ALL
                else groups[(inst, sel)]
            )
            vals.append(feasible_pct(g))
            cnts.append(f"{sum(r.feasible for r in g)}/{len(g)}")
        values.append(vals)
        counts.append(cnts)

    cmap = LinearSegmentedColormap.from_list("blue", ["#f4f8fd", *BLUE_RAMP])
    fig, ax = plt.subplots(figsize=(1.5 * len(selectors) + 2.2, 0.36 * len(rows) + 1.3))
    ax.imshow(values, cmap=cmap, vmin=0, vmax=100, aspect="auto")
    for i, row in enumerate(values):
        for j, v in enumerate(row):
            color = "white" if v >= 60 else TEXT_PRIMARY
            weight = "bold" if rows[i] == ALL else "normal"
            ax.text(
                j,
                i,
                f"{v:.0f}%",
                ha="center",
                va="center",
                fontsize=8,
                color=color,
                weight=weight,
            )
    ax.axhline(len(instances) - 0.5, color="white", linewidth=3)
    ax.set_xticks(range(len(selectors)), selectors, fontsize=8)
    ax.set_yticks(range(len(rows)), rows, fontsize=8)
    ax.xaxis.tick_top()
    ax.tick_params(colors=TEXT_SECONDARY, length=0)
    for spine in ax.spines.values():
        spine.set_visible(False)
    footnote(
        fig,
        "Odsetek przebiegów (seedów) zakończonych planem bez naruszeń twardych. SA, 100k iteracji.",
    )
    save(fig, out, top=1.0, bottom=0.04)

    write_csv(
        out,
        [
            "instance_id",
            *[f"{s}_pct" for s in selectors],
            *[f"{s}_n" for s in selectors],
        ],
        [
            [inst, *[round(v, 1) for v in vals], *cnts]
            for inst, vals, cnts in zip(rows, values, counts, strict=True)
        ],
    )


# ── fig02: koszt miękki planów wykonalnych ───────────────────────────────


def fig02_soft_boxes(runs: list[Run], out: Path) -> None:
    groups = by_instance_selector(runs)
    instances = order_instances(r.instance for r in runs)
    selectors = order_selectors(r.selector for r in runs)
    fig, axes = small_multiples(len(instances), 5, 2.6, 2.3)
    table = []
    for ax, inst in zip(axes, instances, strict=True):
        for x, sel in enumerate(selectors):
            g = groups[(inst, sel)]
            objs = [r.obj for r in g if r.feasible]
            table.append(
                [
                    inst,
                    sel,
                    len(objs),
                    len(g),
                    *(_quartiles(objs) if objs else ["", "", ""]),
                    min(objs, default=""),
                ]
            )
            if not objs:
                continue
            bp = ax.boxplot(
                [objs],
                positions=[x],
                widths=0.55,
                patch_artist=True,
                showfliers=True,
                flierprops={
                    "marker": "o",
                    "markersize": 2.5,
                    "markeredgewidth": 0,
                    "markerfacecolor": TEXT_MUTED,
                },
                medianprops={"color": TEXT_PRIMARY, "linewidth": 1.4},
                whiskerprops={"color": TEXT_SECONDARY, "linewidth": 0.8},
                capprops={"color": TEXT_SECONDARY, "linewidth": 0.8},
            )
            for box in bp["boxes"]:
                box.set_facecolor(selector_color(sel))
                box.set_edgecolor("white")
                box.set_alpha(0.85)
        counts = [sum(r.feasible for r in groups[(inst, sel)]) for sel in selectors]
        totals = [len(groups[(inst, sel)]) for sel in selectors]
        if not any(counts):
            ax.set_yticks([])
            ax.text(
                0.5,
                0.5,
                "brak planów\nwykonalnych",
                transform=ax.transAxes,
                ha="center",
                va="center",
                fontsize=8,
                color=TEXT_MUTED,
            )
        labels = ["" if c == t else f"n={c}" for c, t in zip(counts, totals, strict=True)]
        ax.set_xticks(range(len(selectors)), labels if any(counts) else [""] * len(selectors), fontsize=6)
        ax.set_xlim(-0.6, len(selectors) - 0.4)
        panel_title(ax, inst)
        style_axes(ax)
    axis_label(axes[0], y="koszt miękki")
    handles = [Rectangle((0, 0), 1, 1, color=selector_color(s)) for s in selectors]
    legend_top(fig, handles, selectors)
    footnote(
        fig,
        "Tylko przebiegi zakończone planem wykonalnym; n pod pudełkiem = ich liczba, gdy mniej niż wszystkie seedy. Niżej = lepiej.",
    )
    save(fig, out, top=0.95)
    write_csv(
        out,
        [
            "instance_id",
            "selector",
            "n_feasible",
            "n_runs",
            "q1",
            "median",
            "q3",
            "min",
        ],
        table,
    )


def _quartiles(values: list[int]) -> list[float]:
    if len(values) == 1:
        return [values[0]] * 3
    q = statistics.quantiles(values, n=4, method="inclusive")
    return [q[0], statistics.median(values), q[2]]


# ── porównania par po seedzie (fig10, fig15) ─────────────────────────────


def plot_paired(
    panels: dict[str, dict[str, list[tuple[Run, Run]]]],
    label_a: str,
    label_b: str,
    out: Path,
    note: str,
) -> None:
    """Dla każdej instancji: odsetek seedów, w których A jest lepsze, remis,
    B lepsze (koszt leksykograficzny). '*' przy instancji = p < 0,05 w
    teście Wilcoxona par."""
    panel_names = list(panels)
    instances = order_instances({i for p in panels.values() for i in p})
    fig, axes = plt.subplots(
        1,
        len(panel_names),
        # jeden panel też musi pomieścić stopkę i legendę
        figsize=(max(9.0, 3.3 * len(panel_names) + 0.8), 0.3 * len(instances) + 1.4),
        sharey=True,
        squeeze=False,
    )
    table = []
    for ax, name in zip(axes[0], panel_names, strict=True):
        for y, inst in enumerate(instances):
            pairs = panels[name].get(inst, [])
            if not pairs:
                continue
            a_win = sum(a.lex < b.lex for a, b in pairs)
            b_win = sum(a.lex > b.lex for a, b in pairs)
            tie = len(pairs) - a_win - b_win
            p = wilcoxon_p(pairs)
            table.append([name, inst, len(pairs), a_win, tie, b_win, f"{p:.4g}"])
            left = 0.0
            for value, color in ((a_win, WIN_A), (tie, NEUTRAL), (b_win, WIN_B)):
                share = 100 * value / len(pairs)
                ax.barh(
                    y,
                    share,
                    left=left,
                    color=color,
                    edgecolor="white",
                    linewidth=1.0,
                    height=0.75,
                )
                left += share
            if p < 0.05:
                ax.text(101, y, "*", va="center", fontsize=10, color=TEXT_PRIMARY)
        ax.set_xlim(0, 106)
        ax.set_yticks(range(len(instances)), instances, fontsize=8)
        ax.axvline(50, color="white", linewidth=0.8, linestyle=":")
        panel_title(ax, name)
        axis_label(ax, x="% seedów")
        style_axes(ax, grid_axis="")
    axes[0][0].invert_yaxis()  # oś Y wspólna: jedno odwrócenie dla wszystkich paneli
    handles = [Rectangle((0, 0), 1, 1, color=c) for c in (WIN_A, NEUTRAL, WIN_B)]
    legend_top(fig, handles, [f"{label_a} lepsze", "remis", f"{label_b} lepsze"])
    footnote(
        fig, note + " * = p < 0,05 (Wilcoxon par po seedzie, koszt leksykograficzny)."
    )
    save(fig, out, top=0.9, bottom=0.05)
    write_csv(
        out,
        [
            "panel",
            "instance_id",
            "pairs",
            f"{label_a}_better",
            "tie",
            f"{label_b}_better",
            "wilcoxon_p",
        ],
        table,
    )


def fig10_sa_vs_lahc(sa: list[Run], lahc: list[Run], out: Path) -> None:
    selectors = order_selectors(r.selector for r in sa)
    panels = {
        sel: pair_by_seed(
            [r for r in sa if r.selector == sel], [r for r in lahc if r.selector == sel]
        )
        for sel in selectors
    }
    plot_paired(
        panels, "SA", "LAHC", out, "Ten sam selektor, instancja i seed; 100k iteracji."
    )


def fig15_two_states(default: list[Run], ablation: list[Run], out: Path) -> None:
    six = [r for r in default if r.selector == "ucb"]
    two = [r for r in ablation if r.selector == "ucb-2state"]
    plot_paired(
        {"UCB: 6 stanów vs 2 stany": pair_by_seed(six, two)},
        "6 stanów",
        "2 stany",
        out,
        "2 stany = tylko wykonalny/niewykonalny, bez poziomu stagnacji.",
    )


# ── fig12: preset bazowy vs konfiguracja domyślna ────────────────────────


# w presecie bazowym UCB ma alfa 0,08, więc experiments.py nazywa go inaczej
PRESET_ALIASES = {"ucb-a=0.08": "ucb"}


def fig12_preset(default: list[Run], baseline: list[Run], out: Path) -> None:
    baseline = [
        replace(r, selector=PRESET_ALIASES.get(r.selector, r.selector))
        for r in baseline
    ]
    selectors = order_selectors(r.selector for r in default)
    instances = order_instances(r.instance for r in default)
    g_def, g_base = by_instance_selector(default), by_instance_selector(baseline)
    rows = instances + [ALL]
    fig, axes = plt.subplots(
        1,
        len(selectors),
        figsize=(2.9 * len(selectors) + 0.8, 0.3 * len(rows) + 1.4),
        sharey=True,
        squeeze=False,
    )
    table = []
    for ax, sel in zip(axes[0], selectors, strict=True):
        color = selector_color(sel)
        for y, inst in enumerate(rows):
            if inst == ALL:
                d = [r for r in default if r.selector == sel]
                b = [r for r in baseline if r.selector == sel]
            else:
                d, b = g_def[(inst, sel)], g_base[(inst, sel)]
            pd, pb = feasible_pct(d), feasible_pct(b)
            table.append(
                [sel, inst, round(pb, 1), round(pd, 1), _median_obj(b), _median_obj(d)]
            )
            ax.plot([pb, pd], [y, y], color=NEUTRAL, linewidth=1.6, zorder=2)
            ax.scatter(
                [pb],
                [y],
                s=36,
                color="white",
                edgecolors=TEXT_SECONDARY,
                linewidths=1.2,
                zorder=3,
            )
            ax.scatter(
                [pd],
                [y],
                s=40,
                color=color,
                edgecolors="white",
                linewidths=1.0,
                zorder=4,
            )
        ax.axhline(len(instances) - 0.5, color=TEXT_MUTED, linewidth=0.5)
        ax.set_xlim(-4, 104)
        ax.set_yticks(range(len(rows)), rows, fontsize=8)
        panel_title(ax, sel)
        axis_label(ax, x="% planów wykonalnych")
        style_axes(ax, grid_axis="x")
    axes[0][0].invert_yaxis()
    handles = [
        plt.Line2D(
            [],
            [],
            marker="o",
            linestyle="",
            markerfacecolor="white",
            markeredgecolor=TEXT_SECONDARY,
        ),
        plt.Line2D(
            [],
            [],
            marker="o",
            linestyle="",
            markerfacecolor=TEXT_SECONDARY,
            markeredgecolor="white",
        ),
    ]
    legend_top(
        fig, handles, ["preset bazowy (sprzed issue #82)", "konfiguracja domyślna"]
    )
    footnote(fig, "Kolor pełnego punktu = selektor. Ten sam budżet 100k iteracji.")
    save(fig, out, top=0.9, bottom=0.05)
    write_csv(
        out,
        [
            "selector",
            "instance_id",
            "baseline_feasible_pct",
            "default_feasible_pct",
            "baseline_median_soft_feasible",
            "default_median_soft_feasible",
        ],
        table,
    )


def _median_obj(runs: list[Run]) -> float | str:
    objs = [r.obj for r in runs if r.feasible]
    return statistics.median(objs) if objs else ""


# ── fig13: wrażliwość UCB na parametry ───────────────────────────────────

SENSITIVITY = {
    "c": [
        ("0,005", "ucb-c=0.005"),
        ("0,02", "ucb"),
        ("0,1", "ucb-c=0.1"),
        ("0,5", "ucb-c=0.5"),
    ],
    "alfa": [("0,005", "ucb-a=0.005"), ("0,02", "ucb"), ("0,08", "ucb-a=0.08")],
}


def fig13_sensitivity(default: list[Run], ablation: list[Run], out: Path) -> None:
    runs = [r for r in default if r.selector == "ucb"] + ablation
    groups = by_instance_selector(runs)
    instances = order_instances(r.instance for r in runs)
    fig, axes = plt.subplots(2, 2, figsize=(8.4, 6.0), squeeze=False)
    table = []
    for row, (param, variants) in enumerate(SENSITIVITY.items()):
        labels = [lab for lab, _ in variants]
        xs = list(range(len(variants)))
        # odsetek wykonalnych
        per_inst_feas = {
            inst: [feasible_pct(groups[(inst, sel)]) for _, sel in variants]
            for inst in instances
        }
        # średnia ranga wariantu w obrębie (instancja, seed): 1 = najlepszy
        ranks: dict[str, dict[str, list[float]]] = {
            inst: defaultdict(list) for inst in instances
        }
        for inst in instances:
            seeds = set.intersection(
                *({r.seed for r in groups[(inst, sel)]} for _, sel in variants)
            )
            by_seed = {
                sel: {r.seed: r for r in groups[(inst, sel)]} for _, sel in variants
            }
            for seed in seeds:
                rk = rankdata([by_seed[sel][seed].lex for _, sel in variants])
                for (_, sel), v in zip(variants, rk, strict=True):
                    ranks[inst][sel].append(float(v))
        per_inst_rank = {
            inst: [statistics.mean(ranks[inst][sel]) for _, sel in variants]
            for inst in instances
        }

        for col, (data, ylabel) in enumerate(
            (
                (per_inst_feas, "% planów wykonalnych"),
                (per_inst_rank, "średnia ranga (1 = najlepszy)"),
            )
        ):
            ax = axes[row][col]
            for vals in data.values():
                ax.plot(xs, vals, color=NEUTRAL, linewidth=0.8, alpha=0.7, zorder=2)
            pooled = [statistics.mean(v[i] for v in data.values()) for i in xs]
            ax.plot(
                xs,
                pooled,
                color=selector_color("ucb"),
                linewidth=2.2,
                marker="o",
                markersize=6,
                zorder=3,
            )
            default_x = labels.index("0,02")
            ax.axvline(default_x, color=TEXT_MUTED, linewidth=0.6, linestyle=":")
            ax.set_xticks(xs, labels)
            panel_title(ax, f"{param}: {ylabel}")
            axis_label(ax, x=f"{param} (0,02 = domyślne)")
            style_axes(ax)
            if col == 1:
                ax.invert_yaxis()
            for i, (lab, sel) in enumerate(variants):
                table.append(
                    [
                        param,
                        lab,
                        sel,
                        "feasible_pct" if col == 0 else "mean_rank",
                        round(pooled[i], 3),
                    ]
                )
    handles = [
        plt.Line2D([], [], color=selector_color("ucb"), linewidth=2.2, marker="o"),
        plt.Line2D([], [], color=NEUTRAL, linewidth=0.8),
    ]
    legend_top(fig, handles, ["średnia po instancjach", "pojedyncza instancja"])
    footnote(
        fig,
        "Ranga liczona w obrębie (instancja, seed) na koszcie leksykograficznym, osobno dla c i alfa. 100k iteracji.",
    )
    save(fig, out, top=0.93, bottom=0.04)
    write_csv(
        out, ["param", "value", "selector", "metric", "mean_over_instances"], table
    )


# ── fig16: budżet ─────────────────────────────────────────────────────────


def fig16_budget(
    budgets: dict[int, list[Run]], best: dict[str, int], out: Path
) -> None:
    # tylko (instancja, selektor, seed) obecne we wszystkich budżetach, żeby
    # niedokończona faza nie zmieniała składu porównania
    keys = set.intersection(
        *({(r.instance, r.selector, r.seed) for r in runs} for runs in budgets.values())
    )
    selectors = order_selectors({k[1] for k in keys})
    xs = sorted(budgets)
    fig, axes = plt.subplots(1, 2, figsize=(9.0, 3.6), squeeze=False)
    table = []
    for sel in selectors:
        feas, gaps = [], []
        for it in xs:
            runs = [
                r
                for r in budgets[it]
                if (r.instance, r.selector, r.seed) in keys and r.selector == sel
            ]
            g = [x for x in (gap_pct(r, best) for r in runs) if x is not None]
            feas.append(feasible_pct(runs))
            gaps.append(statistics.median(g) if g else math.nan)
            table.append([sel, it, len(runs), round(feas[-1], 1), round(gaps[-1], 2)])
        color = selector_color(sel)
        for ax, ys in zip(axes[0], (feas, gaps), strict=True):
            ax.plot(
                xs, ys, color=color, linewidth=2, marker="o", markersize=6, label=sel
            )
    for ax, title in zip(
        axes[0],
        ("% planów wykonalnych", "mediana odstępu kosztu miękkiego [%]"),
        strict=True,
    ):
        ax.set_xscale("log")
        ax.set_xticks(xs, [f"{x // 1000}k" for x in xs])
        ax.minorticks_off()
        panel_title(ax, title)
        axis_label(ax, x="budżet [iteracje]")
        style_axes(ax)
    handles, labels = axes[0][0].get_legend_handles_labels()
    legend_top(fig, handles, labels)
    footnote(
        fig,
        f"{len(keys)} trójek (instancja, selektor, seed) obecnych we wszystkich budżetach. "
        "Odstęp = (koszt − najlepszy znany) / najlepszy znany, tylko plany wykonalne.",
    )
    save(fig, out, top=0.88, bottom=0.07)
    write_csv(
        out,
        ["selector", "iterations", "n_runs", "feasible_pct", "median_gap_pct"],
        table,
    )


def wilson_interval(k: int, n: int, z: float = 1.96) -> tuple[float, float]:
    """95% przedział Wilsona dla odsetka k/n, w procentach."""
    p = k / n
    denom = 1 + z * z / n
    center = (p + z * z / (2 * n)) / denom
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    return 100 * max(0.0, center - half), 100 * min(1.0, center + half)


def fig17_ucb_budget(
    budgets: dict[int, list[Run]], lahc_logs: list[RunLog], out: Path
) -> None:
    """UCB: odsetek planów wykonalnych vs liczba iteracji, SA i LAHC, na tych
    samych seedach. SA ma osobne przebiegi na każdy budżet (temperatura zależy
    od budżetu). LAHC od budżetu nie zależy, więc jego wynik po k iteracjach
    odczytujemy z logu przebiegu 100k; przy 300k LAHC nie był uruchamiany."""
    xs = sorted(budgets)
    ucb = {it: [r for r in budgets[it] if r.selector == "ucb"] for it in xs}
    keys = set.intersection(*({(r.instance, r.seed) for r in ucb[it]} for it in xs))
    lahc = {
        (x["instance"], x["seed"]): x for x in lahc_logs if x["selector"] == "ucb"
    }
    lahc_max = max(x["max_iterations"] for x in lahc.values())
    instances = order_instances({k[0] for k in keys})
    count = {i: sum(1 for k in keys if k[0] == i) for i in instances}
    full_n = max(count.values())
    complete = {i for i in instances if count[i] == full_n}
    panels: list[tuple[str, set[str]]] = [("Razem", complete)] + [
        (i, {i}) for i in instances
    ]
    colors = {"SA": WIN_A, "LAHC": WIN_B}
    dodge = {"SA": -0.05, "LAHC": 0.05}
    fig, axes = small_multiples(len(panels), 5, 2.15, 2.15, sharex=True, sharey=True)
    table = []
    handles = {}
    for ax, (name, members) in zip(axes, panels, strict=True):
        panel_keys = [k for k in keys if k[0] in members]
        n = len(panel_keys)
        series: dict[str, dict[int, tuple[float, float, float]]] = {"SA": {}, "LAHC": {}}
        for pos, it in enumerate(xs):
            sa_k = sum(
                r.feasible for r in ucb[it] if (r.instance, r.seed) in set(panel_keys)
            )
            counts = {"SA": sa_k}
            if it <= lahc_max:
                counts["LAHC"] = sum(
                    best_inf_at(lahc[k], it - 1) == 0 for k in panel_keys
                )
            for acc, k in counts.items():
                lo, hi = wilson_interval(k, n)
                series[acc][pos] = (100 * k / n, lo, hi)
                table.append(
                    [name, acc, it, n, k, round(100 * k / n, 1), round(lo, 1), round(hi, 1)]
                )
        partial = len(members) == 1 and not members <= complete
        for acc, pts in series.items():
            px = [q + dodge[acc] for q in pts]
            ys = [v[0] for v in pts.values()]
            handle = ax.errorbar(
                px,
                ys,
                yerr=[
                    [v[0] - v[1] for v in pts.values()],
                    [v[2] - v[0] for v in pts.values()],
                ],
                color=colors[acc],
                linewidth=2,
                marker="o" if acc == "SA" else "s",
                markersize=5,
                markerfacecolor="white" if partial else colors[acc],
                elinewidth=0.9,
                capsize=2,
            )
            handles.setdefault(acc, handle)  # z pierwszego panelu: pełny znacznik
        for q in range(len(xs)):
            sa, la = series["SA"].get(q), series["LAHC"].get(q)
            if la is None or round(sa[0]) == round(la[0]):
                ax.annotate(
                    f"{sa[0]:.0f}", (q, max(sa[2], la[2] if la else 0)),
                    xytext=(0, 3), textcoords="offset points",
                    ha="center", fontsize=7, color=TEXT_PRIMARY,
                )
                continue
            high, low = (sa, la) if sa[0] > la[0] else (la, sa)
            for v, anchor, dy, va in ((high, high[2], 3, "bottom"), (low, low[1], -3, "top")):
                ax.annotate(
                    f"{v[0]:.0f}", (q, anchor),
                    xytext=(0, dy), textcoords="offset points",
                    ha="center", va=va, fontsize=7, color=TEXT_PRIMARY,
                )
        panel_title(ax, f"{name} (n={n})")
        ax.title.set_fontsize(8)
        ax.set_ylim(-20, 122)
        ax.set_yticks([0, 50, 100])
        ax.set_xlim(-0.35, len(xs) - 0.65)
        ax.set_xticks(range(len(xs)), [f"{x // 1000}k" for x in xs])
        ax.tick_params(labelbottom=True)
        style_axes(ax)
    for ax in axes[::5]:
        axis_label(ax, y="% planów wykonalnych")
    legend_top(fig, list(handles.values()), list(handles))
    footnote(
        fig,
        f"UCB, te same seedy. Razem: {len(complete)} instancji z kompletem seedów. "
        "Wąsy: 95% przedział Wilsona. Puste znaczniki: niepełna faza 300k. "
        "LAHC: 10k odczytane z logu przebiegu 100k, 300k nie uruchamiano.",
    )
    save(fig, out, top=0.95, bottom=0.04)
    write_csv(
        out,
        ["panel", "acceptance", "iterations", "n_runs", "n_feasible",
         "feasible_pct", "ci95_low", "ci95_high"],
        table,
    )


def fig18_selectors_budget(budgets: dict[int, list[Run]], out: Path) -> None:
    """Cztery selektory (SA): odsetek planów wykonalnych vs liczba iteracji.
    Tylko pary (instancja, seed) obecne dla każdego selektora i budżetu;
    "Razem" tylko z instancji z kompletem seedów."""
    xs = sorted(budgets)
    selectors = order_selectors({r.selector for it in xs for r in budgets[it]})
    index = {
        (it, r.selector, r.instance, r.seed): r for it in xs for r in budgets[it]
    }
    candidates = {(r.instance, r.seed) for it in xs for r in budgets[it]}
    keys = {
        (i, seed)
        for i, seed in candidates
        if all((it, sel, i, seed) in index for it in xs for sel in selectors)
    }
    instances = order_instances({k[0] for k in keys})
    missing = order_instances({c[0] for c in candidates} - set(instances))
    count = {i: sum(1 for k in keys if k[0] == i) for i in instances}
    full_n = max(count.values())
    complete = {i for i in instances if count[i] == full_n}
    panels: list[tuple[str, set[str]]] = [("Razem", complete)] + [
        (i, {i}) for i in instances
    ]
    markers = dict(zip(SELECTOR_ORDER, ("o", "s", "^", "D"), strict=True))
    fig, axes = small_multiples(len(panels), 5, 2.15, 2.05, sharex=True, sharey=True)
    table = []
    handles = {}
    for ax, (name, members) in zip(axes, panels, strict=True):
        panel_keys = [k for k in keys if k[0] in members]
        n = len(panel_keys)
        partial = len(members) == 1 and not members <= complete
        for j, sel in enumerate(selectors):
            ys, low, high = [], [], []
            for it in xs:
                k = sum(index[(it, sel, i, seed)].feasible for i, seed in panel_keys)
                lo, hi = wilson_interval(k, n)
                y = 100 * k / n
                ys.append(y)
                low.append(y - lo)
                high.append(hi - y)
                table.append(
                    [name, sel, it, n, k, round(y, 1), round(lo, 1), round(hi, 1)]
                )
            color = selector_color(sel)
            px = [q + (j - 1.5) * 0.045 for q in range(len(xs))]
            # wąsy tylko w panelu zbiorczym: w panelach instancji cztery serie
            # z przedziałami przy n=50 przestają być czytelne
            handle = ax.errorbar(
                px,
                ys,
                yerr=[low, high] if len(members) > 1 else None,
                color=color,
                linewidth=1.8,
                marker=markers.get(sel, "o"),
                markersize=4.5,
                markerfacecolor="white" if partial else color,
                elinewidth=0.9,
                capsize=2,
            )
            handles.setdefault(sel, handle)
        panel_title(ax, f"{name} (n={n})")
        ax.title.set_fontsize(8)
        ax.set_ylim(-6, 106)
        ax.set_yticks([0, 50, 100])
        ax.set_xlim(-0.35, len(xs) - 0.65)
        ax.set_xticks(range(len(xs)), [f"{x // 1000}k" for x in xs])
        ax.tick_params(labelbottom=True)
        style_axes(ax)
    for ax in axes[::5]:
        axis_label(ax, y="% planów wykonalnych")
    legend_top(fig, list(handles.values()), list(handles))
    footnote(
        fig,
        f"SA, te same seedy dla każdego selektora i budżetu. Razem: {len(complete)} "
        "instancji z kompletem seedów, wąsy: 95% przedział Wilsona. "
        "Puste znaczniki: niepełna faza 300k."
        + (f" Pominięto (brak wspólnych seedów): {', '.join(missing)}." if missing else ""),
    )
    save(fig, out, top=0.95, bottom=0.04)
    write_csv(
        out,
        ["panel", "selector", "iterations", "n_runs", "n_feasible",
         "feasible_pct", "ci95_low", "ci95_high"],
        table,
    )
