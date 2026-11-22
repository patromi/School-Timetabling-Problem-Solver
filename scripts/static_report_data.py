"""Dane dla raportu eksperymentów statycznych (report_static.py).

Dwa źródła:
  * CSV z experiments.py -- jeden wiersz na zakończony przebieg (koszt
    przed/po, liczba iteracji); ładowane do listy `Run`.
  * logi JSONL -- z każdego pliku wyciągamy tylko to, czego potrzebują
    wykresy (`RunLog`, zwykły słownik), równolegle w wielu procesach. Pełne
    logi mają kilka GB, więc wyciąg jest zapisywany do pliku cache
    (`<out-dir>/cache/<faza>.json.gz`) i przy kolejnych uruchomieniach
    czytany z niego.

Przebiegi bez rekordu `run_summary` (przerwane np. limitem czasu SLURM) są
pomijane -- tak samo jak w CSV, do którego trafiają tylko zakończone.
"""

import csv
import gzip
import json
from bisect import bisect_right
from collections import defaultdict
from collections.abc import Iterable
from dataclasses import dataclass
from multiprocessing import Pool
from pathlib import Path
from typing import Any

from scipy.stats import wilcoxon

# kolejność instancji z scripts/slurm_exp_matrix_static.sh
INSTANCE_ORDER = [
    "BR-SA-00",
    "BR-SM-00",
    "BR-SN-00",
    "FI-WP-06",
    "FI-MP-06",
    "FI-PB-98",
    "GR-P3-10",
    "GR-PA-08",
    "GR-H1-97",
    "ZA-LW-09",
    "ZA-WD-09",
    "IT-I4-96",
    "KS-PR-11",
]

# waga twardego kosztu przy porównaniu leksykograficznym jedną liczbą;
# większa niż jakikolwiek koszt miękki w tych instancjach
_LEX_WEIGHT = 10**9


@dataclass(frozen=True)
class Run:
    instance: str
    selector: str
    acceptance: str
    seed: int
    inf_before: int
    obj_before: int
    inf: int
    obj: int
    iterations: int

    @property
    def feasible(self) -> bool:
        return self.inf == 0

    @property
    def lex(self) -> int:
        """Koszt leksykograficzny (najpierw twardy, potem miękki) jako liczba."""
        return self.inf * _LEX_WEIGHT + self.obj


def order_instances(instances: Iterable[str]) -> list[str]:
    present = set(instances)
    first = [i for i in INSTANCE_ORDER if i in present]
    return first + sorted(present - set(first))


def load_runs(path: Path) -> list[Run]:
    with path.open(encoding="utf-8") as f:
        return [
            Run(
                instance=row["instance_id"],
                selector=row["selector"],
                acceptance=row["acceptance"],
                seed=int(row["seed"]),
                inf_before=int(row["infeasibility_before"]),
                obj_before=int(row["objective_before"]),
                inf=int(row["infeasibility_after"]),
                obj=int(row["objective_after"]),
                iterations=int(row["iterations_done"]),
            )
            for row in csv.DictReader(f)
        ]


def by_instance_selector(runs: Iterable[Run]) -> dict[tuple[str, str], list[Run]]:
    groups: dict[tuple[str, str], list[Run]] = defaultdict(list)
    for r in runs:
        groups[(r.instance, r.selector)].append(r)
    return groups


def feasible_pct(runs: list[Run]) -> float:
    return 100.0 * sum(r.feasible for r in runs) / len(runs) if runs else float("nan")


def best_known_objective(runs: Iterable[Run]) -> dict[str, int]:
    """Najniższy koszt miękki planu wykonalnego per instancja (ze wszystkich
    podanych przebiegów) -- punkt odniesienia do normalizacji kosztów."""
    best: dict[str, int] = {}
    for r in runs:
        if r.feasible and (r.instance not in best or r.obj < best[r.instance]):
            best[r.instance] = r.obj
    return best


def gap_pct(run: Run, best: dict[str, int]) -> float | None:
    """Odstęp kosztu miękkiego od najlepszego znanego, w procentach; tylko
    dla planów wykonalnych."""
    if not run.feasible or run.instance not in best:
        return None
    ref = best[run.instance]
    return 100.0 * (run.obj - ref) / ref if ref > 0 else float(run.obj - ref)


def pair_by_seed(
    a: Iterable[Run], b: Iterable[Run]
) -> dict[str, list[tuple[Run, Run]]]:
    """Pary przebiegów (A, B) z tej samej instancji i tego samego seeda."""
    index = {(r.instance, r.seed): r for r in b}
    pairs: dict[str, list[tuple[Run, Run]]] = defaultdict(list)
    for r in a:
        other = index.get((r.instance, r.seed))
        if other is not None:
            pairs[r.instance].append((r, other))
    return pairs


def wilcoxon_p(pairs: list[tuple[Run, Run]]) -> float:
    """Dwustronny test Wilcoxona par na koszcie leksykograficznym; 1.0, gdy
    wszystkie pary są remisami."""
    diffs = [a.lex - b.lex for a, b in pairs]
    if not any(diffs):
        return 1.0
    return float(wilcoxon(diffs, zero_method="wilcox").pvalue)


# ── logi JSONL ─────────────────────────────────────────────────────────────

RunLog = dict[str, Any]


def extract_log(path: Path) -> RunLog | None:
    """Wyciąg z jednego logu: tylko pola potrzebne wykresom."""
    config: dict[str, Any] = {}
    summary: dict[str, Any] | None = None
    bests: list[list[int]] = []
    perturbations: list[list[Any]] = []
    outcomes: list[list[Any]] = []
    windows: list[list[Any]] = []
    snapshots: list[list[Any]] = []
    final_states: dict[str, Any] | None = None
    with path.open(encoding="utf-8") as f:
        for line in f:
            r = json.loads(line)
            event = r["event"]
            if event == "new_best":
                bests.append(
                    [int(r["step"]), int(r["infeasibility"]), int(r["objective"])]
                )
            elif event == "selector_window":
                windows.append(
                    [
                        int(r["window_start"]),
                        int(r["window_end"]),
                        {
                            h: [int(s["calls"]), int(s.get("accepted_better", 0))]
                            for h, s in r["heuristics"].items()
                        },
                    ]
                )
            elif event == "selector_state":
                snapshots.append(
                    [
                        int(r["step"]),
                        int(r["current_cost"]),
                        int(r["best_cost"]),
                        r.get("temperature"),
                        list(r["state"]),
                        int(r["stagnant_iterations"]),
                    ]
                )
                states = r.get("selector", {}).get("states")
                if states:
                    final_states = {
                        key: {
                            "N": int(s["N"]),
                            "n": {h: int(v["n"]) for h, v in s["heuristics"].items()},
                        }
                        for key, s in states.items()
                    }
            elif event == "perturbation":
                perturbations.append([int(r["step"]), str(r["heuristic_id"])])
            elif event == "perturbation_outcome":
                outcomes.append(
                    [
                        str(r["heuristic_id"]),
                        bool(r["recovered"]),
                        r["steps_to_recover"],
                        bool(r["new_best"]),
                    ]
                )
            elif event == "run_config":
                config = r
            elif event == "run_summary":
                summary = r
    if summary is None:
        return None
    return {
        "instance": summary["instance_id"],
        "selector": summary["selector"],
        "acceptance": summary["acceptance"],
        "seed": int(summary["seed"]),
        "max_iterations": int(
            config.get("max_iterations") or summary["iterations_done"]
        ),
        "iterations_done": int(summary["iterations_done"]),
        "idle_limit": int(config.get("idle_limit", 0)),
        "perturbations_enabled": bool(config.get("fired_on_stagnation")),
        "thresholds": list(config.get("stagnation_thresholds", [])),
        "inf_before": int(summary["infeasibility_before"]),
        "obj_before": int(summary["objective_before"]),
        "inf_after": int(summary["infeasibility_after"]),
        "obj_after": int(summary["objective_after"]),
        "bests": bests,
        "perturbations": perturbations,
        "outcomes": outcomes,
        "windows": windows,
        "snapshots": snapshots,
        "final_states": final_states,
    }


def load_logs(
    log_dir: Path, cache: Path, workers: int, refresh: bool = False
) -> list[RunLog]:
    """Wyciągi ze wszystkich logów fazy; z cache, jeśli istnieje."""
    if cache.exists() and not refresh:
        with gzip.open(cache, "rt", encoding="utf-8") as f:
            data: list[RunLog] = json.load(f)
        return data
    paths = sorted(log_dir.glob("*.jsonl"))
    print(f"  wczytywanie {len(paths)} logów z {log_dir} ...", flush=True)
    with Pool(workers) as pool:
        extracted = pool.map(extract_log, paths, chunksize=8)
    logs = [x for x in extracted if x is not None]
    skipped = len(paths) - len(logs)
    if skipped:
        print(f"  pominięto {skipped} niezakończonych przebiegów (brak run_summary)")
    cache.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(cache, "wt", encoding="utf-8") as f:
        json.dump(logs, f)
    return logs


def best_inf_at(log: RunLog, step: int) -> int:
    """Najlepszy koszt twardy po kroku `step` (funkcja schodkowa z new_best)."""
    value = int(log["inf_before"])
    for s, inf, _obj in log["bests"]:
        if s > step:
            break
        value = inf
    return value


def stagnation_resets(log: RunLog) -> list[int]:
    """Kroki, w których solver zeruje licznik stagnacji (src/lahc.py): każdy
    nowy najlepszy wynik (min_relative_improvement = 0), każda perturbacja
    i każda nieudana perturbacja. Nieudana (ValueError) też zeruje licznik,
    ale nie trafia do logu -- wstawiamy ją tam, gdzie solver musiał ją
    wywołać. Perturbacja startuje na początku kroku last + idle_limit + 2
    (licznik po poprzednim kroku przekracza idle_limit). Udana zeruje
    licznik i kończy krok (reset w tym kroku); nieudana zeruje go, ale
    krok wykonuje zwykły ruch i licznik rośnie do 1 -- to tak, jakby reset
    był krok wcześniej, w last + idle_limit + 1."""
    logged = sorted(
        {s for s, _, _ in log["bests"]} | {s for s, _ in log["perturbations"]}
    )
    if not log.get("perturbations_enabled", True):
        return logged
    trigger = log["idle_limit"] + 2
    resets: list[int] = []
    last = -1  # licznik startuje od 0 przed krokiem 0, jak po resecie w kroku -1
    for s in [*logged, log["iterations_done"]]:
        while s > last + trigger:
            last += trigger - 1
            resets.append(last)
        if s < log["iterations_done"]:
            resets.append(s)
            last = s
    return resets


def stagnation_after(resets: list[int], step: int) -> int:
    """Wartość licznika stagnacji po kroku `step` (to, co solver loguje w
    selector_state jako stagnant_iterations)."""
    i = bisect_right(resets, step)
    return step - (resets[i - 1] if i else -1)
