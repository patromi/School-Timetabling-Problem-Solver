"""Generate plots for Etap 6 DoD — selector comparison experiment.

Reads:
  data/results/experiment_selectors.csv   — final costs per run
  data/logs/*.jsonl                       — per-window heuristic stats

Writes (to --output-dir, default data/results/figures/):
  comparison.png       — 2 rows x N_instances cols: infeasibility / objective
  call_share_grid.png  — N_instances rows x N_selectors cols: stacked area
  learning_curves.png  — N_instances panels, all selectors per panel

Usage:
    uv run python plot_results.py
    uv run python plot_results.py --csv data/results/experiment_selectors.csv
    uv run python plot_results.py --log-dir data/logs --output-dir data/results/figures
"""

import argparse
import csv
import json
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

matplotlib.rcParams.update(
    {
        "font.size": 10,
        "axes.titlesize": 11,
        "axes.labelsize": 10,
        "figure.dpi": 120,
    }
)

SELECTOR_COLORS = {
    "random": "#4C72B0",
    "epsilon-greedy": "#55A868",
    "ucb": "#C44E52",
}
SELECTOR_LABELS = {
    "random": "Random",
    "epsilon-greedy": "ε-Greedy",
    "ucb": "UCB",
}
HEURISTIC_LABELS: dict[str, str] = {
    "move_random": "move_random",
    "move_best": "move_best",
    "swap": "swap",
    "repair_hard_violation": "repair_hard",
    "resource_reassign": "resource_reassign",
    "kempe_chain": "kempe_chain",
    "ruin_and_recreate": "ruin_recreate",
    "large_perturbation": "large_perturb",
    "small_perturbation": "small_perturb",
    "split_resize": "split_resize",
}
_HEURISTIC_PALETTE = [
    "#1f77b4",
    "#ff7f0e",
    "#2ca02c",
    "#d62728",
    "#9467bd",
    "#8c564b",
    "#e377c2",
    "#7f7f7f",
    "#bcbd22",
    "#17becf",
]


# ── Data loading ──────────────────────────────────────────────────────────────


def _load_csv(path: Path) -> list[dict[str, str]]:
    with open(path, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def _load_jsonl(path: Path) -> list[dict]:  # type: ignore[type-arg]
    records = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                records.append(json.loads(line))
    return records


def _call_share_from_jsonl(
    records: list[dict],  # type: ignore[type-arg]
) -> tuple[list[int], dict[str, list[float]]]:
    """Extract (window_midpoints, {h_id: [share_per_window]}) from JSONL."""
    windows = [r for r in records if r.get("event") == "selector_window"]
    if not windows:
        return [], {}

    all_h_ids: list[str] = []
    for w in windows:
        for h_id in w.get("heuristics", {}):
            if h_id not in all_h_ids:
                all_h_ids.append(h_id)

    midpoints: list[int] = []
    shares: dict[str, list[float]] = {h: [] for h in all_h_ids}
    for w in windows:
        total = sum(w["heuristics"].get(h, {}).get("calls", 0) for h in all_h_ids)
        midpoints.append((w["window_start"] + w["window_end"]) // 2)
        for h in all_h_ids:
            calls = w["heuristics"].get(h, {}).get("calls", 0)
            shares[h].append(calls / total if total > 0 else 0.0)
    return midpoints, shares


def _extract_new_best_curve(
    records: list[dict],  # type: ignore[type-arg]
) -> tuple[list[int], list[int]]:
    """Extract (steps, costs) from new_best records."""
    steps: list[int] = []
    costs: list[int] = []
    for r in records:
        if r.get("event") == "new_best":
            steps.append(r["step"])
            costs.append(r["new_cost"])
    return steps, costs


# ── Plot 1: comparison bar charts (2 rows x N_instances) ─────────────────────


def _plot_comparison_combined(
    rows: list[dict[str, str]],
    output_path: Path,
) -> None:
    """2 rows x N_instances: row 0 = infeasibility, row 1 = objective."""
    metrics = [
        ("infeasibility_after", "Infeasibility (mediana ± IQR)"),
        ("objective_after", "Objective (mediana ± IQR)"),
    ]
    instances = sorted({r["instance_id"] for r in rows})
    selectors = [
        s
        for s in ["random", "epsilon-greedy", "ucb"]
        if any(r["selector"] == s for r in rows)
    ]
    n_inst = len(instances)
    n_sel = len(selectors)
    bar_w = 0.55 / n_sel

    fig, axes = plt.subplots(
        len(metrics),
        n_inst,
        figsize=(4.2 * n_inst, 3.8 * len(metrics)),
        sharey="row",
    )
    # Guarantee 2D array
    axes_2d = np.array(axes).reshape(len(metrics), n_inst)

    for row_i, (metric, ylabel) in enumerate(metrics):
        for col_i, inst in enumerate(instances):
            ax = axes_2d[row_i, col_i]
            for j, sel in enumerate(selectors):
                vals = [
                    float(r[metric])
                    for r in rows
                    if r["instance_id"] == inst and r["selector"] == sel
                ]
                if not vals:
                    continue
                median = float(np.median(vals))
                lo, hi = np.percentile(vals, [25, 75])
                x = (j - (n_sel - 1) / 2) * (bar_w + 0.03)
                ax.bar(
                    x,
                    median,
                    width=bar_w,
                    color=SELECTOR_COLORS.get(sel, "#888"),
                    label=SELECTOR_LABELS.get(sel, sel),
                    yerr=[[max(0.0, median - lo)], [max(0.0, hi - median)]],
                    capsize=4,
                    error_kw={"elinewidth": 1.2, "ecolor": "black"},
                    zorder=3,
                )
            if row_i == 0:
                ax.set_title(inst, fontweight="bold")
            ax.set_xticks([])
            if col_i == 0:
                ax.set_ylabel(ylabel)
            ax.grid(axis="y", alpha=0.35, zorder=0)
            ax.spines[["top", "right"]].set_visible(False)

    handles, labels = axes_2d[0, 0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="upper center",
        ncol=n_sel,
        frameon=False,
        bbox_to_anchor=(0.5, 1.02),
    )
    fig.suptitle("Porownanie selektorow", y=1.05)
    fig.tight_layout()
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Zapisano: {output_path.name}")


# ── Plot 2: call-share grid (N_instances x N_selectors) ──────────────────────


def _fill_call_share_ax(
    ax: plt.Axes,  # type: ignore[name-defined]
    data: tuple[list[int], dict[str, list[float]]] | None,
    ordered: list[str],
    color_map: dict[str, str],
    col_label: str | None,
    row_label: str | None,
    show_xlabel: bool,
) -> None:
    """Draw one stacked-area panel (or a 'brak danych' placeholder)."""
    if data is None or not data[0]:
        ax.text(
            0.5, 0.5, "brak danych", ha="center", va="center",
            transform=ax.transAxes, color="gray", fontsize=9,
        )
    else:
        midpoints, shares = data
        xs = np.array(midpoints)
        ys = [shares.get(h, [0.0] * len(midpoints)) for h in ordered]
        ax.stackplot(xs, ys, colors=[color_map[h] for h in ordered], alpha=0.85)
        if len(xs) > 1:
            ax.set_xlim(xs[0], xs[-1])
        ax.set_ylim(0, 1)
        ax.grid(axis="y", alpha=0.25)
    ax.spines[["top", "right"]].set_visible(False)
    if col_label:
        ax.set_title(col_label, fontweight="bold")
    if row_label:
        ax.set_ylabel(row_label, fontsize=9)
    if show_xlabel:
        ax.set_xlabel("Iteracja", fontsize=9)


def _plot_call_share_grid(
    log_dir: Path,
    instances: list[str],
    selectors: list[str],
    seed: int,
    output_path: Path,
) -> None:
    """N_instances rows x N_selectors cols stacked-area grid with shared legend."""
    # Load all data and collect heuristic IDs
    all_h_ids: list[str] = []
    all_data: dict[tuple[str, str], tuple[list[int], dict[str, list[float]]]] = {}

    for inst in instances:
        for sel in selectors:
            # Accept both new format ({inst}_{sel}_{acc}_seed{n}.jsonl) and old
            candidates = sorted(log_dir.glob(f"{inst}_{sel}_*_seed{seed:02d}.jsonl"))
            lp = candidates[0] if candidates else (
                log_dir / f"{inst}_{sel}_seed{seed:02d}.jsonl"
            )
            if not lp.exists():
                continue
            midpoints, shares = _call_share_from_jsonl(_load_jsonl(lp))
            all_data[(inst, sel)] = (midpoints, shares)
            for h_id in shares:
                if h_id not in all_h_ids:
                    all_h_ids.append(h_id)

    if not all_h_ids:
        print(f"  (brak logów w {log_dir}, pomijam call-share grid)")
        return

    # Sort heuristics by total share across all runs — most-called first
    h_totals = {
        h: sum(sum(all_data[k][1].get(h, [])) for k in all_data) for h in all_h_ids
    }
    ordered = sorted(all_h_ids, key=lambda h: h_totals[h], reverse=True)
    color_map = {
        h: _HEURISTIC_PALETTE[i % len(_HEURISTIC_PALETTE)]
        for i, h in enumerate(ordered)
    }

    n_inst = len(instances)
    n_sel = len(selectors)
    fig, axes = plt.subplots(
        n_inst,
        n_sel,
        figsize=(5 * n_sel, 3 * n_inst),
        sharey=True,
        sharex=False,
    )
    axes_2d = np.array(axes).reshape(n_inst, n_sel)

    for row_i, inst in enumerate(instances):
        for col_i, sel in enumerate(selectors):
            ax = axes_2d[row_i, col_i]
            _fill_call_share_ax(
                ax,
                all_data.get((inst, sel)),
                ordered,
                color_map,
                col_label=SELECTOR_LABELS.get(sel, sel) if row_i == 0 else None,
                row_label=f"{inst}\nudział" if col_i == 0 else None,
                show_xlabel=row_i == n_inst - 1,
            )

    legend_handles = [
        Patch(facecolor=color_map[h], label=HEURISTIC_LABELS.get(h, h))
        for h in ordered
    ]
    fig.legend(
        handles=legend_handles,
        loc="lower center",
        ncol=min(5, len(ordered)),
        bbox_to_anchor=(0.5, -0.06),
        frameon=False,
        fontsize=9,
    )
    fig.suptitle(f"Udzial wywolan heurystyk w czasie (seed={seed})", y=1.01)
    fig.tight_layout()
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Zapisano: {output_path.name}")


# ── Plot 3: learning curves (1 row x N_instances) ────────────────────────────


def _plot_learning_curves_combined(
    log_dir: Path,
    instances: list[str],
    selectors: list[str],
    seed: int,
    output_path: Path,
) -> None:
    """One subplot per instance, all selectors as lines, shared legend."""
    n_inst = len(instances)
    fig, axes = plt.subplots(1, n_inst, figsize=(6 * n_inst, 4))
    axes_1d = [axes] if n_inst == 1 else list(axes)

    for ax, inst in zip(axes_1d, instances, strict=True):
        any_plotted = False
        for sel in selectors:
            candidates = sorted(log_dir.glob(f"{inst}_{sel}_*_seed{seed:02d}.jsonl"))
            lp = candidates[0] if candidates else (
                log_dir / f"{inst}_{sel}_seed{seed:02d}.jsonl"
            )
            if not lp.exists():
                continue
            steps, costs = _extract_new_best_curve(_load_jsonl(lp))
            if not steps:
                continue
            ax.step(
                steps,
                costs,
                where="post",
                color=SELECTOR_COLORS.get(sel, "#888"),
                label=SELECTOR_LABELS.get(sel, sel),
                linewidth=1.8,
            )
            any_plotted = True
        ax.set_title(inst, fontweight="bold")
        ax.set_xlabel("Iteracja")
        ax.set_ylabel("Koszt (scalar)")
        ax.grid(alpha=0.3)
        ax.spines[["top", "right"]].set_visible(False)
        if not any_plotted:
            ax.text(
                0.5,
                0.5,
                "brak danych",
                ha="center",
                va="center",
                transform=ax.transAxes,
                color="gray",
            )

    handles, labels = axes_1d[0].get_legend_handles_labels()
    if handles:
        fig.legend(
            handles,
            labels,
            loc="upper center",
            ncol=len(selectors),
            frameon=False,
            bbox_to_anchor=(0.5, 1.02),
        )
    fig.suptitle(f"Krzywa uczenia — best cost over time (seed={seed})", y=1.05)
    fig.tight_layout()
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  Zapisano: {output_path.name}")


# ── Main ──────────────────────────────────────────────────────────────────────


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generuje wykresy dla eksperymentu porownawczego selektorow."
    )
    parser.add_argument(
        "--csv",
        type=Path,
        default=Path("data/results/experiment_selectors.csv"),
    )
    parser.add_argument(
        "--log-dir",
        type=Path,
        default=Path("data/logs"),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("data/results/figures"),
    )
    parser.add_argument(
        "--call-share-seed",
        type=int,
        default=0,
        help="Seed used for call-share and learning-curve plots (default 0)",
    )
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    print(f"Wykresy → {args.output_dir}/\n")

    instances: list[str] = []
    selectors = ["random", "epsilon-greedy", "ucb"]

    if args.csv.exists():
        rows = _load_csv(args.csv)
        instances = sorted({r["instance_id"] for r in rows})

        print("Wykres porownawczy (2 rzedy x instancje):")
        _plot_comparison_combined(rows, args.output_dir / "comparison.png")
    else:
        print(f"Brak pliku CSV: {args.csv} — pomijam comparison.png")

    if args.log_dir.exists():
        if not instances:
            seen: set[str] = set()
            for p in args.log_dir.glob("*_seed*.jsonl"):
                stem_no_seed = p.stem.rsplit("_seed", 1)[0]
                # Find instance ID as the prefix before the selector name
                inst = stem_no_seed
                for sel in ("random", "epsilon-greedy", "ucb"):
                    idx = stem_no_seed.find(f"_{sel}")
                    if idx != -1:
                        inst = stem_no_seed[:idx]
                        break
                seen.add(inst)
            instances = sorted(seen)

        print("\nWykres udzialu wywolan (instancje x selektory):")
        _plot_call_share_grid(
            args.log_dir,
            instances,
            selectors,
            args.call_share_seed,
            args.output_dir / "call_share_grid.png",
        )

        print("\nKrzywe uczenia (instancje obok siebie):")
        _plot_learning_curves_combined(
            args.log_dir,
            instances,
            selectors,
            args.call_share_seed,
            args.output_dir / "learning_curves.png",
        )
    else:
        print(f"\nBrak katalogu logow: {args.log_dir} — pomijam wykresy okienkowe")

    print("\nGotowe.")


if __name__ == "__main__":
    main()
