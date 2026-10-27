"""Comparative experiment runner for instance preprocessing and heuristic adaptation.

Runs benchmarks across multiple instances and seeds in parallel using
ProcessPoolExecutor, comparing baseline hyper-heuristic performance against
adaptive preprocessing (pruned heuristic pool, tightness-aware priors, solver
adaptation).

Tracks:
  - instance, mode (adaptive vs baseline), seed, iterations limit/done
  - infeasibility and objective before/after
  - exact iteration where hard constraints reach 0 (first_feasible_iteration)
  - wall-clock time to feasibility (first_feasible_seconds)
  - throughput (iterations/second) and total elapsed time

Usage:
    uv run python preprocessing_experiments.py
    uv run python preprocessing_experiments.py --iterations 500000 --workers 16
    uv run python preprocessing_experiments.py --instances BR-SA-00 GR-PA-08
    uv run python preprocessing_experiments.py --modes adaptive
"""

import argparse
import csv
import os
import random
import sys
import time
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from src.acceptance import AcceptanceCriterion
    from src.selectors import HeuristicSelector

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
DEFAULT_INSTANCES = ["BR-SA-00", "GR-PA-08", "GR-H1-97", "ZA-WD-09"]
DEFAULT_MODES = ["adaptive", "baseline"]
DEFAULT_SEEDS = list(range(10))
DEFAULT_ITERATIONS = 500_000
DEFAULT_CSV_PATH = Path("data/results/preprocessing_experiments.csv")
DEFAULT_LOG_DIR = Path("data/logs/preprocessing_experiments")
LOG_WINDOW_SIZE = 2000
STAGNATION_LIMIT = 3000
MIN_RELATIVE_IMPROVEMENT = 0.0


@dataclass(frozen=True)
class ExperimentResult:
    """Detailed result of a single experiment run."""

    instance_id: str
    mode: str
    seed: int
    iterations_limit: int
    iterations_done: int
    infeasibility_before: int
    objective_before: int
    infeasibility_after: int
    objective_after: int
    first_feasible_iteration: int | None
    first_feasible_seconds: float | None
    is_feasible: bool
    elapsed_s: float
    iterations_per_sec: float
    scale: str
    tightness: str
    selector: str
    acceptance: str
    acceptance_detail: str


def _make_selector(
    selector_name: str, initial_q_map: dict[str, float] | None = None
) -> "HeuristicSelector":
    from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector

    if selector_name == "epsilon-greedy":
        return EpsilonGreedySelector(initial_q_map=initial_q_map)
    if selector_name == "ucb":
        return UCBSelector(initial_q_map=initial_q_map)
    return RandomSelector()


def _make_acceptance(
    acceptance_name: str,
    time_limit: float | None,
    iterations: int,
) -> "AcceptanceCriterion":
    from src.acceptance import AutoSAAcceptance, LAHCAcceptance

    if acceptance_name == "sa":
        if time_limit is not None:
            return AutoSAAcceptance(time_budget=time_limit, reheat="remaining")
        return AutoSAAcceptance(iteration_budget=iterations, reheat="remaining")
    return LAHCAcceptance(history_length=30)


def _run_single_task(
    instance_id: str,
    mode: str,
    selector_name: str,
    acceptance_name: str,
    seed: int,
    iterations: int,
    time_limit: float | None,
    log_path: Path | None,
    archive_path: Path,
) -> ExperimentResult:
    """Run one solver task (instance, mode, seed) in a worker process."""
    from src.construct import build_greedy
    from src.evaluator_ref import evaluate_cost_components
    from src.lahc import RunStats, run_lahc
    from src.parser import parse_archive
    from src.preprocessing import (
        analyze_instance,
        get_recommended_solver_config,
    )
    from src.solve_log import SolveLogger

    archive_text = archive_path.read_text(encoding="utf-8-sig")
    instances = parse_archive(archive_text)
    instance = next(i for i in instances if i.id == instance_id)

    profile = analyze_instance(instance)
    scale = profile.scale
    tightness = profile.tightness_category

    rng = random.Random(seed)
    initial = build_greedy(instance, rng)
    inf0, obj0 = evaluate_cost_components(instance, initial)

    if mode == "adaptive":
        rec = get_recommended_solver_config(profile)
        heuristics_pool = rec["heuristics"]
        initial_q_map = rec["initial_q_map"]
        perturbations = rec["perturbations"]
        idle_limit = rec["idle_limit"]
    else:  # baseline
        heuristics_pool = None
        initial_q_map = None
        perturbations = True
        idle_limit = max(3000, 50 * len(instance.events))

    selector = _make_selector(selector_name, initial_q_map=initial_q_map)
    acceptance = _make_acceptance(
        acceptance_name, time_limit=time_limit, iterations=iterations
    )

    iters_done = [0]

    def _on_progress(it: int, _cost: int) -> None:
        iters_done[0] = it

    log_file = None
    logger = None
    if log_path is not None:
        log_file = log_path.open("w", encoding="utf-8")
        logger = SolveLogger(log_file, window_size=LOG_WINDOW_SIZE)

    stats = RunStats()
    t0 = time.monotonic()
    try:
        best, _ = run_lahc(
            instance,
            initial,
            rng,
            max_iterations=iterations,
            max_seconds=time_limit,
            heuristics=heuristics_pool,
            selector=selector,
            acceptance=acceptance,
            evaluation="incremental",
            on_progress=_on_progress,
            progress_every=LOG_WINDOW_SIZE,
            logger=logger,
            idle_limit=idle_limit,
            min_relative_improvement=MIN_RELATIVE_IMPROVEMENT,
            perturbations=perturbations,
            stats=stats,
        )
    finally:
        if logger is not None:
            logger.flush()
        if log_file is not None:
            log_file.close()

    elapsed = time.monotonic() - t0
    inf1, obj1 = evaluate_cost_components(instance, best)
    it_done = iters_done[0] if iters_done[0] > 0 else stats.steps_done
    it_per_sec = it_done / elapsed if elapsed > 0 else 0.0

    return ExperimentResult(
        instance_id=instance_id,
        mode=mode,
        seed=seed,
        iterations_limit=iterations,
        iterations_done=it_done,
        infeasibility_before=inf0,
        objective_before=obj0,
        infeasibility_after=inf1,
        objective_after=obj1,
        first_feasible_iteration=stats.first_feasible_step,
        first_feasible_seconds=stats.first_feasible_elapsed_s,
        is_feasible=inf1 == 0,
        elapsed_s=round(elapsed, 2),
        iterations_per_sec=round(it_per_sec, 1),
        scale=scale,
        tightness=tightness,
        selector=selector_name,
        acceptance=acceptance_name,
        acceptance_detail=acceptance.name,
    )


def _append_to_csv(csv_path: Path, result: ExperimentResult) -> None:
    """Atomically append a single result row to the CSV file."""
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    file_exists = csv_path.exists() and csv_path.stat().st_size > 0
    row_dict = asdict(result)

    with csv_path.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(row_dict.keys()))
        if not file_exists:
            writer.writeheader()
        writer.writerow(row_dict)
        f.flush()


def _print_run_log(idx: int, total: int, r: ExperimentResult) -> None:
    """Print informative single-line progress to console."""
    if r.is_feasible:
        sec_str = (
            f" ({r.first_feasible_seconds:.2f}s)"
            if r.first_feasible_seconds is not None
            else ""
        )
        status = f"WYKONALNY w kroku {r.first_feasible_iteration}{sec_str}"
    else:
        status = f"NIEWYKONALNY (twarde={r.infeasibility_after})"

    prefix = f"[{idx:>{len(str(total))}}/{total}] {r.instance_id:10} [{r.mode:8}]"
    hard_chg = f"hard {r.infeasibility_before:>3} -> {r.infeasibility_after:>2}"
    soft_chg = f"soft {r.objective_before:>4} -> {r.objective_after:>4}"
    perf = (
        f"{r.iterations_done:,} it w {r.elapsed_s:.1f}s "
        f"({r.iterations_per_sec:,.0f} it/s)"
    )
    print(f"{prefix} seed={r.seed:2d}: {hard_chg}, {soft_chg} | {perf} | {status}")


def _print_summary_table(results: list[ExperimentResult]) -> None:
    """Print comparative aggregation table grouped by instance and mode."""
    # (instance, mode) -> list of results
    groups: dict[tuple[str, str], list[ExperimentResult]] = defaultdict(list)
    for r in results:
        groups[(r.instance_id, r.mode)].append(r)

    print("\n" + "=" * 90)
    print("PODSUMOWANIE WYNIKOW POROWNAWCZYCH (PREPROCESSING)")
    print("=" * 90)
    header = (
        f"{'Instancja':12} {'Wariant':10} {'Runs':>5} {'Wykonalne':>10} "
        f"{'Avg Hard':>9} {'Avg Soft':>9} {'Avg Krok Wykon.':>16} {'Avg Czas(s)':>12}"
    )
    print(header)
    print("-" * len(header))

    for (inst, mode), group in sorted(groups.items()):
        total_runs = len(group)
        feasible_runs = sum(1 for r in group if r.is_feasible)
        feas_pct = f"{feasible_runs}/{total_runs} ({100*feasible_runs/total_runs:.0f}%)"
        avg_hard = sum(r.infeasibility_after for r in group) / total_runs
        avg_soft = sum(r.objective_after for r in group) / total_runs
        avg_time = sum(r.elapsed_s for r in group) / total_runs

        feas_steps = [
            r.first_feasible_iteration
            for r in group
            if r.first_feasible_iteration is not None
        ]
        avg_feas_step = (
            f"{sum(feas_steps)/len(feas_steps):,.0f}" if feas_steps else "-"
        )

        print(
            f"{inst:12} {mode:10} {total_runs:5d} {feas_pct:>10} "
            f"{avg_hard:9.1f} {avg_soft:9.1f} {avg_feas_step:>16} {avg_time:12.1f}"
        )
    print("=" * 90 + "\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Porownawczy runner eksperymentow dla preprocessingu."
    )
    parser.add_argument(
        "--instances",
        nargs="+",
        default=DEFAULT_INSTANCES,
        help=f"Lista ID instancji (domyslnie: {' '.join(DEFAULT_INSTANCES)})",
    )
    parser.add_argument(
        "--modes",
        nargs="+",
        choices=["adaptive", "baseline"],
        default=DEFAULT_MODES,
        help="Warianty do porownania: adaptive (preprocessing) lub baseline",
    )
    parser.add_argument(
        "--seeds",
        nargs="+",
        type=int,
        default=DEFAULT_SEEDS,
        help=f"Lista seedow (domyslnie: {' '.join(str(s) for s in DEFAULT_SEEDS)})",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=DEFAULT_ITERATIONS,
        help=f"Liczba iteracji na przebieg (domyslnie: {DEFAULT_ITERATIONS:,})",
    )
    parser.add_argument(
        "--time-limit",
        type=float,
        default=None,
        help="Opcjonalny limit czasu w sekundach",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=os.cpu_count() or 4,
        help="Liczba rownoleglych procesow roboczych (domyslnie: liczba rdzeni CPU)",
    )
    parser.add_argument(
        "--selector",
        default="ucb",
        choices=["ucb", "epsilon-greedy", "random"],
        help="Rodzaj selektora heurystyk (domyslnie: ucb)",
    )
    parser.add_argument(
        "--acceptance",
        default="sa",
        choices=["sa", "lahc"],
        help="Kryterium akceptacji (domyslnie: sa)",
    )
    parser.add_argument(
        "--archive",
        type=Path,
        default=DEFAULT_ARCHIVE,
        help="Sciezka do archiwum XHSTT XML",
    )
    parser.add_argument(
        "--output-csv",
        type=Path,
        default=DEFAULT_CSV_PATH,
        help=f"Sciezka do pliku wynikowego CSV (domyslnie: {DEFAULT_CSV_PATH})",
    )
    parser.add_argument(
        "--log-dir",
        type=Path,
        default=DEFAULT_LOG_DIR,
        help=f"Katalog na szczegolowe logi JSONL (domyslnie: {DEFAULT_LOG_DIR})",
    )
    parser.add_argument(
        "--no-logs",
        action="store_true",
        help="Wylacz generowanie szczegolowych plikow logow JSONL",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not args.archive.exists():
        raise SystemExit(f"Nie znaleziono archiwum: {args.archive}")

    # Build matrix of tasks
    tasks: list[tuple[str, str, int]] = [
        (inst, mode, seed)
        for inst in args.instances
        for mode in args.modes
        for seed in args.seeds
    ]
    total_tasks = len(tasks)

    if not args.no_logs:
        args.log_dir.mkdir(parents=True, exist_ok=True)

    print("=" * 80)
    print("START EKSPERYMENTOW POROWNAWCZYCH (PREPROCESSING & ADAPTIVE SOLVER)")
    print("=" * 80)
    print(f"Instancje ({len(args.instances)}): {', '.join(args.instances)}")
    print(f"Warianty  ({len(args.modes)}): {', '.join(args.modes)}")
    print(f"Seedy     ({len(args.seeds)}): {', '.join(str(s) for s in args.seeds)}")
    print(f"Iteracje na run: {args.iterations:,}")
    if args.time_limit is not None:
        print(f"Limit czasu na run: {args.time_limit:.1f}s")
    print(f"Liczba zadan: {total_tasks}")
    print(f"Liczba procesow (workers): {args.workers}")
    print(f"Plik CSV: {args.output_csv}")
    if not args.no_logs:
        print(f"Logi JSONL: {args.log_dir}/")
    print("=" * 80 + "\n")

    results: list[ExperimentResult] = []
    t_start_all = time.monotonic()
    done_count = 0

    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        futures = {
            pool.submit(
                _run_single_task,
                inst,
                mode,
                args.selector,
                args.acceptance,
                seed,
                args.iterations,
                args.time_limit,
                args.log_dir / f"{inst}_{mode}_seed{seed:02d}.jsonl"
                if not args.no_logs
                else None,
                args.archive,
            ): (inst, mode, seed)
            for inst, mode, seed in tasks
        }

        for fut in as_completed(futures):
            inst, mode, seed = futures[fut]
            try:
                res = fut.result()
                results.append(res)
                done_count += 1
                _append_to_csv(args.output_csv, res)
                _print_run_log(done_count, total_tasks, res)
            except Exception as e:
                done_count += 1
                print(f"[BLAD] {inst} [{mode}] seed={seed}: {e}", file=sys.stderr)

    total_time = time.monotonic() - t_start_all
    min_time = total_time / 60.0
    print(f"\nWszystkie zadania ukonczone w {total_time:.1f}s ({min_time:.1f} min).")
    print(f"Wyniki zapisano do: {args.output_csv}")

    if results:
        _print_summary_table(results)


if __name__ == "__main__":
    main()

