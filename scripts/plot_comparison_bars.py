#!/usr/bin/env python3
"""Generate comparative bar charts for each instance from baseline and adaptive results.

For each instance, creates a high-resolution figure with two subplots:
1. Top subplot: Iterations to Feasibility (first_feasible_iteration when hard = 0)
   across seeds + average, comparing Baseline vs Adaptive (Preprocessing).
2. Bottom subplot: Final Objective Value (objective_after)
   across seeds + average, comparing Baseline vs Adaptive.

Usage:
    uv run python scripts/plot_comparison_bars.py
    uv run python scripts/plot_comparison_bars.py --out-dir data/results/plots
    uv run python scripts/plot_comparison_bars.py --instances GR-PA-08 IT-I4-96
"""

from __future__ import annotations

import argparse
import csv
import os
import sys
import tempfile
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

# Ensure matplotlib uses a writable temp cache directory on Windows
os.environ.setdefault("MPLCONFIGDIR", tempfile.gettempdir())

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.axes import Axes
from matplotlib.container import BarContainer

# Palette: professional, color-blind friendly contrasting colors
COLOR_BASELINE = "#2b5c8f"  # Steel Navy
COLOR_ADAPTIVE = "#1b9e77"  # Teal / Emerald Green
COLOR_INFEASIBLE = "#d95f02"  # Rust / Orange-Red for warnings
COLOR_BG_GRID = "#e6e6e6"


@dataclass(frozen=True)
class RunRecord:
    instance_id: str
    seed: int
    mode: str
    iterations_done: int
    infeasibility_after: int
    objective_after: int
    first_feasible_iteration: int | None
    is_feasible: bool
    scale: str
    tightness: str
    events_count: int
    resources_count: int
    constraints_count: int


def _parse_row(row: dict[str, str], default_mode: str) -> RunRecord:
    inst = row["instance_id"]
    seed = int(row["seed"])
    mode = row.get("mode", default_mode) or default_mode

    inf_after = int(float(row.get("infeasibility_after", 0)))
    obj_after = int(float(row.get("objective_after", 0)))
    it_done = int(float(row.get("iterations_done", 500000)))

    feas_val = row.get("first_feasible_iteration", "").strip()
    feas_step: int | None = (
        int(float(feas_val)) if feas_val and feas_val.lower() != "none" else None
    )

    is_feas_raw = row.get("is_feasible", "").strip().lower()
    is_feasible = is_feas_raw in ("true", "1") or (inf_after == 0)

    return RunRecord(
        instance_id=inst,
        seed=seed,
        mode=mode,
        iterations_done=it_done,
        infeasibility_after=inf_after,
        objective_after=obj_after,
        first_feasible_iteration=feas_step,
        is_feasible=is_feasible,
        scale=row.get("scale", "medium"),
        tightness=row.get("tightness", "moderate"),
        events_count=int(row.get("events_count", 0)),
        resources_count=int(row.get("resources_count", 0)),
        constraints_count=int(row.get("constraints_count", 0)),
    )


def load_dataset(path: Path, default_mode: str) -> dict[str, dict[int, RunRecord]]:
    """Load runs grouped by instance_id -> {seed -> RunRecord}."""
    if not path.exists():
        raise FileNotFoundError(f"Nie znaleziono pliku: {path}")

    results: dict[str, dict[int, RunRecord]] = defaultdict(dict)
    with path.open("r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rec = _parse_row(row, default_mode)
            results[rec.instance_id][rec.seed] = rec
    return results


def _format_num(val: float | int) -> str:
    """Format large numbers compactly for plot labels (e.g. 12.3k or 542)."""
    if val >= 100_000:
        return f"{val / 1000:.0f}k"
    if val >= 10_000:
        return f"{val / 1000:.1f}k"
    if val >= 1000:
        return f"{val:,.0f}".replace(",", " ")
    if isinstance(val, float):
        return f"{val:.1f}"
    return str(val)


def _draw_labels_top(
    ax: Axes,
    rects: BarContainer,
    values: list[float],
    feasibles: list[bool],
    hard_vals: list[int],
    bar_color: str,
) -> None:
    """Add annotations above iteration bars (or mark infeasible)."""
    for rect, val, feas, h in zip(rects, values, feasibles, hard_vals, strict=False):
        if feas and val > 0:
            ax.annotate(
                _format_num(val),
                xy=(rect.get_x() + rect.get_width() / 2, rect.get_height()),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center",
                va="bottom",
                fontsize=8,
                fontweight="semibold",
                color=bar_color,
                rotation=0,
            )
        else:
            # Mark infeasible run
            ax.annotate(
                f"Niewyk.\n(H={_format_num(h)})",
                xy=(rect.get_x() + rect.get_width() / 2, 0),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center",
                va="bottom",
                fontsize=7,
                color=COLOR_INFEASIBLE,
                fontstyle="italic",
            )


def _draw_labels_bottom(
    ax: Axes, rects: BarContainer, values: list[float], bar_color: str
) -> None:
    """Add annotations above objective bars."""
    for rect, val in zip(rects, values, strict=False):
        ax.annotate(
            _format_num(val),
            xy=(rect.get_x() + rect.get_width() / 2, rect.get_height()),
            xytext=(0, 3),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8,
            fontweight="semibold",
            color=bar_color,
            rotation=0,
        )


def plot_instance_bars(
    instance_id: str,
    base_seeds_map: dict[int, RunRecord],
    adapt_seeds_map: dict[int, RunRecord],
    out_path: Path,
    dpi: int = 300,
) -> None:
    """Plot grouped bar charts for a single instance comparing Baseline vs Adaptive."""
    all_seeds = sorted(set(base_seeds_map.keys()) | set(adapt_seeds_map.keys()))
    if not all_seeds:
        return

    sample_rec = next(
        iter(adapt_seeds_map.values()),
        next(iter(base_seeds_map.values()), None),
    )
    scale = sample_rec.scale if sample_rec else "N/A"
    tightness = sample_rec.tightness if sample_rec else "N/A"
    events = sample_rec.events_count if sample_rec else 0
    resources = sample_rec.resources_count if sample_rec else 0
    constraints = sample_rec.constraints_count if sample_rec else 0

    b_feas_steps: list[float] = []
    a_feas_steps: list[float] = []
    b_feas_flags: list[bool] = []
    a_feas_flags: list[bool] = []
    b_hard_vals: list[int] = []
    a_hard_vals: list[int] = []
    b_objs: list[float] = []
    a_objs: list[float] = []

    for s in all_seeds:
        b_rec = base_seeds_map.get(s)
        a_rec = adapt_seeds_map.get(s)

        if b_rec:
            b_feas_flags.append(b_rec.is_feasible)
            b_feas_steps.append(float(b_rec.first_feasible_iteration or 0))
            b_hard_vals.append(b_rec.infeasibility_after)
            b_objs.append(float(b_rec.objective_after))
        else:
            b_feas_flags.append(False)
            b_feas_steps.append(0.0)
            b_hard_vals.append(0)
            b_objs.append(0.0)

        if a_rec:
            a_feas_flags.append(a_rec.is_feasible)
            a_feas_steps.append(float(a_rec.first_feasible_iteration or 0))
            a_hard_vals.append(a_rec.infeasibility_after)
            a_objs.append(float(a_rec.objective_after))
        else:
            a_feas_flags.append(False)
            a_feas_steps.append(0.0)
            a_hard_vals.append(0)
            a_objs.append(0.0)

    # Compute Averages
    b_feas_valid = [v for v, f in zip(b_feas_steps, b_feas_flags, strict=False) if f]
    a_feas_valid = [v for v, f in zip(a_feas_steps, a_feas_flags, strict=False) if f]

    b_feas_avg = float(np.mean(b_feas_valid)) if b_feas_valid else 0.0
    a_feas_avg = float(np.mean(a_feas_valid)) if a_feas_valid else 0.0
    b_obj_avg = float(np.mean(b_objs)) if b_objs else 0.0
    a_obj_avg = float(np.mean(a_objs)) if a_objs else 0.0

    x_labels = [f"Seed {s}" for s in all_seeds] + ["Średnia"]
    b_feas_plot = [*b_feas_steps, b_feas_avg]
    a_feas_plot = [*a_feas_steps, a_feas_avg]
    b_feas_flags_plot = [*b_feas_flags, bool(b_feas_valid)]
    a_feas_flags_plot = [*a_feas_flags, bool(a_feas_valid)]
    b_hard_plot = [*b_hard_vals, int(np.mean(b_hard_vals))]
    a_hard_plot = [*a_hard_vals, int(np.mean(a_hard_vals))]

    b_obj_plot = [*b_objs, b_obj_avg]
    a_obj_plot = [*a_objs, a_obj_avg]

    n_groups = len(x_labels)
    x = np.arange(n_groups)
    bar_width = 0.36

    fig, (ax_top, ax_bot) = plt.subplots(
        nrows=2,
        ncols=1,
        figsize=(12, 8.5),
        sharex=True,
        gridspec_kw={"height_ratios": [1, 1], "hspace": 0.16},
    )

    # =========================================================================
    # Subplot 1: Iterations to Feasibility (hard = 0)
    # =========================================================================
    rects1 = ax_top.bar(
        x - bar_width / 2,
        b_feas_plot,
        bar_width,
        label="Baseline (bez preprocessingu)",
        color=COLOR_BASELINE,
        edgecolor="black",
        linewidth=0.7,
        alpha=0.9,
    )
    rects2 = ax_top.bar(
        x + bar_width / 2,
        a_feas_plot,
        bar_width,
        label="Adaptive (z preprocessingiem)",
        color=COLOR_ADAPTIVE,
        edgecolor="black",
        linewidth=0.7,
        alpha=0.9,
    )

    _draw_labels_top(
        ax_top,
        rects1,
        b_feas_plot,
        b_feas_flags_plot,
        b_hard_plot,
        COLOR_BASELINE,
    )
    _draw_labels_top(
        ax_top,
        rects2,
        a_feas_plot,
        a_feas_flags_plot,
        a_hard_plot,
        COLOR_ADAPTIVE,
    )

    max_feas = max(max(b_feas_plot, default=0), max(a_feas_plot, default=0))
    if max_feas > 0:
        ax_top.set_ylim(0, max_feas * 1.22)
    else:
        ax_top.set_ylim(0, 100)

    ax_top.set_ylabel(
        "Krok osiągnięcia wykonalności\n(liczba iteracji gdy hard = 0)",
        fontsize=10,
        fontweight="bold",
    )
    ax_top.grid(axis="y", linestyle="--", alpha=0.5)
    ax_top.legend(loc="upper left", framealpha=0.9)

    sep_x = n_groups - 1.5
    ax_top.axvline(x=sep_x, color="gray", linestyle=":", linewidth=1.2, alpha=0.8)

    total_runs = len(all_seeds)
    b_feas_count = sum(1 for f in b_feas_flags if f)
    a_feas_count = sum(1 for f in a_feas_flags if f)
    feas_info = (
        f"Wykonalność (hard=0): Baseline {b_feas_count}/{total_runs} "
        f"| Adaptive {a_feas_count}/{total_runs}"
    )
    ax_top.set_title(feas_info, fontsize=10, color="#444444", pad=6)

    # =========================================================================
    # Subplot 2: Objective After Calculations
    # =========================================================================
    rects3 = ax_bot.bar(
        x - bar_width / 2,
        b_obj_plot,
        bar_width,
        color=COLOR_BASELINE,
        edgecolor="black",
        linewidth=0.7,
        alpha=0.9,
    )
    rects4 = ax_bot.bar(
        x + bar_width / 2,
        a_obj_plot,
        bar_width,
        color=COLOR_ADAPTIVE,
        edgecolor="black",
        linewidth=0.7,
        alpha=0.9,
    )

    _draw_labels_bottom(ax_bot, rects3, b_obj_plot, COLOR_BASELINE)
    _draw_labels_bottom(ax_bot, rects4, a_obj_plot, COLOR_ADAPTIVE)

    max_obj = max(max(b_obj_plot, default=0), max(a_obj_plot, default=0))
    if max_obj > 0:
        ax_bot.set_ylim(0, max_obj * 1.18)
    else:
        ax_bot.set_ylim(0, 10)

    ax_bot.set_ylabel(
        "Końcowa funkcja celu (objective)\n(kara ograniczeń miękkich)",
        fontsize=10,
        fontweight="bold",
    )
    ax_bot.set_xlabel("Ziarno losowości (Seed)", fontsize=10, fontweight="bold")
    ax_bot.set_xticks(x)
    ax_bot.set_xticklabels(x_labels, fontsize=9.5)
    ax_bot.grid(axis="y", linestyle="--", alpha=0.5)

    ax_bot.axvline(x=sep_x, color="gray", linestyle=":", linewidth=1.2, alpha=0.8)

    for label in ax_bot.get_xticklabels():
        if label.get_text() == "Średnia":
            label.set_fontweight("bold")
            label.set_color("#990000")

    # =========================================================================
    # Suptitle
    # =========================================================================
    meta_str = (
        f"Skala: {scale} | Tightness: {tightness} | Wydarzenia: {events} "
        f"| Zasoby: {resources} | Ograniczenia: {constraints}"
    )
    fig.suptitle(
        f"Porównanie Baseline vs Adaptive — Instancja: {instance_id}\n({meta_str})",
        fontsize=13,
        fontweight="bold",
        y=0.98,
    )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, dpi=dpi, bbox_inches="tight")
    plt.close(fig)
    print(f"Wygenerowano wykres: {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Tworzy wykresy słupkowe porównujące Baseline vs Adaptive."
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        default=Path("data/results/baseline_experiments_500k.csv"),
        help="Ścieżka do pliku CSV z wynikami baseline.",
    )
    parser.add_argument(
        "--adaptive",
        type=Path,
        default=Path("data/results/preprocessing_experiments_500k.csv"),
        help="Ścieżka do pliku CSV z wynikami adaptive / preprocessing.",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("data/results/plots/comparison"),
        help="Katalog zapisu wygenerowanych wykresów.",
    )
    parser.add_argument(
        "--instances",
        nargs="*",
        default=None,
        help="Opcjonalna lista instancji do wygenerowania (domyślnie wszystkie).",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=300,
        help="Rozdzielczość generowanych obrazów PNG (domyślnie 300 DPI).",
    )
    parser.add_argument(
        "--format",
        type=str,
        default="png",
        choices=["png", "pdf", "svg"],
        help="Format pliku graficznego (domyślnie png).",
    )

    args = parser.parse_args()

    baseline_path = args.baseline
    if not baseline_path.exists():
        cand = Path("data/results/baseline_experiment.csv")
        if cand.exists():
            baseline_path = cand

    adaptive_path = args.adaptive
    if not adaptive_path.exists():
        cand = Path("data/results/preprocessing_experiment.csv")
        if cand.exists():
            adaptive_path = cand

    if not baseline_path.exists():
        print(f"BŁĄD: Nie znaleziono: {baseline_path}", file=sys.stderr)
        sys.exit(1)

    if not adaptive_path.exists():
        print(f"BŁĄD: Nie znaleziono: {adaptive_path}", file=sys.stderr)
        sys.exit(1)

    print(f"Wczytywanie Baseline z: {baseline_path}")
    base_data = load_dataset(baseline_path, default_mode="baseline")

    print(f"Wczytywanie Adaptive z: {adaptive_path}")
    adapt_data = load_dataset(adaptive_path, default_mode="adaptive")

    all_instances = sorted(set(base_data.keys()) | set(adapt_data.keys()))
    if args.instances:
        all_instances = [i for i in all_instances if i in args.instances]

    if not all_instances:
        print("Brak pasujących instancji do wygenerowania wykresów.")
        sys.exit(1)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    msg = (
        f"Generowanie wykresów dla {len(all_instances)} instancji w: {args.out_dir}..."
    )
    print(msg)

    for inst in all_instances:
        base_seeds = base_data.get(inst, {})
        adapt_seeds = adapt_data.get(inst, {})
        plot_file = args.out_dir / f"{inst}_comparison.{args.format}"
        plot_instance_bars(
            instance_id=inst,
            base_seeds_map=base_seeds,
            adapt_seeds_map=adapt_seeds,
            out_path=plot_file,
            dpi=args.dpi,
        )

    print(f"\nGotowe! Wygenerowano {len(all_instances)} wykresów w: {args.out_dir}")


if __name__ == "__main__":
    main()
