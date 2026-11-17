#!/usr/bin/env python3
"""Share of heuristic calls over the course of a run: Baseline vs Preprocessing.

Reads selector_window records from Baseline and Preprocessing JSONL logs,
places each window by its position in the run (fraction of run iterations), and
sums calls over all seeds into `--bins` equal slices of the run.

Outputs comparative grid plots styled after
docs/wyniki/selektory_sa_udzial_heurystyk.png:
- Left column: Baseline (bez preprocessingu)
- Right column: Preprocessing (Adaptive)
- Rows: Instances

Generates:
1. Scale-grouped charts: Small, Medium, Large instances (ideal for thesis).
2. Complete all-instance overview chart.
3. Individual per-instance comparison charts.
4. Summary CSV table with exact percentages.

Usage:
    uv run python scripts/plot_heuristic_usage_comparison.py
    uv run python scripts/plot_heuristic_usage_comparison.py --bins 10
    uv run python scripts/plot_heuristic_usage_comparison.py --instances BR-SA-00
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import sys
import tempfile
from collections import defaultdict
from pathlib import Path
from typing import Any

# Use a writable temp directory for matplotlib cache on Windows
os.environ.setdefault("MPLCONFIGDIR", tempfile.gettempdir())

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from matplotlib.patches import Rectangle

# Heuristic grouping consistent with existing project visualizations
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

# Fixed palette from reference visualizations
GROUP_COLORS = {
    "ruin_and_recreate": "#2a78d6",  # blue
    "move_best": "#eb6834",  # orange
    "repair_hard_violation": "#1baf7a",  # sea green
    "repair_clash": "#eda100",  # amber
    "ruchy losowe": "#e87ba4",  # pink (move_random + swap)
    "kempe_chain": "#6250d6",  # violet
    "resource_reassign": "#8c564b",  # brown
    "split_resize": "#008300",  # dark green
    "perturbacje": "#e34948",  # red
}
OTHER_COLOR = "#b5b3ad"
TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
SURFACE = "#ffffff"

# Instance scale groupings for readable sub-reports
SCALE_GROUPS = {
    "small": [
        "BR-SA-00",
        "BR-SM-00",
        "BR-SN-00",
        "FI-WP-06",
        "GR-P3-10",
        "ZA-LW-09",
    ],
    "medium": [
        "AU-BG-98",
        "AU-SA-96",
        "FI-MP-06",
        "FI-PB-98",
        "GR-H1-97",
        "GR-PA-08",
        "ZA-WD-09",
    ],
    "large": [
        "DK-FG-12",
        "IT-I4-96",
        "KS-PR-11",
        "NL-KP-05",
    ],
}

# (instance_id, variant) -> bin_idx -> group -> calls count
Usage = dict[tuple[str, str], list[dict[str, int]]]


def _extract_instance_id(filename: str) -> str:
    """Extract instance identifier from log filename like BR-SA-00_ucb_seed00.jsonl."""
    m = re.match(r"^([A-Z]{2}-[A-Za-z0-9]+-\d+)", filename)
    if m:
        return m.group(1)
    return filename.split("_")[0]


def load_variant_logs(log_dir: Path, variant_name: str, bins: int, usage: Usage) -> int:
    """Load selector_window records from a directory into the usage dictionary."""
    if not log_dir.exists():
        print(f"OSTRZEŻENIE: Katalog {log_dir} nie istnieje!", file=sys.stderr)
        return 0

    log_files = sorted(log_dir.glob("*.jsonl"))
    loaded_files = 0

    for path in log_files:
        inst_id = _extract_instance_id(path.name)
        key = (inst_id, variant_name)

        windows: list[dict[str, Any]] = []
        try:
            with path.open("r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    if '"selector_window"' in line:
                        windows.append(json.loads(line))
        except Exception as e:
            print(f"Błąd odczytu {path}: {e}", file=sys.stderr)
            continue

        if not windows:
            continue

        run_length = max(int(w.get("window_end", 500000)) for w in windows) + 1

        for w in windows:
            w_start = int(w.get("window_start", 0))
            w_end = int(w.get("window_end", w_start))
            middle = (w_start + w_end) / 2
            b = min(bins - 1, int(bins * middle / run_length))

            heuristics = w.get("heuristics", {})
            for h, stats in heuristics.items():
                calls = int(stats.get("calls", 0))
                group = GROUP_OF.get(h, "inne")
                usage[key][b][group] += calls

        loaded_files += 1

    return loaded_files


def _groups_present(usage: Usage) -> list[str]:
    seen = {g for slices in usage.values() for s in slices for g in s}
    ordered = [g for g in GROUP_COLORS if g in seen]
    return ordered + sorted(seen - set(ordered))


def _draw_stacked_cell(
    ax: Axes,
    slices: list[dict[str, int]],
    groups: list[str],
    xs: list[float],
    width: float,
    bins: int,
) -> None:
    """Draw stacked bars into a single cell axis."""
    totals = [sum(s.values()) or 1 for s in slices]
    bottom = [0.0] * bins

    for g in groups:
        share = [100 * s.get(g, 0) / t for s, t in zip(slices, totals, strict=True)]
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
        bottom = [b_val + s_val for b_val, s_val in zip(bottom, share, strict=True)]

    ax.set_ylim(0, 100)
    ax.set_xlim(0, 100)
    ax.tick_params(colors=TEXT_SECONDARY, labelsize=7, length=2)
    for side in ("top", "right", "left", "bottom"):
        ax.spines[side].set_visible(False)


def plot_grid(
    usage: Usage,
    instances: list[str],
    variants: list[str],
    bins: int,
    output_png: Path,
    title_suffix: str = "",
) -> None:
    """Render a multi-row, multi-column comparative stacked bar chart."""
    groups = _groups_present(usage)
    if not groups or not instances:
        return

    n_rows = len(instances)
    n_cols = len(variants)

    fig, axes = plt.subplots(
        n_rows,
        n_cols,
        figsize=(3.8 * n_cols, 2.5 * n_rows + 1.1),
        sharex=True,
        sharey=True,
        squeeze=False,
    )

    xs = [100 * (b + 0.5) / bins for b in range(bins)]
    width = 100 / bins * 0.9

    for i, inst in enumerate(instances):
        for j, var in enumerate(variants):
            ax: Axes = axes[i][j]
            slices = usage.get((inst, var))

            if slices is None:
                ax.set_axis_off()
                continue

            _draw_stacked_cell(ax, slices, groups, xs, width, bins)

            if i == 0:
                ax.set_title(var, color=TEXT_PRIMARY, fontsize=10, fontweight="bold")

            if j == 0:
                ax.set_ylabel(
                    f"{inst}\nudział wywołań [%]",
                    color=TEXT_PRIMARY,
                    fontsize=8,
                    fontweight="semibold",
                )

            if i == n_rows - 1:
                ax.set_xlabel("postęp przebiegu [%]", color=TEXT_SECONDARY, fontsize=8)

    handles = [
        Rectangle((0, 0), 1, 1, color=GROUP_COLORS.get(g, OTHER_COLOR)) for g in groups
    ]
    fig.legend(
        handles,
        groups,
        loc="upper center",
        ncol=min(5, len(groups)),
        frameon=False,
        fontsize=8.5,
        labelcolor=TEXT_PRIMARY,
    )

    caption = (
        f"Suma wywołań ze wszystkich seedów w każdej 1/{bins} przebiegu "
        "(nieudane ruchy nie są logowane)."
    )
    if title_suffix:
        caption = f"{title_suffix} | {caption}"

    fig.text(0.5, 0.005, caption, ha="center", fontsize=7.5, color=TEXT_SECONDARY)
    fig.tight_layout(rect=(0, 0.02, 1, 0.94))

    output_png.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_png, dpi=200, facecolor=SURFACE)
    plt.close(fig)
    print(f"Wygenerowano wykres: {output_png}")


def plot_single_instance(
    usage: Usage,
    instance_id: str,
    variants: list[str],
    bins: int,
    output_png: Path,
) -> None:
    """Render a standalone 2-column comparative bar chart for one instance."""
    groups = _groups_present(usage)
    fig, axes = plt.subplots(
        1,
        len(variants),
        figsize=(9.0, 4.5),
        sharey=True,
        squeeze=False,
    )

    xs = [100 * (b + 0.5) / bins for b in range(bins)]
    width = 100 / bins * 0.9

    for j, var in enumerate(variants):
        ax: Axes = axes[0][j]
        slices = usage.get((instance_id, var))
        if slices is None:
            ax.set_axis_off()
            continue

        _draw_stacked_cell(ax, slices, groups, xs, width, bins)
        ax.set_title(var, color=TEXT_PRIMARY, fontsize=10.5, fontweight="bold", pad=8)
        ax.set_xlabel("postęp przebiegu [%]", color=TEXT_SECONDARY, fontsize=8.5)

        if j == 0:
            ax.set_ylabel("udział wywołań [%]", color=TEXT_SECONDARY, fontsize=8.5)

    handles = [
        Rectangle((0, 0), 1, 1, color=GROUP_COLORS.get(g, OTHER_COLOR)) for g in groups
    ]

    fig.suptitle(
        f"Udział heurystyk w trakcie przebiegu — {instance_id}",
        fontsize=12,
        fontweight="bold",
        y=0.97,
    )

    fig.legend(
        handles,
        groups,
        loc="upper center",
        bbox_to_anchor=(0.5, 0.90),
        ncol=min(4, len(groups)),
        frameon=False,
        fontsize=8.5,
        labelcolor=TEXT_PRIMARY,
    )

    fig.tight_layout(rect=(0, 0.04, 1, 0.77))
    output_png.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_png, dpi=200, facecolor=SURFACE)
    plt.close(fig)


def write_table(
    usage: Usage,
    instances: list[str],
    variants: list[str],
    bins: int,
    output_csv: Path,
) -> None:
    """Export raw percentage shares to a CSV table."""
    groups = _groups_present(usage)
    output_csv.parent.mkdir(parents=True, exist_ok=True)

    with output_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["instance_id", "variant", "progress_pct", *groups])

        for inst in instances:
            for var in variants:
                slices = usage.get((inst, var))
                if not slices:
                    continue
                for b, s in enumerate(slices):
                    total = sum(s.values()) or 1
                    pct = round(100 * (b + 1) / bins)
                    row = [inst, var, pct]
                    for g in groups:
                        row.append(round(100 * s.get(g, 0) / total, 2))
                    writer.writerow(row)

    print(f"Zapisano tabelę danych: {output_csv}")


def _find_dir(preferred: Path, fallbacks: list[Path]) -> Path:
    if preferred.exists():
        return preferred
    for fb in fallbacks:
        if fb.exists():
            return fb
    return preferred


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Porównanie udziału heurystyk: Baseline vs Preprocessing."
    )
    parser.add_argument(
        "--baseline-dir",
        type=Path,
        default=Path("data/logs/logs_baseline_500k"),
        help="Ścieżka do logów baseline (domyślnie data/logs/logs_baseline_500k).",
    )
    parser.add_argument(
        "--adaptive-dir",
        "--preprocessing-dir",
        type=Path,
        default=Path("data/logs/preprocessing_experiments"),
        help=(
            "Ścieżka do logów adaptive/preprocessing "
            "(domyślnie data/logs/preprocessing_experiments)."
        ),
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("docs/wyniki"),
        help="Katalog zapisu wykresów zbiorczych (domyślnie docs/wyniki).",
    )
    parser.add_argument(
        "--plots-dir",
        type=Path,
        default=Path("data/results/plots/heuristic_usage"),
        help="Katalog zapisu indywidualnych wykresów per instancja.",
    )
    parser.add_argument(
        "--bins",
        type=int,
        default=10,
        help="Liczba przedziałów postępu przebiegu (domyślnie 10).",
    )
    parser.add_argument(
        "--instances",
        nargs="*",
        default=None,
        help="Opcjonalny filtr instancji.",
    )

    args = parser.parse_args()

    base_dir = _find_dir(
        args.baseline_dir,
        [
            Path("logs/baseline_500_k"),
            Path("data/logs/baseline_500k"),
            Path("logs/baseline_500k"),
        ],
    )
    adapt_dir = _find_dir(
        args.adaptive_dir,
        [
            Path("logs/preprocessing_experiment"),
            Path("data/logs/logs_adaptive_500k"),
            Path("logs/preprocessing_experiments"),
        ],
    )

    print(f"Katalog logów Baseline:      {base_dir}")
    print(f"Katalog logów Preprocessing: {adapt_dir}")

    usage: Usage = defaultdict(lambda: [defaultdict(int) for _ in range(args.bins)])

    n_base = load_variant_logs(base_dir, "Baseline", args.bins, usage)
    n_adapt = load_variant_logs(adapt_dir, "Preprocessing", args.bins, usage)

    print(f"Załadowano {n_base} logów Baseline oraz {n_adapt} logów Preprocessing.")

    all_instances = sorted({inst for inst, _ in usage})
    if args.instances:
        all_instances = [i for i in all_instances if i in args.instances]

    if not all_instances:
        print(
            "BŁĄD: Nie znaleziono pasujących logów w podanych katalogach!",
            file=sys.stderr,
        )
        sys.exit(1)

    variants = ["Baseline", "Preprocessing"]

    # 1. Main comparison image (directly in docs/wyniki/porownanie_udzial_heurystyk.png)
    main_out = args.out_dir / "porownanie_udzial_heurystyk.png"
    plot_grid(
        usage,
        all_instances,
        variants,
        args.bins,
        main_out,
        title_suffix="Wszystkie instancje",
    )

    # 2. Category-based reports (Small, Medium, Large)
    for scale_name, scale_insts in SCALE_GROUPS.items():
        sub_insts = [i for i in scale_insts if i in all_instances]
        if sub_insts:
            scale_out = args.out_dir / f"porownanie_udzial_heurystyk_{scale_name}.png"
            plot_grid(
                usage,
                sub_insts,
                variants,
                args.bins,
                scale_out,
                title_suffix=f"Instancje: {scale_name.upper()}",
            )

    # 3. Individual plots per instance
    args.plots_dir.mkdir(parents=True, exist_ok=True)
    for inst in all_instances:
        single_out = args.plots_dir / f"{inst}_udzial_heurystyk.png"
        plot_single_instance(usage, inst, variants, args.bins, single_out)

    print(f"Wygenerowano indywidualne wykresy per instancja w: {args.plots_dir}")

    # 4. CSV summary table
    csv_out = args.out_dir / "porownanie_udzial_heurystyk.csv"
    write_table(usage, all_instances, variants, args.bins, csv_out)

    msg = f"\nGotowe! Wykresy zapisano w: {args.out_dir} oraz {args.plots_dir}"
    print(msg)


if __name__ == "__main__":
    main()
