#!/usr/bin/env python
"""Które heurystyki UCB wybiera, gdy plan jest niewykonalny, i czy to one
faktycznie redukują koszt twardy. Punktem odniesienia jest selektor random,
który wywołuje heurystyki po równo, więc pokazuje ich skuteczność bez
wpływu selekcji.

Liczymy tylko to, co logi rozstrzygają jednoznacznie:
  * okno selector_window należy do fazy niewykonalnej, gdy na jego końcu
    best_inf > 0 (do tego momentu żaden plan nie był wykonalny),
  * redukcja kosztu twardego to rekord new_best z mniejszym infeasibility;
    log podaje heurystykę, która ją dała. Poprawa rozwiązania bieżącego bez
    nowego najlepszego nie jest rozbita na twardą i miękką.
"Późna faza" to kroki od 20% budżetu, gdy łatwe poprawy startowe są już za nami.

Użycie:
    uv run python scripts/analyze_ucb_states.py --log-dir data/logs_static_v2/budzet_100000it
    uv run python scripts/analyze_ucb_states.py --log-dir <katalog> \\
        --instances BR-SN-00 ZA-WD-09 --out wynik.csv
"""

import argparse
import csv
import json
import os
from collections import defaultdict
from multiprocessing import Pool
from pathlib import Path
from typing import Any

from scipy.stats import spearmanr

HARD_WEIGHT = 1_000_000  # src.lahc: koszt = infeasibility * waga + objective
LATE_FRACTION = 0.2
SELECTORS = ("ucb", "random")
DEFAULT_INSTANCES = ["BR-SM-00", "BR-SN-00", "FI-PB-98", "ZA-WD-09"]
METRICS = ("calls", "better", "late_calls", "late_better", "hard", "late_hard", "soft_bests")

Stats = dict[str, dict[str, int]]


def extract(path: Path) -> dict[str, Any] | None:
    """Liczniki per heurystyka z jednego logu; None dla przerwanych."""
    stats: Stats = defaultdict(lambda: dict.fromkeys(METRICS, 0))
    budget = 0
    summary = None
    with path.open(encoding="utf-8") as f:
        for line in f:
            e = json.loads(line)
            kind = e["event"]
            if kind == "run_config":
                budget = int(e["max_iterations"])
            elif kind == "run_summary":
                summary = e
            elif kind == "new_best":
                old_inf = int(e["old_cost"]) // HARD_WEIGHT
                new_inf = int(e["infeasibility"])
                if old_inf == 0:
                    continue
                s = stats[e["heuristic_id"]]
                if new_inf < old_inf:
                    s["hard"] += old_inf - new_inf
                    if e["step"] >= budget * LATE_FRACTION:
                        s["late_hard"] += old_inf - new_inf
                else:
                    s["soft_bests"] += 1
            elif kind == "selector_window" and e.get("best_inf", 0) > 0:
                late = e["window_start"] >= budget * LATE_FRACTION
                for h, w in e["heuristics"].items():
                    s = stats[h]
                    s["calls"] += int(w["calls"])
                    s["better"] += int(w.get("accepted_better", 0))
                    if late:
                        s["late_calls"] += int(w["calls"])
                        s["late_better"] += int(w.get("accepted_better", 0))
    if summary is None:
        return None
    return {
        "instance": summary["instance_id"],
        "selector": summary["selector"],
        "stats": {h: dict(v) for h, v in stats.items()},
    }


def per_1000(value: int, calls: int) -> float:
    return 1000 * value / calls if calls else 0.0


def table(runs: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Wiersze per heurystyka dla jednej grupy przebiegów."""
    total: dict[str, Stats] = {s: defaultdict(lambda: dict.fromkeys(METRICS, 0)) for s in SELECTORS}
    for run in runs:
        for h, values in run["stats"].items():
            for k, v in values.items():
                total[run["selector"]][h][k] += v
    heuristics = sorted(set(total["ucb"]) | set(total["random"]))
    sums = {
        s: {k: sum(total[s][h][k] for h in heuristics) for k in METRICS} for s in SELECTORS
    }
    rows = []
    for h in heuristics:
        u, r = total["ucb"][h], total["random"][h]
        if not (u["calls"] or r["calls"]):
            continue  # perturbacje: wywołuje je solver, nie selektor
        rows.append(
            {
                "heuristic": h,
                "ucb_share_late": 100 * u["late_calls"] / max(1, sums["ucb"]["late_calls"]),
                "random_share_late": 100 * r["late_calls"] / max(1, sums["random"]["late_calls"]),
                "ucb_better_per_1k": per_1000(u["late_better"], u["late_calls"]),
                "random_better_per_1k": per_1000(r["late_better"], r["late_calls"]),
                "ucb_hard_per_1k": per_1000(u["late_hard"], u["late_calls"]),
                "random_hard_per_1k": per_1000(r["late_hard"], r["late_calls"]),
                "ucb_hard_share": 100 * u["late_hard"] / max(1, sums["ucb"]["late_hard"]),
                "random_hard_share": 100 * r["late_hard"] / max(1, sums["random"]["late_hard"]),
                "ucb_late_hard": u["late_hard"],
                "random_late_hard": r["late_hard"],
                "ucb_soft_bests": u["soft_bests"],
            }
        )
    rows.sort(key=lambda x: -x["ucb_share_late"])
    return rows


def show(name: str, runs: list[dict[str, Any]]) -> list[dict[str, Any]]:
    rows = table(runs)
    n = {s: sum(r["selector"] == s for r in runs) for s in SELECTORS}
    print(f"\n=== {name} (UCB: {n['ucb']} przebiegów, random: {n['random']})")
    print(
        f"{'heurystyka':23}{'udział UCB':>11}{'random':>8} | {'popr./1k UCB':>13}{'random':>8}"
        f" | {'twarde/1k UCB':>14}{'random':>8} | {'% redukcji UCB':>15}{'random':>8}"
    )
    for r in rows:
        print(
            f"{r['heuristic']:23}{r['ucb_share_late']:10.1f}%{r['random_share_late']:7.1f}% | "
            f"{r['ucb_better_per_1k']:13.1f}{r['random_better_per_1k']:8.1f} | "
            f"{r['ucb_hard_per_1k']:14.3f}{r['random_hard_per_1k']:8.3f} | "
            f"{r['ucb_hard_share']:14.1f}%{r['random_hard_share']:7.1f}%"
        )
    if len(rows) > 2:
        share = [r["ucb_share_late"] for r in rows]
        hard = spearmanr(share, [r["random_hard_per_1k"] for r in rows]).statistic
        better = spearmanr(share, [r["random_better_per_1k"] for r in rows]).statistic
        print(
            f"Spearman: udział UCB vs skuteczność w redukcji twardej (random) {hard:+.2f}, "
            f"vs częstość jakiejkolwiek poprawy (random) {better:+.2f}"
        )
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description="Wybory UCB w stanie niewykonalnym.")
    parser.add_argument("--log-dir", type=Path, required=True)
    parser.add_argument("--instances", nargs="+", default=DEFAULT_INSTANCES)
    parser.add_argument("--out", type=Path, default=None, help="CSV z tabelą")
    parser.add_argument("--workers", type=int, default=os.cpu_count() or 1)
    args = parser.parse_args()

    paths = [
        p
        for p in sorted(args.log_dir.glob("*.jsonl"))
        if p.name.split("_")[0] in args.instances and p.name.split("_")[1] in SELECTORS
    ]
    with Pool(args.workers) as pool:
        runs = [r for r in pool.map(extract, paths, chunksize=4) if r is not None]
    print(f"Wczytano {len(runs)} z {len(paths)} logów z {args.log_dir}")
    if not runs:
        raise SystemExit("Brak zakończonych przebiegów UCB i random.")

    output = [("razem", r) for r in show("Razem, późna faza niewykonalna", runs)]
    for instance in args.instances:
        group = [r for r in runs if r["instance"] == instance]
        if group:
            output += [(instance, r) for r in show(instance, group)]
    if args.out is not None:
        with args.out.open("w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["group", *output[0][1]])
            for group_name, row in output:
                writer.writerow(
                    [group_name]
                    + [round(v, 3) if isinstance(v, float) else v for v in row.values()]
                )


if __name__ == "__main__":
    main()
