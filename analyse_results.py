"""Analiza wyników eksperymentów — Etap 6.

Wczytuje ~/Downloads/all_results.csv i generuje wykresy do data/results/figures/.

Użycie:
    uv run python analyse_results.py
    uv run python analyse_results.py --csv /inna/sciezka.csv
"""

from __future__ import annotations

import argparse
import csv
import warnings
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

matplotlib.rcParams.update(
    {"font.size": 10, "axes.titlesize": 11, "axes.labelsize": 10, "figure.dpi": 130}
)
warnings.filterwarnings("ignore", category=UserWarning)

# ── Stałe ────────────────────────────────────────────────────────────────────

INSTANCES_EXP1 = ["BR-SA-00", "ZA-WD-09", "AU-SA-96", "GR-PA-08", "AU-BG-98"]
INSTANCES_EXP2 = ["BR-SA-00", "ZA-WD-09", "AU-SA-96"]

SEL_ORDER = ["random", "epsilon-greedy", "ucb"]
SEL_COLORS = {"random": "#4C72B0", "epsilon-greedy": "#55A868", "ucb": "#C44E52"}
SEL_LABELS = {"random": "Random", "epsilon-greedy": "ε-Greedy", "ucb": "UCB"}

UCB_VARIANTS = ["ucb", "ucb-c03", "ucb-c2", "ucb-alpha005", "ucb-alpha015",
                "ucb-2state", "ucb-2state-c03"]
UCB_LABELS = {
    "ucb":           "UCB\n(domyślny)",
    "ucb-c03":       "c=0.3",
    "ucb-c2":        "c=2.0",
    "ucb-alpha005":  "α=0.05",
    "ucb-alpha015":  "α=0.15",
    "ucb-2state":    "2-stany",
    "ucb-2state-c03":"2-stany\nc=0.3",
}
UCB_COLORS = [
    "#C44E52", "#e08080", "#a01010",
    "#8172B2", "#5a4a8a",
    "#64B5CD", "#2a7fa0",
]

BUDGET_COLORS = {"random": "#4C72B0", "ucb": "#C44E52"}

# ── Ładowanie i separacja danych ─────────────────────────────────────────────


def load_csv(path: Path) -> list[dict]:
    with open(path, encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        r["infeasibility_after"] = int(r["infeasibility_after"])
        r["objective_after"] = int(r["objective_after"])
        r["infeasibility_before"] = int(r["infeasibility_before"])
        r["elapsed_s"] = float(r["elapsed_s"])
        r["seed"] = int(r["seed"])
        r["iterations_done"] = int(r["iterations_done"])
    return rows


def exp1_rows(rows: list[dict]) -> list[dict]:
    """Główny eksperyment: random/eps-greedy/ucb, 300s, 5 instancji.

    Dla ZA-WD-09 i AU-SA-96 odfiltrowujemy krótsze przebiegi z Exp4
    (zostawiamy elapsed ≥ 250s) i deduplicujemy po (inst, sel, seed).
    """
    base = [
        r for r in rows
        if r["selector"] in SEL_ORDER
        and r["instance_id"] in INSTANCES_EXP1
        and r["elapsed_s"] >= 250.0
    ]
    seen: set[tuple] = set()
    out: list[dict] = []
    for r in base:
        key = (r["instance_id"], r["selector"], r["seed"])
        if key not in seen:
            seen.add(key)
            out.append(r)
    return out


def exp2_rows(rows: list[dict]) -> list[dict]:
    """Tuning UCB: warianty na 3 instancjach."""
    return [
        r for r in rows
        if r["selector"] in UCB_VARIANTS
        and r["instance_id"] in INSTANCES_EXP2
    ]


def exp4_rows(rows: list[dict]) -> list[dict]:
    """Skalowanie budżetu: random/ucb na ZA-WD-09 i AU-SA-96.

    Deduplicujemy po (inst, sel, seed, bucket czasu), żeby ten sam przebieg
    300s z Exp1 nie był liczony dwukrotnie w kubełku 300s.
    """
    def _bucket(t: float) -> int:
        for nom in (60, 120, 300, 600):
            if abs(t - nom) < nom * 0.3:
                return nom
        return round(t)

    base = [
        r for r in rows
        if r["selector"] in ("random", "ucb")
        and r["instance_id"] in ("ZA-WD-09", "AU-SA-96")
    ]
    seen: set[tuple] = set()
    out: list[dict] = []
    for r in base:
        key = (r["instance_id"], r["selector"], r["seed"], _bucket(r["elapsed_s"]))
        if key not in seen:
            seen.add(key)
            out.append(r)
    return out


# ── Narzędzia statystyczne ───────────────────────────────────────────────────


def wilcoxon_p(a: dict[int, int], b: dict[int, int]) -> float:
    """Wilcoxon signed-rank (H1: a > b), parowanie po wspólnych seedach."""
    shared = sorted(set(a) & set(b))
    if len(shared) < 3:
        return float("nan")
    xs = [a[s] for s in shared]
    ys = [b[s] for s in shared]
    if xs == ys:
        return 1.0
    try:
        _, p = stats.wilcoxon(xs, ys, alternative="greater")
        return float(p)
    except Exception:
        return float("nan")


def stars(p: float) -> str:
    if np.isnan(p):
        return "n/a"
    if p < 0.001:
        return "***"
    if p < 0.01:
        return "**"
    if p < 0.05:
        return "*"
    return "ns"


# ── Wykres 1: porównanie selektorów ─────────────────────────────────────────


def plot_exp1_comparison(rows: list[dict], out_dir: Path) -> None:
    instances = [i for i in INSTANCES_EXP1
                 if any(r["instance_id"] == i for r in rows)]
    n = len(instances)

    fig, axes = plt.subplots(2, n, figsize=(3.5 * n, 7))
    axes = np.array(axes).reshape(2, n)

    metrics = [
        ("infeasibility_after", "Naruszenia twarde (mediana ± IQR)"),
        ("objective_after",     "Koszt miękki (mediana ± IQR)"),
    ]

    for col, inst in enumerate(instances):
        for row_i, (metric, ylabel) in enumerate(metrics):
            ax = axes[row_i, col]
            for j, sel in enumerate(SEL_ORDER):
                vals = [r[metric] for r in rows
                        if r["instance_id"] == inst and r["selector"] == sel]
                if not vals:
                    continue
                med = float(np.median(vals))
                lo, hi = float(np.percentile(vals, 25)), float(np.percentile(vals, 75))
                x = j * 0.28
                ax.bar(x, med, width=0.24,
                       color=SEL_COLORS[sel], label=SEL_LABELS[sel],
                       yerr=[[max(0, med - lo)], [max(0, hi - med)]],
                       capsize=4,
                       error_kw={"elinewidth": 1.2, "ecolor": "#333"},
                       zorder=3)
            if row_i == 0:
                ax.set_title(inst, fontweight="bold", fontsize=10)
            ax.set_xticks([])
            if col == 0:
                ax.set_ylabel(ylabel)
            ax.grid(axis="y", alpha=0.3, zorder=0)
            ax.spines[["top", "right"]].set_visible(False)

    handles = [plt.Rectangle((0, 0), 1, 1, color=SEL_COLORS[s], label=SEL_LABELS[s])
               for s in SEL_ORDER]
    fig.legend(handles=handles, loc="upper center", ncol=3,
               frameon=False, bbox_to_anchor=(0.5, 1.01))
    fig.suptitle("Exp1 — Porównanie selektorów (300s, mediana ± IQR, 20 seedów)",
                 y=1.04, fontsize=12)
    fig.tight_layout()
    p = out_dir / "exp1_comparison.png"
    fig.savefig(p, bbox_inches="tight")
    plt.close(fig)
    print(f"  {p.name}")


# ── Wykres 2: odsetek wykonalnych rozwiązań ──────────────────────────────────


def plot_exp1_feasibility(rows: list[dict], out_dir: Path) -> None:
    instances = [i for i in INSTANCES_EXP1
                 if any(r["instance_id"] == i for r in rows)]
    n = len(instances)

    fig, axes = plt.subplots(1, n, figsize=(3.2 * n, 4), sharey=True)
    if n == 1:
        axes = [axes]

    for ax, inst in zip(axes, instances):
        xs, ys, colors = [], [], []
        for j, sel in enumerate(SEL_ORDER):
            vals = [r["infeasibility_after"] for r in rows
                    if r["instance_id"] == inst and r["selector"] == sel]
            if not vals:
                continue
            rate = sum(1 for v in vals if v == 0) / len(vals) * 100
            xs.append(j * 0.28)
            ys.append(rate)
            colors.append(SEL_COLORS[sel])
        bars = ax.bar(xs, ys, width=0.24, color=colors, zorder=3)
        for bar, rate in zip(bars, ys):
            label = f"{rate:.0f}%"
            ypos = rate + 2 if rate > 0 else 3
            ax.text(bar.get_x() + bar.get_width() / 2, ypos,
                    label, ha="center", va="bottom", fontsize=8)
        ax.set_title(inst, fontweight="bold", fontsize=10)
        ax.set_xticks([])
        ax.set_ylim(0, 115)
        ax.grid(axis="y", alpha=0.3, zorder=0)
        ax.spines[["top", "right"]].set_visible(False)

    axes[0].set_ylabel("Odsetek wykonalnych rozwiązań [%]")
    handles = [plt.Rectangle((0, 0), 1, 1, color=SEL_COLORS[s], label=SEL_LABELS[s])
               for s in SEL_ORDER]
    fig.legend(handles=handles, loc="upper center", ncol=3,
               frameon=False, bbox_to_anchor=(0.5, 1.01))
    fig.suptitle("Exp1 — % przebiegów z infeasibility=0", y=1.04, fontsize=12)
    fig.tight_layout()
    p = out_dir / "exp1_feasibility_rate.png"
    fig.savefig(p, bbox_inches="tight")
    plt.close(fig)
    print(f"  {p.name}")


# ── Wykres 3: UCB warianty ────────────────────────────────────────────────────


def plot_exp2_ucb_variants(rows: list[dict], out_dir: Path) -> None:
    variants = [v for v in UCB_VARIANTS
                if any(r["selector"] == v for r in rows)]
    instances = [i for i in INSTANCES_EXP2
                 if any(r["instance_id"] == i for r in rows)]
    n_inst = len(instances)

    fig, axes = plt.subplots(1, n_inst, figsize=(4.5 * n_inst, 5))
    if n_inst == 1:
        axes = [axes]

    for ax, inst in zip(axes, instances):
        data = []
        labels = []
        for var in variants:
            vals = [r["infeasibility_after"] for r in rows
                    if r["selector"] == var and r["instance_id"] == inst]
            if vals:
                data.append(vals)
                labels.append(UCB_LABELS.get(var, var))

        bp = ax.boxplot(data, patch_artist=True, widths=0.55,
                        medianprops={"color": "black", "linewidth": 2},
                        flierprops={"marker": ".", "markersize": 4, "alpha": 0.5},
                        whiskerprops={"linewidth": 1.2},
                        capprops={"linewidth": 1.2})
        for patch, color in zip(bp["boxes"],
                                UCB_COLORS[:len(data)]):
            patch.set_facecolor(color)
            patch.set_alpha(0.8)

        ax.set_xticklabels(labels, fontsize=8)
        ax.set_title(inst, fontweight="bold")
        ax.set_ylabel("Naruszenia twarde")
        ax.grid(axis="y", alpha=0.3)
        ax.spines[["top", "right"]].set_visible(False)

    fig.suptitle("Exp2 — Wrażliwość UCB na parametry (lahc, 300s, 10 seedów)",
                 fontsize=12)
    fig.tight_layout()
    p = out_dir / "exp2_ucb_variants.png"
    fig.savefig(p, bbox_inches="tight")
    plt.close(fig)
    print(f"  {p.name}")


# ── Wykres 4: skalowanie budżetu czasu ───────────────────────────────────────


def plot_exp4_budget(rows: list[dict], out_dir: Path) -> None:
    instances = ["ZA-WD-09", "AU-SA-96"]
    selectors = ["random", "ucb"]

    # Szufladkuj czas do nominalnych wartości
    def bucket(t: float) -> int:
        for nom in (60, 120, 300, 600):
            if abs(t - nom) < nom * 0.3:
                return nom
        return round(t)

    budgets = sorted({bucket(r["elapsed_s"]) for r in rows})
    n_inst = len([i for i in instances if any(r["instance_id"] == i for r in rows)])
    if n_inst == 0:
        return

    fig, axes = plt.subplots(1, n_inst, figsize=(5 * n_inst, 4))
    if n_inst == 1:
        axes = [axes]

    for ax, inst in zip(axes, instances):
        if not any(r["instance_id"] == inst for r in rows):
            continue
        for sel in selectors:
            medians, q25s, q75s = [], [], []
            for b in budgets:
                vals = [r["infeasibility_after"] for r in rows
                        if r["instance_id"] == inst
                        and r["selector"] == sel
                        and bucket(r["elapsed_s"]) == b]
                if not vals:
                    medians.append(None)
                    q25s.append(None)
                    q75s.append(None)
                else:
                    medians.append(float(np.median(vals)))
                    q25s.append(float(np.percentile(vals, 25)))
                    q75s.append(float(np.percentile(vals, 75)))

            valid = [(b, m, lo, hi) for b, m, lo, hi
                     in zip(budgets, medians, q25s, q75s) if m is not None]
            if not valid:
                continue
            xs, ms, los, his = zip(*valid)
            ax.plot(xs, ms, "o-", color=BUDGET_COLORS[sel],
                    label=SEL_LABELS[sel], linewidth=2, markersize=6)
            ax.fill_between(xs, los, his,
                            color=BUDGET_COLORS[sel], alpha=0.15)

        ax.set_xlabel("Budżet czasu [s]")
        ax.set_ylabel("Naruszenia twarde (mediana)")
        ax.set_title(inst, fontweight="bold")
        ax.set_xticks(budgets)
        ax.legend(frameon=False)
        ax.grid(alpha=0.3)
        ax.spines[["top", "right"]].set_visible(False)

    fig.suptitle("Exp4 — Wpływ budżetu czasu na jakość (mediana ± IQR, 15 seedów)",
                 fontsize=12)
    fig.tight_layout()
    p = out_dir / "exp4_budget_scaling.png"
    fig.savefig(p, bbox_inches="tight")
    plt.close(fig)
    print(f"  {p.name}")


# ── Wydruk statystyk ─────────────────────────────────────────────────────────


def print_stats(rows: list[dict]) -> None:
    e1 = exp1_rows(rows)
    print("\n═══ Exp1 — statystyki (mediana infeasibility) ═══")
    header = f"{'Instancja':12} {'Random':>10} {'ε-Greedy':>10} {'UCB':>10}  p(R>UCB)  p(ε>UCB)"
    print(header)
    print("─" * len(header))
    for inst in INSTANCES_EXP1:
        # Pairing by seed for valid Wilcoxon — {seed: cost}
        vals = {
            sel: {r["seed"]: r["infeasibility_after"] for r in e1
                  if r["instance_id"] == inst and r["selector"] == sel}
            for sel in SEL_ORDER
        }
        if not any(vals.values()):
            continue
        meds = {
            s: (int(np.median(list(v.values()))) if v else "—")
            for s, v in vals.items()
        }
        pr = wilcoxon_p(vals["random"], vals["ucb"])
        pe = wilcoxon_p(vals["epsilon-greedy"], vals["ucb"])
        print(f"{inst:12} {str(meds['random']):>10} {str(meds['epsilon-greedy']):>10}"
              f" {str(meds['ucb']):>10}  {stars(pr):>8}  {stars(pe):>8}")
    print("Legenda: * p<0.05  ** p<0.01  *** p<0.001  ns=brak istotności")
    print("         Wilcoxon (H1: baseline > UCB, jeden ogon)")

    print("\n═══ Exp1 — odsetek wykonalnych [%] ═══")
    for inst in INSTANCES_EXP1:
        parts = []
        for sel in SEL_ORDER:
            vals = [r["infeasibility_after"] for r in e1
                    if r["instance_id"] == inst and r["selector"] == sel]
            if vals:
                rate = sum(1 for v in vals if v == 0) / len(vals) * 100
                parts.append(f"{SEL_LABELS[sel]}: {rate:.0f}%")
        if parts:
            print(f"  {inst:12}  {',  '.join(parts)}")

    e2 = exp2_rows(rows)
    if e2:
        print("\n═══ Exp2 — UCB warianty (mediana infeasibility, 3 instancje łącznie) ═══")
        for var in UCB_VARIANTS:
            vals = [r["infeasibility_after"] for r in e2 if r["selector"] == var]
            if vals:
                print(f"  {UCB_LABELS.get(var, var):16} n={len(vals):3}  "
                      f"mediana={int(np.median(vals)):>6}  "
                      f"min={min(vals):>5}  zeros={vals.count(0)}")


# ── main ─────────────────────────────────────────────────────────────────────


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", type=Path,
                    default=Path.home() / "Downloads" / "all_results.csv")
    ap.add_argument("--output-dir", type=Path,
                    default=Path("data/results/figures"))
    args = ap.parse_args()

    if not args.csv.exists():
        print(f"Brak pliku: {args.csv}")
        return

    args.output_dir.mkdir(parents=True, exist_ok=True)
    rows = load_csv(args.csv)
    print(f"Wczytano {len(rows)} wierszy → {args.output_dir}/\n")

    e1 = exp1_rows(rows)
    e2 = exp2_rows(rows)
    e4 = exp4_rows(rows)

    print("Generowanie wykresów:")
    plot_exp1_comparison(e1, args.output_dir)
    plot_exp1_feasibility(e1, args.output_dir)
    if e2:
        plot_exp2_ucb_variants(e2, args.output_dir)
    if e4:
        plot_exp4_budget(e4, args.output_dir)

    print_stats(rows)
    print("\nGotowe.")


if __name__ == "__main__":
    main()
