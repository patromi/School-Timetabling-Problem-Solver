#!/usr/bin/env python
"""Convergence curves from experiments.py JSONL logs.

For every (instance, selector) the best-so-far infeasibility of each seed is
sampled on a common time grid (step function over the run's new_best records,
starting from infeasibility_before at t = 0). The plot shows the median over
seeds with the interquartile band, one panel per instance. The same curves
are written to CSV as a table view.

Usage:
    uv run python scripts/plot_convergence.py --log-dir data/logs/experiment_v2
"""

import argparse
import csv
import json
import statistics
from collections import defaultdict
from collections.abc import Iterable
from pathlib import Path
from typing import Any

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter

# Fixed selector -> colour mapping (colour follows the selector, never its
# rank), categorical slots 1-3 of the reference palette, which stay
# distinguishable for colour-blind readers.
SELECTOR_COLORS = {
    "random": "#2a78d6",
    "epsilon-greedy": "#eb6834",
    "ucb": "#1baf7a",
}
SELECTOR_ORDER = ["random", "epsilon-greedy", "ucb"]
# The reference categorical palette in its validated order; labels that
# are not a plain selector take the free slots after the selectors' own.
CATEGORICAL = [
    "#2a78d6",
    "#eb6834",
    "#1baf7a",
    "#eda100",
    "#e87ba4",
    "#008300",
    "#6250d6",
    "#e34948",
]
TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
GRID = "#e4e3df"

Curve = list[int]  # best-so-far infeasibility on the time grid


def config_label(selector: str, acceptance: str) -> str:
    """Name of one solver configuration on plots and in tables: the
    selector alone under the default acceptance (SA), otherwise selector
    and acceptance, so that variants of the acceptance criterion (e.g. a
    temperature sweep) stay apart."""
    return selector if acceptance == "sa" else f"{selector} · {acceptance}"


def order_labels(labels: Iterable[str]) -> list[str]:
    """Plain selectors first in their fixed order, then the rest sorted."""
    present = set(labels)
    first = [s for s in SELECTOR_ORDER if s in present]
    return first + sorted(present - set(first))


def label_colors(labels: Iterable[str]) -> dict[str, str]:
    """Colour per configuration: a plain selector keeps its fixed colour;
    any other label takes the next free categorical slot in label order,
    so the mapping is stable for a given set of configurations."""
    ordered = order_labels(labels)
    colors = {lab: SELECTOR_COLORS[lab] for lab in ordered if lab in SELECTOR_COLORS}
    free = [c for c in CATEGORICAL if c not in colors.values()]
    for lab in ordered:
        if lab not in colors:
            colors[lab] = free.pop(0) if free else TEXT_SECONDARY
    return colors


def _seed_curve(records: list[dict[str, Any]], grid: list[float]) -> Curve:
    summary = next(r for r in records if r["event"] == "run_summary")
    bests = sorted(
        (float(r["elapsed_s"]), int(r["infeasibility"]))
        for r in records
        if r["event"] == "new_best"
    )
    current = int(summary["infeasibility_before"])
    curve: Curve = []
    i = 0
    for t in grid:
        while i < len(bests) and bests[i][0] <= t:
            current = min(current, bests[i][1])
            i += 1
        curve.append(current)
    return curve


def load_curves(log_dir: Path, grid: list[float]) -> dict[tuple[str, str], list[Curve]]:
    curves: dict[tuple[str, str], list[Curve]] = defaultdict(list)
    for path in sorted(log_dir.glob("*.jsonl")):
        records = [json.loads(line) for line in path.open(encoding="utf-8")]
        summary = next((r for r in records if r["event"] == "run_summary"), None)
        if summary is None:
            continue
        label = config_label(
            str(summary["selector"]), str(summary.get("acceptance", "lahc"))
        )
        key = (str(summary["instance_id"]), label)
        curves[key].append(_seed_curve(records, grid))
    return curves


def _quantiles(values: list[int]) -> tuple[float, float, float]:
    if len(values) == 1:  # a single seed: no band, just the run itself
        return values[0], values[0], values[0]
    q1, med, q3 = statistics.quantiles(values, n=4, method="inclusive")
    return q1, med, q3


def _format_count(value: float, _pos: int) -> str:
    if value >= 1_000_000:
        return f"{value / 1_000_000:g} mln"
    if value >= 1_000:
        return f"{value / 1_000:g} tys."
    return f"{value:g}"


def plot(
    curves: dict[tuple[str, str], list[Curve]], grid: list[float], output: Path
) -> None:
    instances = sorted({inst for inst, _ in curves})
    fig, axes = plt.subplots(
        1, len(instances), figsize=(max(6.0, 4.2 * len(instances)), 3.6), squeeze=False
    )
    colors = label_colors(k[1] for k in curves)
    for ax, inst in zip(axes[0], instances, strict=True):
        selectors = [
            s for s in order_labels(k[1] for k in curves) if (inst, s) in curves
        ]
        lowest = min(min(run) for sel in selectors for run in curves[(inst, sel)])
        for sel in selectors:
            runs = curves[(inst, sel)]
            stats = [_quantiles([run[k] for run in runs]) for k in range(len(grid))]
            color = colors[sel]
            ax.fill_between(
                grid,
                [s[0] for s in stats],
                [s[2] for s in stats],
                color=color,
                alpha=0.15,
                linewidth=0,
            )
            ax.plot(grid, [s[1] for s in stats], color=color, linewidth=1.6, label=sel)
        if lowest == 0:
            # symlog: logarithmic like the cost decay, but linear around 0
            # so that feasible runs (infeasibility 0) stay on the plot
            ax.set_yscale("symlog", linthresh=1)
            ax.set_ylim(bottom=0)
        else:
            ax.set_yscale("log")
            # 1-2-5 ticks: these ranges often span less than one decade
            ax.yaxis.set_major_locator(LogLocator(base=10, subs=(1.0, 2.0, 5.0)))
        ax.yaxis.set_major_formatter(FuncFormatter(_format_count))
        ax.yaxis.set_minor_formatter(NullFormatter())
        ax.set_title(inst, color=TEXT_PRIMARY, fontsize=11, loc="left")
        ax.set_xlabel("czas [s]", color=TEXT_SECONDARY, fontsize=9)
        ax.set_xlim(0, grid[-1])
        ax.grid(True, which="major", color=GRID, linewidth=0.6)
        ax.tick_params(colors=TEXT_SECONDARY, labelsize=8)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)
        for side in ("left", "bottom"):
            ax.spines[side].set_color(GRID)
    axes[0][0].set_ylabel(
        "koszt twardy (infeasibility, skala log.)", color=TEXT_SECONDARY, fontsize=9
    )
    handles, labels = axes[0][0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="upper center",
        ncol=len(labels),
        frameon=False,
        fontsize=9,
        labelcolor=TEXT_PRIMARY,
    )
    fig.text(
        0.5,
        0.005,
        (
            "Pojedynczy przebieg (jeden seed); najlepsze dotąd rozwiązanie."
            if all(len(runs) == 1 for runs in curves.values())
            else "Mediana z seedów (linia) i rozstęp międzykwartylowy (pasmo); "
            "najlepsze dotąd rozwiązanie."
        ),
        ha="center",
        fontsize=8,
        color=TEXT_SECONDARY,
    )
    fig.tight_layout(rect=(0, 0.04, 1, 0.92))
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=200, facecolor="white")
    fig.savefig(output.with_suffix(".pdf"), facecolor="white")
    plt.close(fig)


def write_table(
    curves: dict[tuple[str, str], list[Curve]], grid: list[float], output: Path
) -> None:
    with output.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            ["instance_id", "selector", "time_s", "inf_q1", "inf_median", "inf_q3"]
        )
        for (inst, sel), runs in sorted(curves.items()):
            for k, t in enumerate(grid):
                q1, med, q3 = _quantiles([run[k] for run in runs])
                writer.writerow([inst, sel, t, q1, med, q3])


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot convergence curves.")
    parser.add_argument("--log-dir", type=Path, required=True)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/results/figures/convergence.png"),
        help="PNG path; a PDF and a CSV table are written next to it.",
    )
    parser.add_argument("--time-limit", type=float, default=120.0)
    parser.add_argument("--step", type=float, default=0.5)
    args = parser.parse_args()

    n = round(args.time_limit / args.step)
    grid = [k * args.step for k in range(n + 1)]
    curves = load_curves(args.log_dir, grid)
    plot(curves, grid, args.output)
    write_table(curves, grid, args.output.with_suffix(".csv"))
    print(f"Wykres zapisany do: {args.output} (+ .pdf, .csv)")


if __name__ == "__main__":
    main()
