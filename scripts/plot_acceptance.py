#!/usr/bin/env python
"""Acceptance rate over time, per selector, from experiments.py JSONL logs.

Each selector_window record covers LOG_WINDOW_SIZE iterations and stores,
per heuristic, how many candidates were proposed and how many the acceptance
criterion took. The window's acceptance rate (all heuristics together) is
placed on the wall-clock span of the window: the run start is recovered from
any new_best record (wall-clock `t` minus `elapsed_s`), and a window spans
from the previous window's `t` to its own. Each seed is sampled on a common
time grid; the plot shows the median over seeds with the interquartile band.

A rate near 0 means the search has stopped moving: almost no candidate is
accepted, so the current solution sits in a local optimum.

Failed moves (a heuristic raised ValueError) are not logged, so they count
neither as proposed nor as accepted.

Usage:
    uv run python scripts/plot_acceptance.py --log-dir data/logs/experiment_v2
"""

import argparse
import csv
import json
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Any

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from plot_convergence import (
    GRID,
    TEXT_PRIMARY,
    TEXT_SECONDARY,
    config_label,
    label_colors,
    order_labels,
)

Curve = list[float | None]  # acceptance rate [%] on the time grid


def _seed_curve(
    records: list[dict[str, Any]], grid: list[float], field: str
) -> Curve | None:
    anchor = next((r for r in records if r["event"] == "new_best"), None)
    windows = [r for r in records if r["event"] == "selector_window"]
    if anchor is None or not windows:
        return None
    start = float(anchor["t"]) - float(anchor["elapsed_s"])
    spans: list[tuple[float, float, float]] = []  # (from_s, to_s, rate %)
    previous = 0.0
    for w in windows:
        end = float(w["t"]) - start
        calls = sum(int(h["calls"]) for h in w["heuristics"].values())
        if any(field not in h for h in w["heuristics"].values()):
            return None  # log written before this counter existed
        accepted = sum(int(h[field]) for h in w["heuristics"].values())
        if calls:
            spans.append((previous, end, 100 * accepted / calls))
        previous = end
    curve: Curve = []
    for t in grid:
        rate = next((r for a, b, r in spans if a <= t <= b), None)
        curve.append(rate)
    return curve


def load_curves(
    log_dir: Path, grid: list[float], field: str = "accepted"
) -> dict[tuple[str, str], list[Curve]]:
    """Per (instance, selector) the seeds' curves of `field` as a percentage
    of proposed candidates: "accepted", or one of "accepted_better",
    "accepted_equal", "accepted_worse" (logs written after those counters
    were added only)."""
    curves: dict[tuple[str, str], list[Curve]] = defaultdict(list)
    for path in sorted(log_dir.glob("*.jsonl")):
        records = [json.loads(line) for line in path.open(encoding="utf-8")]
        summary = next((r for r in records if r["event"] == "run_summary"), None)
        if summary is None:
            continue
        curve = _seed_curve(records, grid, field)
        if curve is not None:
            label = config_label(
                str(summary["selector"]), str(summary.get("acceptance", "lahc"))
            )
            key = (str(summary["instance_id"]), label)
            curves[key].append(curve)
    return curves


def _quantiles(values: list[float]) -> tuple[float, float, float] | None:
    if not values:
        return None
    if len(values) == 1:
        return values[0], values[0], values[0]
    q1, med, q3 = statistics.quantiles(values, n=4, method="inclusive")
    return q1, med, q3


def _stats(
    runs: list[Curve], n: int, min_seeds: int
) -> list[tuple[float, float, float] | None]:
    """Per grid point quartiles over the seeds that have a window there;
    None where fewer than `min_seeds` seeds do (e.g. past the last window)."""
    out: list[tuple[float, float, float] | None] = []
    for k in range(n):
        values = [run[k] for run in runs if run[k] is not None]
        present = [v for v in values if v is not None]
        out.append(_quantiles(present) if len(present) >= min_seeds else None)
    return out


def _style_axis(ax: Axes, labelsize: int) -> None:
    """Recessive grid and axes shared by every panel."""
    ax.grid(True, color=GRID, linewidth=0.6)
    ax.tick_params(colors=TEXT_SECONDARY, labelsize=labelsize)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)


def plot(
    curves: dict[tuple[str, str], list[Curve]], grid: list[float], output: Path
) -> None:
    instances = sorted({inst for inst, _ in curves})
    fig, axes = plt.subplots(
        1,
        len(instances),
        figsize=(max(6.0, 4.2 * len(instances)), 3.6),
        sharey=True,
        squeeze=False,
    )
    colors = label_colors(k[1] for k in curves)
    for ax, inst in zip(axes[0], instances, strict=True):
        for sel in [
            s for s in order_labels(k[1] for k in curves) if (inst, s) in curves
        ]:
            runs = curves[(inst, sel)]
            stats = _stats(runs, len(grid), min_seeds=max(1, len(runs) // 2))
            color = colors[sel]
            xs = [t for t, s in zip(grid, stats, strict=True) if s is not None]
            kept = [s for s in stats if s is not None]
            ax.fill_between(
                xs,
                [s[0] for s in kept],
                [s[2] for s in kept],
                color=color,
                alpha=0.15,
                linewidth=0,
            )
            ax.plot(xs, [s[1] for s in kept], color=color, linewidth=1.6, label=sel)
        ax.set_ylim(0, None)
        ax.set_xlim(0, grid[-1])
        ax.set_title(inst, color=TEXT_PRIMARY, fontsize=11, loc="left")
        ax.set_xlabel("czas [s]", color=TEXT_SECONDARY, fontsize=9)
        _style_axis(ax, labelsize=8)
    axes[0][0].set_ylabel(
        "zaakceptowani kandydaci [%]", color=TEXT_SECONDARY, fontsize=9
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
        "Odsetek kandydatów przyjętych przez kryterium akceptacji, w oknach "
        "logu selektora; mediana z seedów i rozstęp międzykwartylowy.",
        ha="center",
        fontsize=8,
        color=TEXT_SECONDARY,
    )
    fig.tight_layout(rect=(0, 0.04, 1, 0.92))
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=200, facecolor="white")
    fig.savefig(output.with_suffix(".pdf"), facecolor="white")
    plt.close(fig)


# Accepted candidates by the sign of their cost change: an ordered polarity,
# so the two poles of the reference diverging pair with a neutral gray between.
BREAKDOWN = [
    ("accepted_better", "lepszy", "#2a78d6"),
    ("accepted_equal", "bez zmiany kosztu", "#8a8984"),
    ("accepted_worse", "gorszy", "#e34948"),
]


def plot_breakdown(log_dir: Path, grid: list[float], output: Path) -> bool:
    """Grid instances x selectors: share of proposed candidates accepted as
    better / equal-cost / worse. Returns False (and writes nothing) when the
    logs predate the split counters."""
    per_field = {f: load_curves(log_dir, grid, f) for f, _, _ in BREAKDOWN}
    keys = set(per_field["accepted_better"])
    if not keys:
        return False
    instances = sorted({inst for inst, _ in keys})
    selectors = order_labels(k[1] for k in keys)
    fig, axes = plt.subplots(
        len(instances),
        len(selectors),
        figsize=(3.6 * len(selectors), 2.4 * len(instances) + 1.0),
        sharex=True,
        sharey=True,
        squeeze=False,
    )
    for i, inst in enumerate(instances):
        for j, sel in enumerate(selectors):
            ax = axes[i][j]
            for field, label, color in BREAKDOWN:
                runs = per_field[field].get((inst, sel), [])
                if not runs:
                    continue
                stats = _stats(runs, len(grid), min_seeds=max(1, len(runs) // 2))
                xs = [t for t, s in zip(grid, stats, strict=True) if s is not None]
                ys = [s[1] for s in stats if s is not None]
                ax.plot(xs, ys, color=color, linewidth=1.4, label=label)
            ax.set_xlim(0, grid[-1])
            _style_axis(ax, labelsize=7)
            if i == 0:
                ax.set_title(sel, color=TEXT_PRIMARY, fontsize=10)
            if j == 0:
                ax.set_ylabel(f"{inst}\n% kandydatów", color=TEXT_SECONDARY, fontsize=8)
            if i == len(instances) - 1:
                ax.set_xlabel("czas [s]", color=TEXT_SECONDARY, fontsize=8)
    # shared y axis: set the range once, after every panel is drawn, so no
    # panel's line is clipped by another panel's autoscaled limit
    top = max(line.get_ydata().max() for ax in axes.flat for line in ax.lines)
    axes[0][0].set_ylim(0, top * 1.05)
    handles, labels = axes[0][0].get_legend_handles_labels()
    fig.legend(
        handles,
        [f"zaakceptowany, {label}" for label in labels],
        loc="upper center",
        ncol=len(labels),
        frameon=False,
        fontsize=8,
        labelcolor=TEXT_PRIMARY,
    )
    fig.text(
        0.5,
        0.005,
        "Odsetek proponowanych kandydatów przyjętych z poprawą, bez zmiany "
        "kosztu i z pogorszeniem; mediana z seedów.",
        ha="center",
        fontsize=7,
        color=TEXT_SECONDARY,
    )
    fig.tight_layout(rect=(0, 0.02, 1, 0.94))
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=200, facecolor="white")
    fig.savefig(output.with_suffix(".pdf"), facecolor="white")
    plt.close(fig)
    return True


def write_table(
    curves: dict[tuple[str, str], list[Curve]], grid: list[float], output: Path
) -> None:
    with output.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(
            ["instance_id", "selector", "time_s", "acc_q1", "acc_median", "acc_q3"]
        )
        for (inst, sel), runs in sorted(curves.items()):
            stats = _stats(runs, len(grid), min_seeds=max(1, len(runs) // 2))
            for t, s in zip(grid, stats, strict=True):
                if s is not None:
                    writer.writerow([inst, sel, t, *(round(v, 2) for v in s)])


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot acceptance rate over time.")
    parser.add_argument("--log-dir", type=Path, required=True)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/results/figures/acceptance.png"),
        help="PNG path; a PDF and a CSV table are written next to it.",
    )
    parser.add_argument("--time-limit", type=float, default=120.0)
    parser.add_argument("--step", type=float, default=1.0)
    args = parser.parse_args()

    n = round(args.time_limit / args.step)
    grid = [k * args.step for k in range(n + 1)]
    curves = load_curves(args.log_dir, grid)
    plot(curves, grid, args.output)
    write_table(curves, grid, args.output.with_suffix(".csv"))
    print(f"Wykres zapisany do: {args.output} (+ .pdf, .csv)")
    breakdown = args.output.with_name(args.output.stem + "_breakdown.png")
    if plot_breakdown(args.log_dir, grid, breakdown):
        print(f"Wykres zapisany do: {breakdown} (+ .pdf)")
    else:
        print("Logi bez podziału akceptacji (lepszy/równy/gorszy) -- pominięto.")


if __name__ == "__main__":
    main()
