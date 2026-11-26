"""Analiza i wykresy eksperymentu EXP-L1 (DeepSeek-R1, warianty promptu a/b/rag).

Wczytuje:
  data/results/exp_l1_r1_10000it.csv
  data/results/exp_l1_r1_100000it.csv
  (opcjonalnie) data/results/exp_l1_r1_300000it.csv

Zapisuje do data/results/figures/exp_l1/:
  feasibility.png       — odsetek wykonalnych planów per instancja × budżet
  llm_overhead.png      — czas LLM vs czas szukania per przebieg
  llm_funnel.png        — lejek: wyzwolenia → wywołania → kandydaci → zaakceptowani
  variant_check.png     — porównanie wariantów a/b/rag (sprawdzenie identyczności)

Użycie:
  uv run python scripts/plot_exp_l1.py
  uv run python scripts/plot_exp_l1.py --out data/results/figures/exp_l1
"""

from __future__ import annotations

import argparse
import csv
import warnings
from collections import defaultdict
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

matplotlib.rcParams.update({
    "font.size": 10,
    "axes.titlesize": 11,
    "axes.labelsize": 10,
    "figure.dpi": 130,
})
warnings.filterwarnings("ignore", category=UserWarning)

VARIANT_COLORS = {"a": "#4C72B0", "b": "#55A868", "rag": "#C44E52", "none": "#808080"}
VARIANT_LABELS = {"a": "Wariant A", "b": "Wariant B", "rag": "Wariant RAG", "none": "UCB (bez LLM)"}
BUDGET_COLORS  = {"10000": "#4C72B0", "100000": "#C44E52", "300000": "#55A868"}
BUDGET_LABELS  = {"10000": "10k iter.", "100000": "100k iter.", "300000": "300k iter."}

INSTANCES_ORDER = [
    "BR-SA-00", "BR-SM-00", "BR-SN-00",
    "FI-WP-06", "FI-MP-06", "FI-PB-98",
    "GR-H1-97", "GR-P3-10", "GR-PA-08",
    "IT-I4-96", "KS-PR-11",
    "ZA-LW-09", "ZA-WD-09",
]


# ── wczytywanie danych ────────────────────────────────────────────────────────

def load_csv(path: Path) -> list[dict]:
    with open(path) as f:
        return list(csv.DictReader(f))


def as_float(v: str) -> float:
    try:
        return float(v)
    except ValueError:
        return 0.0


def as_int(v: str) -> int:
    try:
        return int(v)
    except ValueError:
        return 0


def budget_key(rows: list[dict]) -> str:
    its = {r["iterations_done"] for r in rows}
    return max(its, key=int)


# ── wykres 1: wykonalność per instancja × budżet ─────────────────────────────

def plot_feasibility(datasets: dict[str, list[dict]], out: Path) -> None:
    """Słupki: odsetek wykonalnych planów per instancja dla każdego budżetu.

    Warianty LLM są identyczne (0 zaakceptowanych), więc uśredniamy po wariantach.
    """
    instances = [i for i in INSTANCES_ORDER
                 if any(r["instance_id"] == i for rows in datasets.values() for r in rows)]

    budgets = sorted(datasets.keys(), key=int)
    n_inst = len(instances)
    n_bud  = len(budgets)
    width  = 0.8 / n_bud
    x      = np.arange(n_inst)

    fig, ax = plt.subplots(figsize=(13, 4.5))

    for bi, budget in enumerate(budgets):
        rows = datasets[budget]
        feasible_rate = []
        for inst in instances:
            inst_rows = [r for r in rows if r["instance_id"] == inst]
            if inst_rows:
                rate = sum(1 for r in inst_rows if as_int(r["infeasibility_after"]) == 0) / len(inst_rows)
            else:
                rate = 0.0
            feasible_rate.append(rate * 100)

        offset = (bi - (n_bud - 1) / 2) * width
        bars = ax.bar(x + offset, feasible_rate, width * 0.9,
                      color=BUDGET_COLORS[budget], label=BUDGET_LABELS[budget],
                      zorder=3)
        for bar, val in zip(bars, feasible_rate):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 1.5,
                        f"{val:.0f}", ha="center", va="bottom", fontsize=7.5, zorder=4)

    ax.set_xticks(x)
    ax.set_xticklabels(instances, rotation=35, ha="right", fontsize=9)
    ax.set_ylabel("Plany wykonalne [%]")
    ax.set_ylim(0, 115)
    ax.set_title("EXP-L1 (DeepSeek-R1): odsetek wykonalnych planów per instancja\n"
                 "(warianty a/b/rag uśrednione — identyczne wyniki, 0 heurystyk zaakceptowanych)")
    ax.legend()
    ax.grid(axis="y", alpha=0.4, zorder=0)
    ax.axhline(100, color="gray", linewidth=0.8, linestyle="--", alpha=0.5)
    fig.tight_layout()
    fig.savefig(out / "feasibility.png")
    plt.close(fig)
    print(f"  → {out / 'feasibility.png'}")


# ── wykres 2: narzut LLM ────────────────────────────────────────────────────

def plot_llm_overhead(datasets: dict[str, list[dict]], out: Path) -> None:
    """Scatter/bar: czas szukania vs czas LLM per przebieg (tylko 100k i 300k)."""
    budgets_with_llm = {b: rows for b, rows in datasets.items()
                        if any(as_int(r["llm_calls"]) > 0 for r in rows)}
    if not budgets_with_llm:
        print("  (llm_overhead: brak przebiegów z wywołaniami LLM — pomijam)")
        return

    fig, axes = plt.subplots(1, len(budgets_with_llm), figsize=(6 * len(budgets_with_llm), 4.5),
                             squeeze=False)

    for ax, (budget, rows) in zip(axes[0], sorted(budgets_with_llm.items(), key=lambda x: int(x[0]))):
        rows_llm = [r for r in rows if as_int(r["llm_calls"]) > 0]
        elapsed  = [as_float(r["elapsed_s"]) / 60 for r in rows_llm]  # minuty
        llm_t    = [as_float(r["llm_s"]) / 60 for r in rows_llm]
        search_t = [e - l for e, l in zip(elapsed, llm_t)]

        ax.scatter(search_t, llm_t, alpha=0.4, s=15, color=BUDGET_COLORS[budget])
        # linia y = x (LLM = tyle co szukanie)
        lim = max(max(search_t, default=1), max(llm_t, default=1)) * 1.05
        ax.plot([0, lim], [0, lim], "k--", linewidth=0.8, alpha=0.5, label="LLM = szukanie")
        ax.set_xlabel("Czas szukania [min]")
        ax.set_ylabel("Czas LLM [min]")
        ax.set_title(f"{BUDGET_LABELS[budget]}: czas LLM vs szukanie\n"
                     f"(n={len(rows_llm)}, śr. LLM: {np.mean(llm_t):.1f} min, "
                     f"śr. szukanie: {np.mean(search_t):.1f} min)")
        ax.legend(fontsize=8)
        ax.grid(alpha=0.3)

        # statystyki w rogu
        ratio = np.mean(llm_t) / (np.mean(elapsed) + 1e-9) * 100
        ax.text(0.97, 0.05, f"LLM: {ratio:.0f}% łącznego czasu",
                transform=ax.transAxes, ha="right", va="bottom", fontsize=9,
                bbox=dict(boxstyle="round,pad=0.3", facecolor="wheat", alpha=0.6))

    fig.tight_layout()
    fig.savefig(out / "llm_overhead.png")
    plt.close(fig)
    print(f"  → {out / 'llm_overhead.png'}")


# ── wykres 3: lejek LLM ─────────────────────────────────────────────────────

def plot_llm_funnel(datasets: dict[str, list[dict]], out: Path) -> None:
    """Słupki: wyzwolenia, wywołania API, kandydaci, zaakceptowani — per instancja."""
    budgets_with_llm = {b: rows for b, rows in datasets.items()
                        if any(as_int(r["llm_calls"]) > 0 for r in rows)}
    if not budgets_with_llm:
        print("  (llm_funnel: brak wywołań LLM — pomijam)")
        return

    # agreguj po instancji (suma po seedach i wariantach)
    for budget, rows in sorted(budgets_with_llm.items(), key=lambda x: int(x[0])):
        instances = [i for i in INSTANCES_ORDER if any(r["instance_id"] == i for r in rows)]

        triggers   = []
        calls      = []
        candidates = []
        accepted   = []

        for inst in instances:
            ir = [r for r in rows if r["instance_id"] == inst]
            triggers.append(sum(as_int(r["llm_trigger"]) for r in ir))
            calls.append(sum(as_int(r["llm_calls"]) for r in ir))
            candidates.append(sum(as_int(r["llm_candidates"]) for r in ir))
            accepted.append(sum(as_int(r["llm_accepted"]) for r in ir))

        x = np.arange(len(instances))
        width = 0.2

        fig, ax = plt.subplots(figsize=(13, 4.5))
        ax.bar(x - 1.5 * width, triggers,   width, label="Wyzwolenia",   color="#4C72B0", zorder=3)
        ax.bar(x - 0.5 * width, calls,      width, label="Wywołania API", color="#55A868", zorder=3)
        ax.bar(x + 0.5 * width, candidates, width, label="Kandydaci",     color="#DD8452", zorder=3)
        ax.bar(x + 1.5 * width, accepted,   width, label="Zaakceptowani", color="#C44E52", zorder=3)

        ax.set_xticks(x)
        ax.set_xticklabels(instances, rotation=35, ha="right", fontsize=9)
        ax.set_ylabel("Liczba (suma po seedach × wariantach)")
        ax.set_title(f"EXP-L1 (DeepSeek-R1) {BUDGET_LABELS[budget]}: "
                     f"lejek pipeline LLM per instancja\n"
                     f"(łącznie: wyzwolenia={sum(triggers)}, kandydaci={sum(candidates)}, "
                     f"zaakceptowani={sum(accepted)})")
        ax.legend()
        ax.grid(axis="y", alpha=0.4, zorder=0)
        fig.tight_layout()

        fname = out / f"llm_funnel_{budget}it.png"
        fig.savefig(fname)
        plt.close(fig)
        print(f"  → {fname}")


# ── wykres 4: porównanie wariantów a/b/rag ──────────────────────────────────

def plot_variant_check(datasets: dict[str, list[dict]], out: Path) -> None:
    """Scatter: infeasibility_after dla wariantu a vs b vs rag (sprawdzenie identyczności)."""
    # Użyj największego budżetu z dostępnych
    budget = max(datasets.keys(), key=int)
    rows   = datasets[budget]
    variants = sorted(set(r["llm_variant"] for r in rows))

    if len(variants) < 2:
        return

    instances = [i for i in INSTANCES_ORDER if any(r["instance_id"] == i for r in rows)]
    n_inst = len(instances)
    n_var  = len(variants)
    width  = 0.8 / n_var
    x      = np.arange(n_inst)

    fig, ax = plt.subplots(figsize=(13, 4.5))

    for vi, var in enumerate(variants):
        feasible_rate = []
        for inst in instances:
            ir = [r for r in rows if r["instance_id"] == inst and r["llm_variant"] == var]
            rate = sum(1 for r in ir if as_int(r["infeasibility_after"]) == 0) / len(ir) * 100 if ir else 0.0
            feasible_rate.append(rate)
        offset = (vi - (n_var - 1) / 2) * width
        ax.bar(x + offset, feasible_rate, width * 0.9,
               color=VARIANT_COLORS.get(var, "#888888"),
               label=VARIANT_LABELS.get(var, var),
               alpha=0.85, zorder=3)

    ax.set_xticks(x)
    ax.set_xticklabels(instances, rotation=35, ha="right", fontsize=9)
    ax.set_ylabel("Plany wykonalne [%]")
    ax.set_ylim(0, 115)
    ax.set_title(f"EXP-L1 {BUDGET_LABELS[budget]}: porównanie wariantów promptu a / b / rag\n"
                 f"(identyczne wyniki potwierdzają brak wpływu LLM — 0 heurystyk zaakceptowanych)")
    ax.legend()
    ax.grid(axis="y", alpha=0.4, zorder=0)
    fig.tight_layout()
    fig.savefig(out / "variant_check.png")
    plt.close(fig)
    print(f"  → {out / 'variant_check.png'}")


# ── statystyki tekstowe ──────────────────────────────────────────────────────

def print_stats(datasets: dict[str, list[dict]]) -> None:
    print("\n========== STATYSTYKI EXP-L1 (DeepSeek-R1) ==========")
    for budget in sorted(datasets.keys(), key=int):
        rows = datasets[budget]
        variants = sorted(set(r["llm_variant"] for r in rows))
        n_runs = len(rows)
        feasible = sum(1 for r in rows if as_int(r["infeasibility_after"]) == 0)

        llm_calls  = sum(as_int(r["llm_calls"]) for r in rows)
        llm_cand   = sum(as_int(r["llm_candidates"]) for r in rows)
        llm_acc    = sum(as_int(r["llm_accepted"]) for r in rows)
        llm_t_mean = np.mean([as_float(r["llm_s"]) for r in rows if as_int(r["llm_calls"]) > 0]) if any(as_int(r["llm_calls"]) > 0 for r in rows) else 0.0
        elapsed_mean = np.mean([as_float(r["elapsed_s"]) for r in rows])

        print(f"\n--- Budżet: {int(budget):,} iteracji ---")
        print(f"  Przebiegi:       {n_runs}  ({n_runs // len(variants)} per wariant × {len(variants)} wariantów)")
        print(f"  Wykonalne:       {feasible}/{n_runs} ({100*feasible/n_runs:.1f}%)")
        print(f"  LLM wywołania:   {llm_calls}  (kandydaci: {llm_cand}, zaakceptowani: {llm_acc})")
        if llm_cand > 0:
            print(f"  Acceptance rate: {100*llm_acc/llm_cand:.1f}%")
        print(f"  Śr. czas łącznie:{elapsed_mean/60:.1f} min  (z czego LLM śr.: {llm_t_mean/60:.1f} min = {100*llm_t_mean/(elapsed_mean+1e-9):.0f}%)")

        print(f"\n  Wykonalne per instancja ({BUDGET_LABELS[budget]}):")
        instances = [i for i in INSTANCES_ORDER if any(r["instance_id"] == i for r in rows)]
        for inst in instances:
            ir = [r for r in rows if r["instance_id"] == inst]
            n_feas = sum(1 for r in ir if as_int(r["infeasibility_after"]) == 0)
            n_tot  = len(ir)
            obj_feas = [as_int(r["objective_after"]) for r in ir if as_int(r["infeasibility_after"]) == 0]
            obj_str = f"  obj med={int(np.median(obj_feas))}" if obj_feas else ""
            print(f"    {inst:<14} {n_feas:3d}/{n_tot}  ({100*n_feas/n_tot:5.1f}%){obj_str}")

    # Test Wilcoxona: porównanie wariantów (pary a-b, a-rag, b-rag) na 100k
    budget_100k = "100000"
    if budget_100k in datasets:
        rows = datasets[budget_100k]
        variants = sorted(set(r["llm_variant"] for r in rows))
        print(f"\n--- Test Wilcoxona (rank-sum) dla {BUDGET_LABELS[budget_100k]}: infeasibility_after ---")
        pairs = [(variants[i], variants[j]) for i in range(len(variants)) for j in range(i+1, len(variants))]
        for v1, v2 in pairs:
            x1 = [as_int(r["infeasibility_after"]) for r in rows if r["llm_variant"] == v1]
            x2 = [as_int(r["infeasibility_after"]) for r in rows if r["llm_variant"] == v2]
            stat, p = stats.ranksums(x1, x2)
            print(f"  {v1} vs {v2}: statistic={stat:.4f}, p={p:.4f}  {'(brak różnicy)' if p > 0.05 else '(*)' }")

        print(f"\n--- Test Wilcoxona: objective_after (tylko wykonalne przebiegi) ---")
        for v1, v2 in pairs:
            x1 = [as_int(r["objective_after"]) for r in rows
                  if r["llm_variant"] == v1 and as_int(r["infeasibility_after"]) == 0]
            x2 = [as_int(r["objective_after"]) for r in rows
                  if r["llm_variant"] == v2 and as_int(r["infeasibility_after"]) == 0]
            if len(x1) < 5 or len(x2) < 5:
                print(f"  {v1} vs {v2}: za mało danych ({len(x1)} / {len(x2)})")
                continue
            stat, p = stats.ranksums(x1, x2)
            print(f"  {v1} vs {v2}: statistic={stat:.4f}, p={p:.4f}  {'(brak różnicy)' if p > 0.05 else '(*)' }")

    print("\n=====================================================\n")


# ── main ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data-dir", default="data/results",
                        help="Katalog z plikami exp_l1_r1_*.csv")
    parser.add_argument("--out", default="data/results/figures/exp_l1",
                        help="Katalog wyjściowy dla wykresów")
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    out_dir  = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    # wczytaj wszystkie dostępne budżety
    csv_files = {
        "10000":  data_dir / "exp_l1_r1_10000it.csv",
        "100000": data_dir / "exp_l1_r1_100000it.csv",
        "300000": data_dir / "exp_l1_r1_300000it.csv",
    }
    datasets: dict[str, list[dict]] = {}
    for budget, path in csv_files.items():
        if path.exists():
            datasets[budget] = load_csv(path)
            print(f"Wczytano: {path.name}  ({len(datasets[budget])} wierszy)")
        else:
            print(f"Pominięto (brak): {path.name}")

    if not datasets:
        print("BŁĄD: Brak plików exp_l1_r1_*.csv w", data_dir)
        return

    print_stats(datasets)
    print("Generuję wykresy...")
    plot_feasibility(datasets, out_dir)
    plot_llm_overhead(datasets, out_dir)
    plot_llm_funnel(datasets, out_dir)
    plot_variant_check(datasets, out_dir)
    print(f"\nGotowe. Wykresy w: {out_dir}/")


if __name__ == "__main__":
    main()
