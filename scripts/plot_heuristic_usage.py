#!/usr/bin/env python
"""Share of heuristic calls over the course of a run, per selector.

Reads the selector_window records of experiments.py JSONL logs, places each
window by its position in the run (fraction of the run's iterations), and
sums calls over all seeds into `--bins` equal slices of the run. One panel
per (instance, selector): stacked columns, each column = 100% of the calls
in that slice of the run. The same shares are written to CSV as a table.

Failed moves (the heuristic raised ValueError) are not logged by the solver,
so a heuristic that always fails (e.g. resource_reassign on instances
without resource choices) does not appear.

Usage:
    uv run python scripts/plot_heuristic_usage.py --log-dir data/logs/experiment_v2
"""

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from plot_convergence import config_label, order_labels

# Heuristic -> group shown on the plot. The two perturbation operators, and
# the two random moves, are each folded into one group so the pool fits the
# 8-colour categorical palette.
GROUP_OF = {
    "ruin_and_recreate": "ruin_and_recreate",
    "move_best": "move_best",
    "repair_hard_violation": "repair_hard_violation",
    "repair_clash": "repair_clash",
    "move_random": "ruchy losowe",
    "swap": "ruchy losowe",
    "kempe_chain": "kempe_chain",
    "split_resize": "split_resize",
    "small_perturbation": "perturbacje",
    "large_perturbation": "perturbacje",
    "resource_reassign": "resource_reassign",
}
# Fixed group -> colour mapping (colour follows the heuristic, never its
# rank), reference categorical palette slots 1-8 in their validated order.
GROUP_COLORS = {
    "ruin_and_recreate": "#2a78d6",
    "move_best": "#eb6834",
    "repair_hard_violation": "#1baf7a",
    "repair_clash": "#eda100",
    "ruchy losowe": "#e87ba4",
    # kempe_chain dominates some instances; violet keeps it apart from the
    # aqua of repair_hard_violation, green goes to the rarely used split_resize
    "kempe_chain": "#6250d6",
    "split_resize": "#008300",
    "perturbacje": "#e34948",
}
OTHER_COLOR = "#b5b3ad"
TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
SURFACE = "#ffffff"

# (instance, selector) -> bin -> group -> calls
Usage = dict[tuple[str, str], list[dict[str, int]]]


def load_usage(log_dir: Path, bins: int) -> Usage:
    usage: Usage = defaultdict(lambda: [defaultdict(int) for _ in range(bins)])
    for path in sorted(log_dir.glob("*.jsonl")):
        records: list[dict[str, Any]] = [
            json.loads(line) for line in path.open(encoding="utf-8")
        ]
        summary = next((r for r in records if r["event"] == "run_summary"), None)
        windows = [r for r in records if r["event"] == "selector_window"]
        if summary is None or not windows:
            continue
        run_length = max(int(w["window_end"]) for w in windows) + 1
        label = config_label(
            str(summary["selector"]), str(summary.get("acceptance", "lahc"))
        )
        key = (str(summary["instance_id"]), label)
        for w in windows:
            middle = (int(w["window_start"]) + int(w["window_end"])) / 2
            b = min(bins - 1, int(bins * middle / run_length))
            for h, stats in w["heuristics"].items():
                usage[key][b][GROUP_OF.get(h, "inne")] += int(stats["calls"])
    return usage


def _groups_present(usage: Usage) -> list[str]:
    seen = {g for slices in usage.values() for s in slices for g in s}
    ordered = [g for g in GROUP_COLORS if g in seen]
    return ordered + sorted(seen - set(ordered))


def plot(usage: Usage, bins: int, output: Path) -> None:
    instances = sorted({inst for inst, _ in usage})
    selectors = order_labels(k[1] for k in usage)
    groups = _groups_present(usage)

    fig, axes = plt.subplots(
        len(instances),
        len(selectors),
        figsize=(3.6 * len(selectors), 2.5 * len(instances) + 1.0),
        sharex=True,
        sharey=True,
        squeeze=False,
    )
    xs = [100 * (b + 0.5) / bins for b in range(bins)]
    width = 100 / bins * 0.9
    for i, inst in enumerate(instances):
        for j, sel in enumerate(selectors):
            ax = axes[i][j]
            slices = usage.get((inst, sel))
            if slices is None:
                ax.set_axis_off()
                continue
            totals = [sum(s.values()) or 1 for s in slices]
            bottom = [0.0] * bins
            for g in groups:
                share = [
                    100 * s.get(g, 0) / t for s, t in zip(slices, totals, strict=True)
                ]
                ax.bar(
                    xs,
                    share,
                    width=width,
                    bottom=bottom,
                    color=GROUP_COLORS.get(g, OTHER_COLOR),
                    edgecolor=SURFACE,
                    linewidth=0.8,
                    label=g,
                )
                bottom = [b + s for b, s in zip(bottom, share, strict=True)]
            ax.set_ylim(0, 100)
            ax.set_xlim(0, 100)
            ax.tick_params(colors=TEXT_SECONDARY, labelsize=7, length=2)
            for side in ("top", "right", "left", "bottom"):
                ax.spines[side].set_visible(False)
            if i == 0:
                ax.set_title(sel, color=TEXT_PRIMARY, fontsize=10)
            if j == 0:
                ax.set_ylabel(
                    f"{inst}\nudział wywołań [%]", color=TEXT_SECONDARY, fontsize=8
                )
            if i == len(instances) - 1:
                ax.set_xlabel("postęp przebiegu [%]", color=TEXT_SECONDARY, fontsize=8)

    handles = [
        Rectangle((0, 0), 1, 1, color=GROUP_COLORS.get(g, OTHER_COLOR)) for g in groups
    ]
    fig.legend(
        handles,
        groups,
        loc="upper center",
        ncol=min(4, len(groups)),
        frameon=False,
        fontsize=8,
        labelcolor=TEXT_PRIMARY,
    )
    fig.text(
        0.5,
        0.005,
        "Suma wywołań ze wszystkich seedów w każdej 1/"
        f"{bins} przebiegu; nieudane ruchy nie są logowane.",
        ha="center",
        fontsize=7,
        color=TEXT_SECONDARY,
    )
    fig.tight_layout(rect=(0, 0.02, 1, 0.94))
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=200, facecolor=SURFACE)
    fig.savefig(output.with_suffix(".pdf"), facecolor=SURFACE)
    plt.close(fig)


def write_table(usage: Usage, bins: int, output: Path) -> None:
    groups = _groups_present(usage)
    with output.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["instance_id", "selector", "progress_pct", *groups])
        for (inst, sel), slices in sorted(usage.items()):
            for b, s in enumerate(slices):
                total = sum(s.values()) or 1
                writer.writerow(
                    [inst, sel, round(100 * (b + 1) / bins)]
                    + [round(100 * s.get(g, 0) / total, 1) for g in groups]
                )


def main() -> None:
    parser = argparse.ArgumentParser(description="Plot heuristic call shares.")
    parser.add_argument("--log-dir", type=Path, required=True)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/results/figures/heuristic_usage.png"),
        help="PNG path; a PDF and a CSV table are written next to it.",
    )
    parser.add_argument("--bins", type=int, default=10)
    args = parser.parse_args()

    usage = load_usage(args.log_dir, args.bins)
    plot(usage, args.bins, args.output)
    write_table(usage, args.bins, args.output.with_suffix(".csv"))
    print(f"Wykres zapisany do: {args.output} (+ .pdf, .csv)")


if __name__ == "__main__":
    main()
