#!/usr/bin/env python
"""Wykresy odsetka planów wykonalnych z drugiej rundy strojenia
(scripts/tune_selector.py, etapy test2_100 i test2_300).

  v2_wykonalnosc_selektory  cztery selektory, 100k vs 300k iteracji:
                            razem i każda z 13 instancji
  v2_runda1_vs_runda2       razem, runda 1 (test100/test300) obok rundy 2

Instancje bez żadnego planu wykonalnego (żaden selektor, żaden budżet
rundy 2) są pominięte; "Razem" liczy się z pozostałych. Wąsy to 95%
przedział Wilsona. Rundy liczono na różnych seedach (runda 1:
200-249, runda 2: 300-349), więc porównanie rund nie jest sparowane.

Użycie:
    uv run python scripts/plot_tuning_v2.py
    uv run python scripts/plot_tuning_v2.py --v2 data/results/tuning_v2 \\
        --v1 data/results/tuning --out data/results/tuning_v2/wykresy
"""

import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from static_report_results import wilson_interval  # noqa: E402
from static_report_style import (  # noqa: E402
    NEUTRAL,
    SELECTOR_ORDER,
    TEXT_SECONDARY,
    axis_label,
    footnote,
    legend_top,
    panel_title,
    plt,
    save,
    selector_color,
    small_multiples,
    style_axes,
    write_csv,
)
from tune_selector import ALL_INSTANCES  # noqa: E402

BUDGETS = {100_000: "test2_100", 300_000: "test2_300"}
ROUND1 = {100_000: "test100", 300_000: "test300"}
NCOLS = 4
MARKERS = dict(zip(SELECTOR_ORDER, ("o", "s", "^", "D"), strict=True))

# (selektor, instancja) -> [liczba wykonalnych, liczba przebiegów]
Counts = dict[tuple[str, str], list[int]]


def load(path: Path) -> Counts:
    counts: Counts = defaultdict(lambda: [0, 0])
    with path.open(encoding="utf-8") as f:
        for row in csv.DictReader(f):
            c = counts[(row["selector"], row["instance"])]
            c[0] += int(row["inf"]) == 0
            c[1] += 1
    return counts


def total(counts: Counts, selector: str, instances: list[str]) -> tuple[int, int]:
    k = sum(counts[(selector, i)][0] for i in instances)
    n = sum(counts[(selector, i)][1] for i in instances)
    return k, n


def solved_instances(data: dict[int, Counts]) -> tuple[list[str], list[str]]:
    """Instancje z co najmniej jednym planem wykonalnym i te bez żadnego."""
    present = [
        i
        for i in ALL_INSTANCES
        if any((s, i) in d for d in data.values() for s in SELECTOR_ORDER)
    ]
    solved = [
        i
        for i in present
        if any(
            d[(s, i)][0] for d in data.values() for s in SELECTOR_ORDER if (s, i) in d
        )
    ]
    return solved, [i for i in present if i not in solved]


def fig_selectors(
    data: dict[int, Counts], instances: list[str], skipped: list[str], out: Path
) -> None:
    """Small multiples: razem + instancje, oś x = budżet."""
    xs = sorted(data)
    panels = [("Razem", instances)] + [(i, [i]) for i in instances]
    fig, axes = small_multiples(
        len(panels), NCOLS, 2.15, 2.05, sharex=True, sharey=True
    )
    handles = {}
    table = []
    for ax, (name, members) in zip(axes, panels, strict=True):
        for j, sel in enumerate(SELECTOR_ORDER):
            ys, low, high = [], [], []
            for it in xs:
                k, n = total(data[it], sel, members)
                lo, hi = wilson_interval(k, n)
                y = 100 * k / n
                ys.append(y)
                low.append(y - lo)
                high.append(hi - y)
                table.append(
                    [name, sel, it, n, k, round(y, 1), round(lo, 1), round(hi, 1)]
                )
            px = [q + (j - 1.5) * 0.045 for q in range(len(xs))]
            # wąsy tylko w panelu zbiorczym: przy n = 30-50 na instancję cztery
            # serie z przedziałami przestają być czytelne
            handle = ax.errorbar(
                px,
                ys,
                yerr=[low, high] if len(members) > 1 else None,
                color=selector_color(sel),
                linewidth=1.8,
                marker=MARKERS[sel],
                markersize=4.5,
                elinewidth=0.9,
                capsize=2,
            )
            handles.setdefault(sel, handle)
        panel_title(ax, name)
        ax.title.set_fontsize(8)
        ax.set_ylim(-6, 106)
        ax.set_yticks([0, 50, 100])
        ax.set_xlim(-0.35, len(xs) - 0.65)
        ax.set_xticks(range(len(xs)), [f"{x // 1000}k" for x in xs])
        ax.tick_params(labelbottom=True)
        style_axes(ax)
    for ax in axes[::NCOLS]:
        axis_label(ax, y="% planów wykonalnych")
    legend_top(fig, list(handles.values()), list(handles))
    n100 = total(data[xs[0]], "random", instances[:1])[1]
    n300 = total(data[xs[-1]], "random", instances[:1])[1]
    footnote(
        fig,
        f"Runda 2 strojenia, konfiguracja SA i UCB ze strojenia. {n100} seedów na "
        f"instancję przy 100k, {n300} przy 300k.\nRazem: {len(instances)} instancji, "
        "wąsy: 95% przedział Wilsona."
        + (
            f" Pominięto (0% wykonalnych u wszystkich): {', '.join(skipped)}."
            if skipped
            else ""
        ),
    )
    save(fig, out, top=0.95, bottom=0.06)
    write_csv(
        out,
        [
            "panel",
            "selector",
            "iterations",
            "n",
            "feasible",
            "pct",
            "ci_low",
            "ci_high",
        ],
        table,
    )


def fig_rounds(
    r1: dict[int, Counts],
    r2: dict[int, Counts],
    instances: list[str],
    skipped: list[str],
    out: Path,
) -> None:
    """Słupki: razem, runda 1 (jasny, kreskowany) obok rundy 2 (pełny)."""
    xs = sorted(r2)
    fig, axes = plt.subplots(1, len(xs), figsize=(8.6, 3.6), sharey=True)
    width = 0.38
    table = []
    for ax, it in zip(axes, xs, strict=True):
        for j, sel in enumerate(SELECTOR_ORDER):
            color = selector_color(sel)
            for r, (data, x) in enumerate(
                ((r1, j - width / 2 - 0.01), (r2, j + width / 2 + 0.01))
            ):
                k, n = total(data[it], sel, instances)
                lo, hi = wilson_interval(k, n)
                y = 100 * k / n
                table.append(
                    [
                        it,
                        f"runda {r + 1}",
                        sel,
                        n,
                        k,
                        round(y, 1),
                        round(lo, 1),
                        round(hi, 1),
                    ]
                )
                ax.bar(
                    x,
                    y,
                    width,
                    color=color if r else "white",
                    edgecolor=color,
                    hatch=None if r else "////",
                    linewidth=1.0,
                    zorder=2,
                )
                ax.errorbar(
                    x,
                    y,
                    yerr=[[y - lo], [hi - y]],
                    color=TEXT_SECONDARY,
                    elinewidth=0.8,
                    capsize=2,
                    zorder=3,
                )
                ax.text(
                    x,
                    hi + 1.5,
                    f"{y:.0f}",
                    ha="center",
                    va="bottom",
                    fontsize=7,
                    color=TEXT_SECONDARY,
                )
        panel_title(ax, f"{it // 1000}k iteracji")
        ax.set_xticks(range(len(SELECTOR_ORDER)), SELECTOR_ORDER, fontsize=7)
        ax.set_ylim(0, 100)
        style_axes(ax)
    axis_label(axes[0], y=f"% planów wykonalnych ({len(instances)} instancji)")
    legend_top(
        fig,
        [
            plt.Rectangle(
                (0, 0), 1, 1, facecolor="white", edgecolor=NEUTRAL, hatch="////"
            ),
            plt.Rectangle((0, 0), 1, 1, facecolor=NEUTRAL, edgecolor=NEUTRAL),
        ],
        ["runda 1 (perturbacje, T0 0,01)", "runda 2 (bez perturbacji, T0 0,01 / 0,02)"],
    )
    footnote(
        fig,
        "Rundy na różnych seedach (1: 200-249, 2: 300-349; przy 300k pierwsze 30), "
        "wąsy: 95% przedział Wilsona. Kolor słupka to selektor."
        + (
            f"\nPominięto (0% wykonalnych u wszystkich w rundzie 2): {', '.join(skipped)}."
            if skipped
            else ""
        ),
    )
    save(fig, out, top=0.9, bottom=0.08 if skipped else 0.05)
    write_csv(
        out,
        [
            "iterations",
            "round",
            "selector",
            "n",
            "feasible",
            "pct",
            "ci_low",
            "ci_high",
        ],
        table,
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Wykresy wykonalności z rundy 2 strojenia."
    )
    parser.add_argument("--v2", type=Path, default=Path("data/results/tuning_v2"))
    parser.add_argument("--v1", type=Path, default=Path("data/results/tuning"))
    parser.add_argument("--out", type=Path, default=None, help="domyślnie <v2>/wykresy")
    args = parser.parse_args()
    out = args.out or args.v2 / "wykresy"
    r2 = {it: load(args.v2 / f"{stage}.csv") for it, stage in BUDGETS.items()}
    instances, skipped = solved_instances(r2)
    fig_selectors(r2, instances, skipped, out / "v2_wykonalnosc_selektory")
    r1_paths = {it: args.v1 / f"{stage}.csv" for it, stage in ROUND1.items()}
    if all(p.exists() for p in r1_paths.values()):
        r1 = {it: load(p) for it, p in r1_paths.items()}
        fig_rounds(r1, r2, instances, skipped, out / "v2_runda1_vs_runda2")
    print(f"Zapisano wykresy w {out}/png i {out}/pdf, dane w {out.parent / 'dane'}")


if __name__ == "__main__":
    main()
