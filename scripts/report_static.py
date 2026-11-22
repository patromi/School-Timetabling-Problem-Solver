#!/usr/bin/env python
"""Wszystkie wykresy do rozdziału eksperymentalnego z macierzy bez LLM
(scripts/slurm_exp_matrix_static.sh) jedną komendą.

Wejście: katalog z CSV faz (selektory, akceptacja_lahc, preset_bazowy,
ablacja_ucb, budzet_*it) i katalog z logami JSONL (po jednym podkatalogu
na fazę). Wyjście: <out-dir>/wykresy/png/ i <out-dir>/wykresy/pdf/,
<out-dir>/dane/ (CSV z danymi każdego wykresu), <out-dir>/cache/.

  fig01  odsetek planów wykonalnych (selektory)            CSV
  fig02  koszt miękki planów wykonalnych (selektory)       CSV
  fig03  zbieżność w funkcji iteracji (selektory)          logi
  fig06  udział heurystyk w trakcie przebiegu              logi
  fig10  SA vs LAHC                                        CSV
  fig12  preset bazowy vs konfiguracja domyślna            CSV
  fig13  wrażliwość UCB na c i alfa                        CSV
  fig15  UCB 6 stanów vs 2 stany                           CSV
  fig16  jakość w funkcji budżetu                          CSV
  fig17  UCB: wykonalność a liczba iteracji, SA i LAHC     CSV + logi
  fig18  cztery selektory: wykonalność a iteracje         CSV
  figS1  okresy bez poprawy (SA i LAHC)                    logi
  figS2  czas w stanach selektora UCB                      logi
  figS3  wysoka stagnacja w trakcie przebiegu              logi
  figS4  moment ostatniej poprawy vs budżet                logi (budzet_*)
  figS5  wybory UCB wg stanu                               logi
  figS6  skuteczność perturbacji                           logi
  figS7  anatomia jednego przebiegu                        logi

Pierwsze uruchomienie czyta logi (kilka GB) równolegle i zapisuje wyciąg do
<out-dir>/cache/; kolejne biorą wyciąg z cache (--refresh-cache wymusza
ponowne czytanie, np. po dokończeniu fazy).

Użycie:
    uv run python scripts/report_static.py
    uv run python scripts/report_static.py --only fig01 fig16 figS7 \
        --anatomy GR-PA-08 ucb 3
"""

import argparse
import os
import sys
from collections.abc import Callable
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import static_report_logs as L
import static_report_results as R
from static_report_data import RunLog, best_known_objective, load_logs, load_runs

FIGURES = [
    "fig01",
    "fig02",
    "fig03",
    "fig06",
    "fig10",
    "fig12",
    "fig13",
    "fig15",
    "fig16",
    "fig17",
    "fig18",
    "figS1",
    "figS2",
    "figS3",
    "figS4",
    "figS5",
    "figS6",
    "figS7",
]
BUDGETS = [10000, 100000, 300000]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Wykresy z eksperymentów statycznych (bez LLM)."
    )
    parser.add_argument(
        "--results-dir", type=Path, default=Path("data/results/static_v2")
    )
    parser.add_argument("--log-dir", type=Path, default=Path("data/logs_static_v2"))
    parser.add_argument(
        "--out-dir", type=Path, default=Path("data/results/static_v2/report")
    )
    parser.add_argument(
        "--only",
        nargs="+",
        choices=FIGURES,
        default=FIGURES,
        help="tylko wybrane wykresy",
    )
    parser.add_argument("--workers", type=int, default=os.cpu_count() or 1)
    parser.add_argument(
        "--refresh-cache", action="store_true", help="czytaj logi od nowa"
    )
    parser.add_argument(
        "--anatomy",
        nargs=3,
        metavar=("INSTANCJA", "SELEKTOR", "SEED"),
        default=["BR-SA-00", "ucb", "0"],
        help="przebieg do figS7",
    )
    args = parser.parse_args()
    # wykresy (PNG + PDF) w wykresy/, ich dane (CSV) w dane/, wyciąg logów w cache/
    out: Path = args.out_dir / "wykresy"
    wanted = set(args.only)

    def runs(phase: str) -> list[R.Run]:
        return load_runs(args.results_dir / f"{phase}.csv")

    log_cache: dict[str, list[RunLog]] = {}

    def logs(phase: str) -> list[RunLog]:
        if phase not in log_cache:
            log_cache[phase] = load_logs(
                args.log_dir / phase,
                args.out_dir / "cache" / f"{phase}.json.gz",
                args.workers,
                args.refresh_cache,
            )
        return log_cache[phase]

    def budget_runs() -> dict[int, list[R.Run]]:
        return {it: runs(f"budzet_{it}it") for it in BUDGETS}

    def best_known() -> dict[str, int]:
        phases = ["selektory", "akceptacja_lahc", "preset_bazowy", "ablacja_ucb"] + [
            f"budzet_{b}it" for b in BUDGETS
        ]
        return best_known_objective(r for p in phases for r in runs(p))

    def anatomy() -> None:
        inst, sel, seed = args.anatomy[0], args.anatomy[1], int(args.anatomy[2])
        match = [
            x
            for x in logs("selektory")
            if (x["instance"], x["selector"], x["seed"]) == (inst, sel, seed)
        ]
        if not match:
            print(
                f"  figS7: brak przebiegu {inst} {sel} seed {seed} w logach fazy selektory"
            )
            return
        L.figS7_anatomy(match[0], out / "figS7_anatomia_przebiegu")

    tasks: dict[str, Callable[[], None]] = {
        "fig01": lambda: R.fig01_feasible_heatmap(
            runs("selektory"), out / "fig01_wykonalnosc_selektory"
        ),
        "fig02": lambda: R.fig02_soft_boxes(
            runs("selektory"), out / "fig02_koszt_miekki_selektory"
        ),
        "fig03": lambda: L.fig03_convergence(
            logs("selektory"), out / "fig03_zbieznosc_iteracje"
        ),
        "fig06": lambda: L.fig06_heuristic_usage(
            logs("selektory"), out / "fig06_udzial_heurystyk"
        ),
        "fig10": lambda: R.fig10_sa_vs_lahc(
            runs("selektory"), runs("akceptacja_lahc"), out / "fig10_sa_vs_lahc"
        ),
        "fig12": lambda: R.fig12_preset(
            runs("selektory"), runs("preset_bazowy"), out / "fig12_preset_bazowy"
        ),
        "fig13": lambda: R.fig13_sensitivity(
            runs("selektory"), runs("ablacja_ucb"), out / "fig13_wrazliwosc_ucb"
        ),
        "fig15": lambda: R.fig15_two_states(
            runs("selektory"), runs("ablacja_ucb"), out / "fig15_ucb_2_stany"
        ),
        "fig16": lambda: R.fig16_budget(
            budget_runs(), best_known(), out / "fig16_budzet"
        ),
        "fig17": lambda: R.fig17_ucb_budget(
            budget_runs(),
            logs("akceptacja_lahc"),
            out / "fig17_ucb_iteracje_wykonalnosc",
        ),
        "fig18": lambda: R.fig18_selectors_budget(
            budget_runs(), out / "fig18_selektory_iteracje_wykonalnosc"
        ),
        "figS1": lambda: L.figS1_gaps(
            logs("selektory"), logs("akceptacja_lahc"), out / "figS1_okresy_bez_poprawy"
        ),
        "figS2": lambda: L.figS2_state_occupancy(
            logs("selektory"), out / "figS2_stany_selektora"
        ),
        "figS3": lambda: L.figS3_high_stagnation(
            logs("selektory"), out / "figS3_wysoka_stagnacja"
        ),
        "figS4": lambda: L.figS4_last_improvement(
            {it: logs(f"budzet_{it}it") for it in BUDGETS},
            out / "figS4_ostatnia_poprawa",
        ),
        "figS5": lambda: L.figS5_choices_by_state(
            logs("selektory"), out / "figS5_wybory_ucb_wg_stanu"
        ),
        "figS6": lambda: L.figS6_perturbations(
            logs("selektory"), out / "figS6_perturbacje"
        ),
        "figS7": anatomy,
    }
    for name in FIGURES:
        if name in wanted:
            print(f"{name} ...", flush=True)
            tasks[name]()
    print(f"Gotowe: wykresy w {out}/, dane w {args.out_dir / 'dane'}/")


if __name__ == "__main__":
    main()
