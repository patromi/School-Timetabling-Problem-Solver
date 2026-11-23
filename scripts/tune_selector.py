#!/usr/bin/env python
"""Strojenie parametrów SA i UCB oraz uczciwy test względem selektora random.

Skrypt jest samodzielny: nie zmienia experiments.py ani src/. Wszystkie
przebiegi używają SA z chłodzeniem przez pozostały budżet (reheat="remaining"),
więc przebieg kończy się przy temperaturze t_end także po perturbacjach.

Etapy (każdy dopisuje wyniki do <out>/<etap>.csv i umie się wznowić):

  sa       t0_factor x idle_limit, osobno dla random i UCB    6 inst. x 10 seedów
  ucb      c x alfa x bonus x liczba stanów (SA z etapu sa)   6 inst. x 10 seedów
  ucb2     dogrywka 8 najlepszych na NOWYCH seedach           6 inst. x 20 seedów
           -> <out>/best_config.json
  test100  4 selektory, 100k iteracji                        13 inst. x 50 seedów
  test300  4 selektory, 300k iteracji                        13 inst. x 30 seedów
  report   werdykt względem random -> <out>/test_report.csv

Druga runda (--stages sa2 sa2final test2_100 test2_300 report2, domyślnie do
data/results/tuning_v2). Pierwsza wskazała t0 = 0,01 i idle = 6000 na granicy
siatki, więc rozszerzamy tylko SA; UCB zostaje przy c = 0,02, alfa = 0,02,
premii 0,1 i 6 stanach:

  sa2        t0 {0,01 0,02 0,05} x idle {6000 12000 25000 bez perturbacji},
             osobno dla random i UCB                         6 inst. x 10 seedów
  sa2final   dogrywka 3 najlepszych każdego selektora        6 inst. x 20 seedów
             -> <out>/best_config.json
  test2_100  4 selektory, 100k iteracji, seedy 300-349      13 inst. x 50 seedów
  test2_300  4 selektory, 300k iteracji, seedy 300-329      13 inst. x 30 seedów
  report2    werdykt względem random -> <out>/test_report.csv

Seedy 200-249 widzieliśmy już w pierwszej rundzie, więc test używa nowych.
Wariant bez perturbacji zachowuje progi stanów UCB z idle = 6000 (idle wyznacza
też te progi), żeby różnił się od niego tylko brakiem perturbacji.

Strojenie widzi tylko 6 instancji i seedy 100-129. Test używa seedów 200-249,
których strojenie nie widziało, oraz wszystkich 13 instancji. Random dostaje
własne najlepsze SA, żeby przewaga UCB nie wynikała z lepszej temperatury.
Epsilon-greedy nie jest strojony i używa SA selektora random.

Konfiguracje porównujemy średnią rangą kosztu leksykograficznego w blokach
(instancja, seed), bo koszty różnych instancji mają różną skalę.

Użycie:
    uv run python scripts/tune_selector.py --stages sa ucb ucb2 test100 test300 report
    uv run python scripts/tune_selector.py --stages sa2 sa2final test2_100 \
        test2_300 report2
    uv run python scripts/tune_selector.py --short --out <katalog>   # próba
"""

import argparse
import csv
import itertools
import json
import os
import random
import sys
import time
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import asdict, dataclass, replace
from pathlib import Path
from typing import Any

from scipy.stats import binomtest, rankdata, wilcoxon

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

ARCHIVE = ROOT / "data" / "xhstt2014" / "XHSTT-2014.xml"
STAGES = ["sa", "ucb", "ucb2", "test100", "test300", "report"]
STAGES_V2 = ["sa2", "sa2final", "test2_100", "test2_300", "report2"]

TUNE_INSTANCES = ["BR-SA-00", "FI-WP-06", "FI-MP-06", "GR-PA-08", "GR-H1-97", "ZA-LW-09"]
ALL_INSTANCES = [
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
TEST_SELECTORS = ["random", "epsilon-greedy", "ucb", "ucb-preprocessing"]
CONTENDERS = ["epsilon-greedy", "ucb", "ucb-preprocessing"]

# waga kosztu twardego: większa niż jakikolwiek koszt miękki w tych instancjach
LEX_WEIGHT = 10**9
LOG_WINDOW = 2000
ALPHA_TEST = 0.05

CSV_FIELDS = [
    "config_id",
    "selector",
    "t0",
    "idle",
    "c",
    "alpha",
    "bonus",
    "states",
    "perturb",
    "instance",
    "seed",
    "iterations",
    "inf_before",
    "obj_before",
    "inf",
    "obj",
    "elapsed_s",
]


@dataclass(frozen=True)
class Grid:
    """Przestrzeń strojenia i rozmiar eksperymentu (pełny albo próbny)."""

    tune_instances: tuple[str, ...]
    test_instances: tuple[str, ...]
    t0: tuple[float, ...]
    idle: tuple[int, ...]
    c: tuple[float, ...]
    alpha: tuple[float, ...]
    bonus: tuple[float, ...]
    states: tuple[int, ...]
    top_k: int
    tune_iterations: int
    test_budgets: dict[str, int]
    seeds_tune: tuple[int, ...]
    seeds_final: tuple[int, ...]
    seeds_test: dict[str, tuple[int, ...]]


FULL = Grid(
    tune_instances=tuple(TUNE_INSTANCES),
    test_instances=tuple(ALL_INSTANCES),
    t0=(1e-2, 1e-3, 1e-4),
    idle=(1500, 3000, 6000),
    c=(0.005, 0.02, 0.1, 0.3),
    alpha=(0.005, 0.02, 0.08),
    bonus=(0.1, 1.0),
    states=(2, 6),
    top_k=8,
    tune_iterations=100_000,
    test_budgets={"test100": 100_000, "test300": 300_000},
    seeds_tune=tuple(range(100, 110)),
    seeds_final=tuple(range(110, 130)),
    seeds_test={
        "test100": tuple(range(200, 250)),
        "test300": tuple(range(200, 230)),
    },
)
SHORT = Grid(
    tune_instances=("GR-PA-08", "ZA-LW-09"),
    test_instances=("GR-PA-08", "ZA-LW-09"),
    t0=(1e-2, 1e-3),
    idle=(60, 120),
    c=(0.02, 0.1),
    alpha=(0.02,),
    bonus=(0.1, 1.0),
    states=(2, 6),
    top_k=2,
    tune_iterations=300,
    test_budgets={"test100": 300, "test300": 600},
    seeds_tune=(100, 101),
    seeds_final=(110, 111),
    seeds_test={"test100": (200, 201), "test300": (200, 201)},
)

# wartość w siatce idle oznaczająca wariant bez perturbacji
NO_PERTURB = 0
FULL_V2 = replace(
    FULL,
    t0=(0.01, 0.02, 0.05),
    idle=(6000, 12000, 25000, NO_PERTURB),
    top_k=3,
    test_budgets={"test2_100": 100_000, "test2_300": 300_000},
    seeds_test={
        "test2_100": tuple(range(300, 350)),
        "test2_300": tuple(range(300, 330)),
    },
)
SHORT_V2 = replace(
    SHORT,
    t0=(0.01, 0.02),
    idle=(60, 120, NO_PERTURB),
    top_k=2,
    test_budgets={"test2_100": 300, "test2_300": 600},
    seeds_test={"test2_100": (300, 301), "test2_300": (300, 301)},
)


@dataclass(frozen=True)
class Config:
    """Jedna konfiguracja solvera."""

    selector: str
    t0: float
    idle: int
    c: float = 0.02
    alpha: float = 0.02
    bonus: float = 0.1
    states: int = 6
    perturb: bool = True

    @property
    def id(self) -> str:
        suffix = "" if self.perturb else "|bez_perturbacji"
        return (
            f"{self.selector}|t0={self.t0:g}|idle={self.idle}|c={self.c:g}"
            f"|a={self.alpha:g}|b={self.bonus:g}|s={self.states}{suffix}"
        )


Task = tuple[Config, str, int, int, str | None]
Row = dict[str, Any]

_instances: dict[str, Any] = {}


def _instance(instance_id: str) -> Any:
    """Instancja z archiwum; parsowana raz na proces roboczy."""
    if not _instances:
        from src.parser import parse_archive

        text = ARCHIVE.read_text(encoding="utf-8-sig")
        _instances.update({i.id: i for i in parse_archive(text)})
    return _instances[instance_id]


def _make_selector(cfg: Config, instance: Any, pool: list[Any]) -> tuple[Any, list[Any]]:
    from src.preprocessing import (
        analyze_instance,
        filter_heuristic_pool,
        get_initial_q_map,
    )
    from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector

    ucb_args = {
        "c": cfg.c,
        "alpha": cfg.alpha,
        "collapse_stagnation": cfg.states == 2,
    }
    if cfg.selector == "random":
        return RandomSelector(), pool
    if cfg.selector == "epsilon-greedy":
        return EpsilonGreedySelector(), pool
    if cfg.selector == "ucb":
        return UCBSelector(**ucb_args), pool
    if cfg.selector == "ucb-preprocessing":
        profile = analyze_instance(instance)
        selector = UCBSelector(initial_q_map=get_initial_q_map(profile), **ucb_args)
        return selector, filter_heuristic_pool(profile, pool)
    raise ValueError(f"Nieznany selektor: {cfg.selector}")


def run_task(task: Task) -> Row:
    """Jeden przebieg solvera w procesie roboczym."""
    import src.lahc as lahc_module
    from src.acceptance import AutoSAAcceptance
    from src.construct import build_greedy
    from src.evaluator_ref import evaluate_cost_components
    from src.heuristics import MANUAL_HEURISTICS
    from src.solve_log import SolveLogger

    cfg, instance_id, seed, iterations, log_path = task
    instance = _instance(instance_id)
    # premia za nowe najlepsze jest stałą modułu; proces wykonuje jeden
    # przebieg naraz, więc ustawienie jej tutaj nie wpływa na inne przebiegi
    lahc_module._BEST_BONUS = cfg.bonus

    rng = random.Random(seed)
    initial = build_greedy(instance, rng)
    inf0, obj0 = evaluate_cost_components(instance, initial)
    selector, pool = _make_selector(cfg, instance, list(MANUAL_HEURISTICS))
    acceptance = AutoSAAcceptance(
        t0_factor=cfg.t0, iteration_budget=iterations, reheat="remaining"
    )

    log_file = None
    logger = None
    if log_path is not None:
        Path(log_path).parent.mkdir(parents=True, exist_ok=True)
        log_file = Path(log_path).open("w", encoding="utf-8")
        logger = SolveLogger(log_file, window_size=LOG_WINDOW)
    started = time.monotonic()
    try:
        best, _ = lahc_module.run_lahc(
            instance,
            initial,
            rng,
            max_iterations=iterations,
            selector=selector,
            acceptance=acceptance,
            evaluation="incremental",
            progress_every=LOG_WINDOW,
            logger=logger,
            idle_limit=cfg.idle,
            min_relative_improvement=0.0,
            heuristics=pool,
            perturbations=cfg.perturb,
            seed=seed,
        )
    finally:
        if logger is not None:
            logger.flush()
        if log_file is not None:
            log_file.close()
    elapsed = time.monotonic() - started
    inf1, obj1 = evaluate_cost_components(instance, best)

    if log_path is not None:
        summary = {
            "event": "run_summary",
            "instance_id": instance_id,
            "selector": cfg.selector,
            "acceptance": "sa",
            "acceptance_detail": acceptance.name,
            "config_id": cfg.id,
            "seed": seed,
            "infeasibility_before": inf0,
            "objective_before": obj0,
            "infeasibility_after": inf1,
            "objective_after": obj1,
            "iterations_done": iterations,
            "elapsed_s": round(elapsed, 1),
        }
        with Path(log_path).open("a", encoding="utf-8") as f:
            f.write(json.dumps(summary, separators=(",", ":")) + "\n")

    return {
        "config_id": cfg.id,
        **asdict(cfg),
        "instance": instance_id,
        "seed": seed,
        "iterations": iterations,
        "inf_before": inf0,
        "obj_before": obj0,
        "inf": inf1,
        "obj": obj1,
        "elapsed_s": round(elapsed, 1),
    }


# ── wyniki ─────────────────────────────────────────────────────────────────


def read_rows(path: Path) -> list[Row]:
    if not path.exists():
        return []
    ints = ("idle", "states", "seed", "iterations", "inf_before", "obj_before", "inf", "obj")
    floats = ("t0", "c", "alpha", "bonus", "elapsed_s")
    with path.open(newline="", encoding="utf-8") as f:
        rows: list[Row] = list(csv.DictReader(f))
    for r in rows:
        r.update({k: int(r[k]) for k in ints})
        r.update({k: float(r[k]) for k in floats})
        # pliki sprzed drugiej rundy nie mają tej kolumny (zawsze z perturbacjami)
        r["perturb"] = r.get("perturb", "True") == "True"
    return rows


def lex(row: Row) -> int:
    return int(row["inf"]) * LEX_WEIGHT + int(row["obj"])


def config_of(row: Row) -> Config:
    return Config(
        selector=row["selector"],
        t0=row["t0"],
        idle=row["idle"],
        c=row["c"],
        alpha=row["alpha"],
        bonus=row["bonus"],
        states=row["states"],
        perturb=row["perturb"],
    )


def rank_blocks(rows: list[Row]) -> dict[str, list[float]]:
    """Rangi kosztu każdej konfiguracji w blokach (instancja, seed); tylko
    bloki z kompletem konfiguracji. Lista rang ma wspólną kolejność bloków."""
    configs = sorted({r["config_id"] for r in rows})
    blocks: dict[tuple[str, int], dict[str, int]] = defaultdict(dict)
    for r in rows:
        blocks[(r["instance"], r["seed"])][r["config_id"]] = lex(r)
    ranks: dict[str, list[float]] = {c: [] for c in configs}
    for key in sorted(blocks):
        block = blocks[key]
        if len(block) != len(configs):
            continue
        values = rankdata([block[c] for c in configs])
        for c, value in zip(configs, values, strict=True):
            ranks[c].append(float(value))
    return ranks


def ranking(rows: list[Row]) -> list[Row]:
    """Konfiguracje od najlepszej: średnia ranga, potem odsetek wykonalnych."""
    ranks = rank_blocks(rows)
    by_config: dict[str, list[Row]] = defaultdict(list)
    for r in rows:
        by_config[r["config_id"]].append(r)
    table = []
    for config_id, config_rows in by_config.items():
        feasible = sum(r["inf"] == 0 for r in config_rows)
        table.append(
            {
                "config_id": config_id,
                "mean_rank": round(sum(ranks[config_id]) / len(ranks[config_id]), 4),
                "feasible_pct": round(100 * feasible / len(config_rows), 2),
                "n": len(config_rows),
                "config": config_of(config_rows[0]),
            }
        )
    table.sort(key=lambda t: (t["mean_rank"], -t["feasible_pct"], t["config_id"]))
    return table


def write_ranking(path: Path, table: list[Row]) -> None:
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["config_id", "mean_rank", "feasible_pct", "n_runs"])
        for t in table:
            writer.writerow([t["config_id"], t["mean_rank"], t["feasible_pct"], t["n"]])


def complete_rows(out: Path, stage: str, tasks: list[Task]) -> list[Row]:
    """Wyniki etapu; przerywa, jeśli brakuje przebiegów, żeby nie wybierać
    konfiguracji na niepełnych danych."""
    wanted = {(t[0].id, t[1], t[2]) for t in tasks}
    rows = [
        r
        for r in read_rows(out / f"{stage}.csv")
        if (r["config_id"], r["instance"], r["seed"]) in wanted
    ]
    missing = len(wanted) - len({(r["config_id"], r["instance"], r["seed"]) for r in rows})
    if missing:
        raise SystemExit(
            f"Etap {stage}: brakuje {missing} z {len(wanted)} przebiegów. "
            f"Uruchom ponownie --stages {stage}."
        )
    return rows


# ── plan etapów ────────────────────────────────────────────────────────────


def tasks_for(configs: list[Config], instances: Any, seeds: Any, iters: int) -> list[Task]:
    return [(c, i, s, iters, None) for c in configs for i in instances for s in seeds]


def sa_tasks(grid: Grid) -> list[Task]:
    configs = [
        Config(selector=sel, t0=t0, idle=idle)
        for sel in ("random", "ucb")
        for t0, idle in itertools.product(grid.t0, grid.idle)
    ]
    return tasks_for(configs, grid.tune_instances, grid.seeds_tune, grid.tune_iterations)


def best_sa(out: Path, grid: Grid, selector: str) -> Config:
    rows = complete_rows(out, "sa", sa_tasks(grid))
    return ranking([r for r in rows if r["selector"] == selector])[0]["config"]


def ucb_tasks(out: Path, grid: Grid) -> list[Task]:
    base = best_sa(out, grid, "ucb")
    configs = [
        Config("ucb", base.t0, base.idle, c, alpha, bonus, states)
        for c, alpha, bonus, states in itertools.product(
            grid.c, grid.alpha, grid.bonus, grid.states
        )
    ]
    return tasks_for(configs, grid.tune_instances, grid.seeds_tune, grid.tune_iterations)


def ucb2_tasks(out: Path, grid: Grid) -> list[Task]:
    rows = complete_rows(out, "ucb", ucb_tasks(out, grid))
    top = [t["config"] for t in ranking(rows)[: grid.top_k]]
    return tasks_for(top, grid.tune_instances, grid.seeds_final, grid.tune_iterations)


def best_configs(out: Path, grid: Grid) -> dict[str, Config]:
    """Konfiguracje do testu; zwycięzca UCB wyłącznie z seedów dogrywki."""
    rows = complete_rows(out, "ucb2", ucb2_tasks(out, grid))
    ucb = ranking(rows)[0]["config"]
    rnd = best_sa(out, grid, "random")
    return {
        "random": rnd,
        "epsilon-greedy": Config("epsilon-greedy", rnd.t0, rnd.idle),
        "ucb": ucb,
        "ucb-preprocessing": Config(
            "ucb-preprocessing", ucb.t0, ucb.idle, ucb.c, ucb.alpha, ucb.bonus, ucb.states
        ),
    }


def sa2_tasks(grid: Grid) -> list[Task]:
    # bez perturbacji: progi stanów UCB jak przy najmniejszym idle z siatki
    state_idle = grid.idle[0]
    configs = [
        Config(selector=sel, t0=t0, idle=idle)
        if idle != NO_PERTURB
        else Config(selector=sel, t0=t0, idle=state_idle, perturb=False)
        for sel in ("random", "ucb")
        for t0, idle in itertools.product(grid.t0, grid.idle)
    ]
    return tasks_for(configs, grid.tune_instances, grid.seeds_tune, grid.tune_iterations)


def sa2final_tasks(out: Path, grid: Grid) -> list[Task]:
    rows = complete_rows(out, "sa2", sa2_tasks(grid))
    top = [
        t["config"]
        for sel in ("random", "ucb")
        for t in ranking([r for r in rows if r["selector"] == sel])[: grid.top_k]
    ]
    return tasks_for(top, grid.tune_instances, grid.seeds_final, grid.tune_iterations)


def best_configs_v2(out: Path, grid: Grid) -> dict[str, Config]:
    """Konfiguracje do testu drugiej rundy; zwycięzcy tylko z seedów dogrywki."""
    rows = complete_rows(out, "sa2final", sa2final_tasks(out, grid))
    rnd = ranking([r for r in rows if r["selector"] == "random"])[0]["config"]
    ucb = ranking([r for r in rows if r["selector"] == "ucb"])[0]["config"]
    return {
        "random": rnd,
        "epsilon-greedy": replace(rnd, selector="epsilon-greedy"),
        "ucb": ucb,
        "ucb-preprocessing": replace(ucb, selector="ucb-preprocessing"),
    }


def test_tasks(out: Path, grid: Grid, stage: str) -> list[Task]:
    configs = (
        best_configs_v2(out, grid) if stage in STAGES_V2 else best_configs(out, grid)
    )
    tasks = []
    for sel in TEST_SELECTORS:
        for instance, seed in itertools.product(
            grid.test_instances, grid.seeds_test[stage]
        ):
            log = out / "logs" / stage / f"{instance}_{sel}_seed{seed:03d}.jsonl"
            tasks.append((configs[sel], instance, seed, grid.test_budgets[stage], str(log)))
    return tasks


def stage_tasks(out: Path, grid: Grid, stage: str) -> list[Task]:
    if stage == "sa":
        return sa_tasks(grid)
    if stage == "ucb":
        return ucb_tasks(out, grid)
    if stage == "ucb2":
        return ucb2_tasks(out, grid)
    if stage == "sa2":
        return sa2_tasks(grid)
    if stage == "sa2final":
        return sa2final_tasks(out, grid)
    return test_tasks(out, grid, stage)


def run_stage(out: Path, grid: Grid, stage: str, workers: int) -> None:
    tasks = stage_tasks(out, grid, stage)
    path = out / f"{stage}.csv"
    done = {(r["config_id"], r["instance"], r["seed"]) for r in read_rows(path)}
    pending = [t for t in tasks if (t[0].id, t[1], t[2]) not in done]
    print(
        f"\n=== {stage}: {len(tasks)} przebiegów, zrobione {len(tasks) - len(pending)}, "
        f"do zrobienia {len(pending)} ===",
        flush=True,
    )
    failed = 0
    if pending:
        new_file = not path.exists()
        with path.open("a", newline="", encoding="utf-8") as f:
            # dopisując do starszego pliku trzymamy się jego nagłówka
            fields = CSV_FIELDS
            if not new_file:
                with path.open(newline="", encoding="utf-8") as existing:
                    fields = next(csv.reader(existing))
            writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
            if new_file:
                writer.writeheader()
            with ProcessPoolExecutor(max_workers=workers) as pool:
                futures = {pool.submit(run_task, t): t for t in pending}
                for n, future in enumerate(as_completed(futures), 1):
                    task = futures[future]
                    try:
                        writer.writerow(future.result())
                        f.flush()
                    except Exception as exc:  # jeden błąd nie przerywa etapu
                        failed += 1
                        print(f"BŁĄD {task[0].id} {task[1]} seed {task[2]}: {exc!r}")
                    if n % max(1, len(pending) // 20) == 0:
                        print(f"  {stage}: {n}/{len(pending)}", flush=True)
    if failed:
        raise SystemExit(f"Etap {stage}: {failed} przebiegów zakończonych błędem.")

    rows = complete_rows(out, stage, tasks)
    if stage == "sa":
        for sel in ("random", "ucb"):
            table = ranking([r for r in rows if r["selector"] == sel])
            write_ranking(out / f"ranking_sa_{sel}.csv", table)
            print(f"  najlepsze SA dla {sel}: {table[0]['config_id']}")
    elif stage in ("ucb", "ucb2"):
        table = ranking(rows)
        write_ranking(out / f"ranking_{stage}.csv", table)
        print(f"  najlepsza konfiguracja: {table[0]['config_id']}")
    elif stage in ("sa2", "sa2final"):
        for sel in ("random", "ucb"):
            table = ranking([r for r in rows if r["selector"] == sel])
            write_ranking(out / f"ranking_{stage}_{sel}.csv", table)
            print(f"  najlepsze SA dla {sel}: {table[0]['config_id']}")
    if stage in ("ucb2", "sa2final"):
        chosen = (
            best_configs(out, grid) if stage == "ucb2" else best_configs_v2(out, grid)
        )
        best = {k: asdict(v) for k, v in chosen.items()}
        (out / "best_config.json").write_text(
            json.dumps(best, indent=2), encoding="utf-8"
        )
        print(f"  zapisano {out / 'best_config.json'}")


# ── test końcowy ───────────────────────────────────────────────────────────


def holm(pvalues: list[float]) -> list[float]:
    """Skorygowane p metodą Holma."""
    order = sorted(range(len(pvalues)), key=lambda i: pvalues[i])
    adjusted = [1.0] * len(pvalues)
    running = 0.0
    for position, i in enumerate(order):
        running = max(running, min(1.0, (len(pvalues) - position) * pvalues[i]))
        adjusted[i] = running
    return adjusted


def mcnemar_p(a_only: int, b_only: int) -> float:
    """Dokładny test McNemara na przebiegach, gdzie wykonalny jest tylko jeden."""
    total = a_only + b_only
    return float(binomtest(a_only, total, 0.5).pvalue) if total else 1.0


def verdict(better: list[bool], worse: list[bool]) -> str:
    if any(worse) and not any(better):
        return "gorszy"
    if any(worse):
        return "niejednoznaczny"
    return "lepszy" if any(better) else "brak istotnej różnicy"


def report_stage(rows: list[Row], stage: str) -> list[Row]:
    """Porównania z random na blokach z kompletem czterech selektorów."""
    blocks: dict[tuple[str, int], dict[str, Row]] = defaultdict(dict)
    for r in rows:
        blocks[(r["instance"], r["seed"])][r["selector"]] = r
    full = [b for _, b in sorted(blocks.items()) if len(b) == len(TEST_SELECTORS)]
    if not full:
        return []
    ranks: dict[str, list[float]] = {s: [] for s in TEST_SELECTORS}
    for block in full:
        values = rankdata([lex(block[s]) for s in TEST_SELECTORS])
        for s, value in zip(TEST_SELECTORS, values, strict=True):
            ranks[s].append(float(value))

    raw: list[float] = []
    for sel in CONTENDERS:
        diffs = [a - b for a, b in zip(ranks[sel], ranks["random"], strict=True)]
        raw.append(float(wilcoxon(diffs).pvalue) if any(diffs) else 1.0)
        sel_only = sum(b[sel]["inf"] == 0 and b["random"]["inf"] != 0 for b in full)
        rnd_only = sum(b["random"]["inf"] == 0 and b[sel]["inf"] != 0 for b in full)
        raw.append(mcnemar_p(sel_only, rnd_only))
    adjusted = holm(raw)

    pct = {s: 100 * sum(b[s]["inf"] == 0 for b in full) / len(full) for s in TEST_SELECTORS}
    mean = {s: sum(ranks[s]) / len(ranks[s]) for s in TEST_SELECTORS}
    table = [
        {
            "stage": stage,
            "selector": "random",
            "n_blocks": len(full),
            "feasible_pct": round(pct["random"], 2),
            "mean_rank": round(mean["random"], 4),
            "p_cost_holm": "",
            "p_feasible_holm": "",
            "verdict": "punkt odniesienia",
        }
    ]
    for j, sel in enumerate(CONTENDERS):
        p_cost, p_feas = adjusted[2 * j], adjusted[2 * j + 1]
        better = [
            p_cost < ALPHA_TEST and mean[sel] < mean["random"],
            p_feas < ALPHA_TEST and pct[sel] > pct["random"],
        ]
        worse = [
            p_cost < ALPHA_TEST and mean[sel] > mean["random"],
            p_feas < ALPHA_TEST and pct[sel] < pct["random"],
        ]
        table.append(
            {
                "stage": stage,
                "selector": sel,
                "n_blocks": len(full),
                "feasible_pct": round(pct[sel], 2),
                "mean_rank": round(mean[sel], 4),
                "p_cost_holm": round(p_cost, 5),
                "p_feasible_holm": round(p_feas, 5),
                "verdict": verdict(better, worse),
            }
        )
    return table


def write_report(out: Path, grid: Grid) -> None:
    table: list[Row] = []
    per_instance = []
    for stage in grid.test_budgets:
        rows = read_rows(out / f"{stage}.csv")
        if not rows:
            print(f"  {stage}: brak wyników, pomijam")
            continue
        expected = len(TEST_SELECTORS) * len(grid.test_instances) * len(grid.seeds_test[stage])
        if len(rows) < expected:
            print(f"  UWAGA: {stage} niepełny ({len(rows)}/{expected} przebiegów)")
        table.extend(report_stage(rows, stage))
        groups: dict[tuple[str, str], list[Row]] = defaultdict(list)
        for r in rows:
            groups[(r["instance"], r["selector"])].append(r)
        for instance in grid.test_instances:
            for sel in TEST_SELECTORS:
                g = groups.get((instance, sel), [])
                if g:
                    k = sum(r["inf"] == 0 for r in g)
                    per_instance.append(
                        [stage, instance, sel, len(g), k, round(100 * k / len(g), 1)]
                    )
    if not table:
        raise SystemExit("Brak wyników testu do raportu.")
    with (out / "test_report.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(table[0]))
        writer.writeheader()
        writer.writerows(table)
    with (out / "test_per_instance.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["stage", "instance", "selector", "n_runs", "n_feasible", "feasible_pct"])
        writer.writerows(per_instance)
    print("\n=== Werdykt względem random (Holm, p < 0,05) ===")
    for t in table:
        print(
            f"{t['stage']:8} {t['selector']:18} wykonalne {t['feasible_pct']:6.2f}%  "
            f"ranga {t['mean_rank']:.3f}  p koszt {t['p_cost_holm']!s:8} "
            f"p wyk. {t['p_feasible_holm']!s:8} {t['verdict']}"
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Strojenie SA i UCB oraz test względem random."
    )
    parser.add_argument(
        "--stages", nargs="+", choices=STAGES + STAGES_V2, default=STAGES
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=None,
        help="domyślnie data/results/tuning, dla drugiej rundy tuning_v2",
    )
    parser.add_argument("--workers", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--short", action="store_true", help="krótka próba działania")
    args = parser.parse_args()
    second_round = set(args.stages) <= set(STAGES_V2)
    if not second_round and set(args.stages) & set(STAGES_V2):
        parser.error("etapów pierwszej i drugiej rundy nie można łączyć")
    if args.out is None:
        name = "tuning_v2" if second_round else "tuning"
        args.out = ROOT / "data" / "results" / name
    if second_round:
        grid = SHORT_V2 if args.short else FULL_V2
    else:
        grid = SHORT if args.short else FULL
    args.out.mkdir(parents=True, exist_ok=True)
    for stage in STAGES_V2 if second_round else STAGES:
        if stage not in args.stages:
            continue
        if stage in ("report", "report2"):
            write_report(args.out, grid)
        else:
            run_stage(args.out, grid, stage, args.workers)
    print(f"\nGotowe: {args.out}")


if __name__ == "__main__":
    main()
