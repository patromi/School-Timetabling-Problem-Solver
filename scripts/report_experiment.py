#!/usr/bin/env python
"""Full report for one experiments.py run: every figure plus the test table.

Writes into --out-dir:
  convergence.{png,pdf,csv}      best-so-far hard cost over time (median, IQR)
  heuristic_usage.{png,pdf,csv}  share of heuristic calls over the run
  acceptance.{png,pdf,csv}       share of candidates accepted over time
  acceptance_breakdown.{png,pdf} ... split into better / equal / worse
                                 (logs with those counters only)
  final_results.{png,pdf}        final hard cost of every seed, per selector
  summary.md                     per-selector table + paired exact Wilcoxon
                                 signed-rank tests (by seed) on the
                                 lexicographic cost (hard first, then soft)

Usage:
    uv run python scripts/report_experiment.py \
        --csv data/results/experiment_selectors_sa.csv \
        --log-dir data/logs/experiment_sa --out-dir data/results/report_sa
"""

import argparse
import csv
import itertools
import json
import statistics
import sys
import textwrap
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import plot_acceptance
import plot_convergence
import plot_heuristic_usage
from matplotlib.ticker import FuncFormatter, LogLocator, MaxNLocator, NullFormatter

# (instance, selector) -> seed -> (infeasibility, objective)
Results = dict[tuple[str, str], dict[int, tuple[int, int]]]


def load_results(path: Path) -> tuple[Results, dict[tuple[str, str], list[int]]]:
    results: Results = defaultdict(dict)
    iterations: dict[tuple[str, str], list[int]] = defaultdict(list)
    with path.open(encoding="utf-8") as f:
        for row in csv.DictReader(f):
            key = (
                row["instance_id"],
                plot_convergence.config_label(row["selector"], row["acceptance"]),
            )
            results[key][int(row["seed"])] = (
                int(row["infeasibility_after"]),
                int(row["objective_after"]),
            )
            iterations[key].append(int(row["iterations_done"]))
    return results, iterations


def wilcoxon_exact(diffs: list[int]) -> float:
    """Two-sided exact p-value of the Wilcoxon signed-rank test; zero
    differences are dropped, tied absolute values get average ranks."""
    d = [x for x in diffs if x != 0]
    n = len(d)
    if n == 0:
        return 1.0
    order = sorted(range(n), key=lambda i: abs(d[i]))
    ranks = [0.0] * n
    i = 0
    while i < n:
        j = i
        while j + 1 < n and abs(d[order[j + 1]]) == abs(d[order[i]]):
            j += 1
        for k in range(i, j + 1):
            ranks[order[k]] = (i + j) / 2 + 1
        i = j + 1
    w = sum(r for r, x in zip(ranks, d, strict=True) if x > 0)
    mean = sum(ranks) / 2
    extreme: int = 0
    for signs in itertools.product((0, 1), repeat=n):
        s = sum(r for r, b in zip(ranks, signs, strict=True) if b)
        extreme += int(abs(s - mean) >= abs(w - mean) - 1e-9)
    return float(extreme) / float(2**n)


def _selectors(results: Results) -> list[str]:
    return plot_convergence.order_labels(sel for _, sel in results)


def hard_breakdown_lines(log_dir: Path) -> list[str]:
    """Markdown section: mean hard cost per constraint type at the start
    and at the end of the run, per (instance, selector). Empty when the
    logs predate the by_type_* fields of run_summary."""
    before: dict[tuple[str, str], list[dict[str, int]]] = defaultdict(list)
    after: dict[tuple[str, str], list[dict[str, int]]] = defaultdict(list)
    for path in sorted(log_dir.glob("*.jsonl")):
        with path.open(encoding="utf-8") as f:
            for line in f:
                if '"run_summary"' not in line:
                    continue
                record = json.loads(line)
                if "by_type_after" not in record:
                    continue
                label = plot_convergence.config_label(
                    str(record["selector"]), str(record.get("acceptance", "lahc"))
                )
                key = (str(record["instance_id"]), label)
                before[key].append(record["by_type_before"]["hard"])
                after[key].append(record["by_type_after"]["hard"])
    if not after:
        return []
    lines = [
        "",
        "## Naruszenia twarde na końcu wg typu ograniczenia",
        "",
        "Średni koszt twardy z seedów: start → koniec. Pokazuje, które "
        "ograniczenia zostają naruszone.",
    ]
    for inst in sorted({k[0] for k in after}):
        present = _selector_order({k[1] for k in after})
        selectors = [s for s in present if (inst, s) in after]
        runs = [r for s in selectors for r in after[(inst, s)] + before[(inst, s)]]
        types = sorted({t for run in runs for t in run})
        lines += [
            "",
            f"### {inst}",
            "",
            "| typ ograniczenia | start | " + " | ".join(selectors) + " |",
            "|---|---|" + "---|" * len(selectors),
        ]
        first = before[(inst, selectors[0])]
        for t in types:
            start = statistics.mean(run.get(t, 0) for run in first)
            ends = [
                statistics.mean(run.get(t, 0) for run in after[(inst, s)])
                for s in selectors
            ]
            lines.append(
                f"| {t} | {start:.1f} | " + " | ".join(f"{e:.1f}" for e in ends) + " |"
            )
    return lines


def _selector_order(present: set[str]) -> list[str]:
    return plot_convergence.order_labels(present)


def write_summary(
    results: Results, iterations: dict[tuple[str, str], list[int]], output: Path
) -> None:
    instances = sorted({inst for inst, _ in results})
    selectors = _selectors(results)
    pairs = list(itertools.combinations(selectors, 2))
    n_tests = max(1, len(instances) * len(pairs))  # 1: a single selector, no pairs
    alpha = 0.05
    lines = [
        "# Podsumowanie eksperymentu",
        "",
        "Koszt leksykograficzny: najpierw naruszenia twarde (inf), "
        "potem miękkie (obj).",
        f"Test Wilcoxona dla par wiązanych (ten sam seed), dokładne p, dwustronny. "
        f"Poprawka Bonferroniego na {n_tests} porównań: próg "
        f"{alpha / n_tests:.4f}.",
    ]
    for inst in instances:
        lines += [
            "",
            f"## {inst}",
            "",
            "| selektor | wykonalne | inf śr. | inf mediana | obj śr. | iteracje śr. |",
            "|---|---|---|---|---|---|",
        ]
        for sel in selectors:
            runs = results.get((inst, sel))
            if not runs:
                continue
            infs = [v[0] for v in runs.values()]
            objs = [v[1] for v in runs.values()]
            lines.append(
                f"| {sel} | {sum(i == 0 for i in infs)}/{len(infs)} "
                f"| {statistics.mean(infs):.1f} | {statistics.median(infs):.1f} "
                f"| {statistics.mean(objs):.1f} "
                f"| {statistics.mean(iterations[(inst, sel)]):.0f} |"
            )
        lines += [
            "",
            "| porównanie | wygrane | p | istotne (Bonferroni) |",
            "|---|---|---|---|",
        ]
        for a, b in pairs:
            ra, rb = results.get((inst, a), {}), results.get((inst, b), {})
            seeds = sorted(set(ra) & set(rb))
            if not seeds:
                continue
            # hard difference dominates: scale it above any soft difference
            diffs = [
                (ra[s][0] - rb[s][0]) * 10**9 + (ra[s][1] - rb[s][1]) for s in seeds
            ]
            p = wilcoxon_exact(diffs)
            wins_a, wins_b = sum(x < 0 for x in diffs), sum(x > 0 for x in diffs)
            lines.append(
                f"| {a} vs {b} | {wins_a}:{wins_b} "
                f"| {p:.3f} | {'tak' if p < alpha / n_tests else 'nie'} |"
            )
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def plot_final(results: Results, output: Path) -> None:
    instances = sorted({inst for inst, _ in results})
    selectors = _selectors(results)
    colors = plot_convergence.label_colors(selectors)
    fig, axes = plt.subplots(
        1,
        len(instances),
        figsize=(max(6.0, 3.6 * len(instances)), 3.4),
        squeeze=False,
    )
    for ax, inst in zip(axes[0], instances, strict=True):
        values = [
            v[0] for sel in selectors for v in results.get((inst, sel), {}).values()
        ]
        for x, sel in enumerate(selectors):
            infs = sorted(v[0] for v in results.get((inst, sel), {}).values())
            if not infs:
                continue
            color = colors[sel]
            # deterministic horizontal spread so equal values stay visible
            offsets = [(k - (len(infs) - 1) / 2) * 0.04 for k in range(len(infs))]
            ax.scatter(
                [x + o for o in offsets],
                infs,
                s=22,
                color=color,
                edgecolors="white",
                linewidths=0.8,
                zorder=3,
            )
            med = statistics.median(infs)
            ax.plot(
                [x - 0.28, x + 0.28],
                [med, med],
                color=plot_convergence.TEXT_PRIMARY,
                linewidth=1.4,
                zorder=4,
            )
        positive = [v for v in values if v > 0]
        if max(values) <= 20:
            # a handful of violations: a linear integer axis reads best
            ax.set_ylim(-0.3, max(values) + 0.5)
            ax.yaxis.set_major_locator(MaxNLocator(integer=True))
        elif min(values) > 0 and max(values) / min(positive) < 10:
            # less than a decade apart: linear keeps the ticks readable
            ax.yaxis.set_major_locator(MaxNLocator(nbins=5))
            ax.yaxis.set_major_formatter(FuncFormatter(plot_convergence._format_count))
        else:
            ax.set_yscale("log")
            ax.yaxis.set_major_locator(LogLocator(base=10, subs=(1.0, 2.0, 5.0)))
            ax.yaxis.set_major_formatter(FuncFormatter(plot_convergence._format_count))
        ax.yaxis.set_minor_formatter(NullFormatter())
        # long configuration names wrap onto several lines instead of overlapping
        names = [textwrap.fill(s, 16) for s in selectors]
        ax.set_xticks(range(len(selectors)), names, fontsize=8)
        ax.set_xlim(-0.6, len(selectors) - 0.4)
        ax.set_title(inst, color=plot_convergence.TEXT_PRIMARY, fontsize=11, loc="left")
        ax.grid(True, axis="y", color=plot_convergence.GRID, linewidth=0.6)
        ax.tick_params(colors=plot_convergence.TEXT_SECONDARY, labelsize=8)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)
        for side in ("left", "bottom"):
            ax.spines[side].set_color(plot_convergence.GRID)
    axes[0][0].set_ylabel(
        "końcowy koszt twardy",
        color=plot_convergence.TEXT_SECONDARY,
        fontsize=9,
    )
    fig.text(
        0.5,
        0.005,
        "Punkt = jeden seed, kreska = mediana. Niżej = lepiej.",
        ha="center",
        fontsize=8,
        color=plot_convergence.TEXT_SECONDARY,
    )
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(output, dpi=200, facecolor="white")
    fig.savefig(output.with_suffix(".pdf"), facecolor="white")
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description="Full experiment report.")
    parser.add_argument("--csv", type=Path, required=True)
    parser.add_argument("--log-dir", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--time-limit", type=float, default=120.0)
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)

    results, iterations = load_results(args.csv)
    write_summary(results, iterations, args.out_dir / "summary.md")
    breakdown = hard_breakdown_lines(args.log_dir)
    if breakdown:
        with (args.out_dir / "summary.md").open("a", encoding="utf-8") as f:
            f.write("\n".join(breakdown) + "\n")
    plot_final(results, args.out_dir / "final_results.png")

    step = max(0.5, args.time_limit / 240)
    grid = [k * step for k in range(round(args.time_limit / step) + 1)]
    curves = plot_convergence.load_curves(args.log_dir, grid)
    plot_convergence.plot(curves, grid, args.out_dir / "convergence.png")
    plot_convergence.write_table(curves, grid, args.out_dir / "convergence.csv")

    usage = plot_heuristic_usage.load_usage(args.log_dir, bins=10)
    plot_heuristic_usage.plot(usage, 10, args.out_dir / "heuristic_usage.png")
    plot_heuristic_usage.write_table(usage, 10, args.out_dir / "heuristic_usage.csv")

    acceptance = plot_acceptance.load_curves(args.log_dir, grid)
    plot_acceptance.plot(acceptance, grid, args.out_dir / "acceptance.png")
    plot_acceptance.write_table(acceptance, grid, args.out_dir / "acceptance.csv")
    plot_acceptance.plot_breakdown(
        args.log_dir, grid, args.out_dir / "acceptance_breakdown.png"
    )

    print(f"Raport zapisany do: {args.out_dir}")


if __name__ == "__main__":
    main()
