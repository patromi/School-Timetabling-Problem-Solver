"""Baseline experiment runner across 15 diverse XHSTT instances.

Runs the baseline solver (without preprocessing / adaptive priors) across
15 instances of varied size and complexity, tracking the exact iteration
where hard constraints reach 0.

Designed for high-throughput batch execution on multi-core compute nodes
(e.g., SLURM clusters or direct SSH sessions).

Outputs:
  - data/results/baseline_experiments.csv (appended atomically per finished run)
  - data/logs/baseline_experiments/*.jsonl (detailed solver trajectories)

Usage:
    uv run python baseline_experiments.py
    uv run python baseline_experiments.py --iterations 500000 --workers 120
    uv run python baseline_experiments.py --instances BR-SA-00 GR-PA-08 --seeds 0 1 2
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

# 17 instances spanning small, medium, large, and very large scales,
# various tightness levels, constraint counts (9 to 172), and school systems
# (BR, FI, GR, ZA, AU, IT, KS, DK, NL).
DEFAULT_INSTANCES = [
    # Small (< 200 events)
    "BR-SA-00",  # 63 events, 25 times, tight (class max 100%)
    "BR-SM-00",  # 127 events, 25 times, tight (class max 100%)
    "BR-SN-00",  # 140 events, 25 times, tight (class max 100%)
    "FI-WP-06",  # 172 events, 35 times, moderate
    "GR-P3-10",  # 178 events, 35 times, 102 constraints, loose
    "ZA-LW-09",  # 185 events, 148 times, loose
    # Medium (200 - 500 events)
    "GR-PA-08",  # 262 events, 35 times, 64 constraints
    "ZA-WD-09",  # 278 events, 42 times, tight (100% class utilization)
    "FI-MP-06",  # 280 events, 35 times, 93 constraints
    "AU-SA-96",  # 296 events, 60 times, 61 constraints
    "GR-H1-97",  # 372 events, 35 times, 9 constraints, loose
    "FI-PB-98",  # 387 events, 40 times, 50 constraints, moderate
    "AU-BG-98",  # 387 events, 40 times, 172 constraints
    # Large & Very Large (> 500 events)
    "IT-I4-96",  # 748 events, 36 times, 73 constraints
    "KS-PR-11",  # 809 events, 62 times, 15 constraints, loose
    "DK-FG-12",  # 1077 events, 50 times, 438 resources
    "NL-KP-05",  # 1235 events, 37 times, 644 resources (largest in XHSTT)
]

DEFAULT_SELECTORS = ["ucb"]
DEFAULT_SEEDS = list(range(10))
DEFAULT_ITERATIONS = 500_000
DEFAULT_CSV_PATH = Path("data/results/baseline_experiments.csv")
DEFAULT_LOG_DIR = Path("data/logs/baseline_experiments")
LOG_WINDOW_SIZE = 2000
MIN_RELATIVE_IMPROVEMENT = 0.0


@dataclass(frozen=True)
class RunRecord:
    """Complete result record for a single run, written to CSV."""

    instance_id: str
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
    events_count: int
    times_count: int
    resources_count: int
    constraints_count: int
    scale: str
    tightness: str
    selector: str
    acceptance: str
    acceptance_detail: str


def _make_selector(selector_name: str) -> "HeuristicSelector":
    from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector

    if selector_name == "epsilon-greedy":
        return EpsilonGreedySelector()
    if selector_name == "ucb":
        return UCBSelector()
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
    selector_name: str,
    acceptance_name: str,
    seed: int,
    iterations: int,
    time_limit: float | None,
    log_path: Path | None,
    archive_path: Path,
) -> RunRecord:
    """Worker task executing one baseline solver run."""
    from src.construct import build_greedy
    from src.evaluator_ref import evaluate_cost_components
    from src.lahc import RunStats, run_lahc
    from src.parser import parse_archive
    from src.preprocessing import analyze_instance
    from src.solve_log import SolveLogger

    archive_text = archive_path.read_text(encoding="utf-8-sig")
    instances = parse_archive(archive_text)
    instance = next(i for i in instances if i.id == instance_id)

    profile = analyze_instance(instance)

    rng = random.Random(seed)
    initial = build_greedy(instance, rng)
    inf0, obj0 = evaluate_cost_components(instance, initial)

    selector = _make_selector(selector_name)
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

    idle_limit = max(3000, 50 * len(instance.events))
    stats = RunStats()
    t0 = time.monotonic()

    try:
        best, _ = run_lahc(
            instance,
            initial,
            rng,
            max_iterations=iterations,
            max_seconds=time_limit,
            selector=selector,
            acceptance=acceptance,
            evaluation="incremental",
            on_progress=_on_progress,
            progress_every=LOG_WINDOW_SIZE,
            logger=logger,
            idle_limit=idle_limit,
            min_relative_improvement=MIN_RELATIVE_IMPROVEMENT,
            perturbations=True,
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

    return RunRecord(
        instance_id=instance_id,
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
        events_count=profile.events_count,
        times_count=profile.times_count,
        resources_count=profile.resources_count,
        constraints_count=profile.constraints_count,
        scale=profile.scale,
        tightness=profile.tightness_category,
        selector=selector_name,
        acceptance=acceptance_name,
        acceptance_detail=acceptance.name,
    )


def _append_to_csv(csv_path: Path, result: RunRecord) -> None:
    """Atomically append a single result record to CSV."""
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    file_exists = csv_path.exists() and csv_path.stat().st_size > 0
    row_dict = asdict(result)

    with csv_path.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(row_dict.keys()))
        if not file_exists:
            writer.writeheader()
        writer.writerow(row_dict)
        f.flush()


def _print_run_log(idx: int, total: int, r: RunRecord) -> None:
    """Print clean single-line progress update."""
    if r.is_feasible:
        sec_str = (
            f" ({r.first_feasible_seconds:.2f}s)"
            if r.first_feasible_seconds is not None
            else ""
        )
        status = f"WYKONALNY w kroku {r.first_feasible_iteration}{sec_str}"
    else:
        status = f"NIEWYKONALNY (twarde={r.infeasibility_after})"

    prefix = f"[{idx:>{len(str(total))}}/{total}] {r.instance_id:10} [{r.selector:6}]"
    hard_chg = f"hard {r.infeasibility_before:>3} -> {r.infeasibility_after:>2}"
    soft_chg = f"soft {r.objective_before:>4} -> {r.objective_after:>4}"
    perf = (
        f"{r.iterations_done:,} it w {r.elapsed_s:.1f}s "
        f"({r.iterations_per_sec:,.0f} it/s)"
    )
    print(f"{prefix} seed={r.seed:2d}: {hard_chg}, {soft_chg} | {perf} | {status}")


def _print_summary_table(results: list[RunRecord]) -> None:
    """Print grouped summary table per instance and selector."""
    groups: dict[tuple[str, str], list[RunRecord]] = defaultdict(list)
    for r in results:
        groups[(r.instance_id, r.selector)].append(r)

    print("\n" + "=" * 102)
    print("PODSUMOWANIE EKSPERYMENTU BAZOWEGO")
    print("=" * 102)
    header = (
        f"{'Instancja':12} {'Selektor':8} {'Skala':8} {'Runs':>5} {'Wykonalne':>11} "
        f"{'Avg Hard':>9} {'Avg Soft':>9} {'Avg Krok Wykon.':>16} {'Avg Czas(s)':>12}"
    )
    print(header)
    print("-" * len(header))

    for (inst, sel), group in sorted(
        groups.items(),
        key=lambda item: (item[1][0].events_count, item[0][0], item[0][1]),
    ):
        total_runs = len(group)
        feasible_runs = sum(1 for r in group if r.is_feasible)
        feas_pct = f"{feasible_runs}/{total_runs} ({100*feasible_runs/total_runs:.0f}%)"
        avg_hard = sum(r.infeasibility_after for r in group) / total_runs
        avg_soft = sum(r.objective_after for r in group) / total_runs
        avg_time = sum(r.elapsed_s for r in group) / total_runs
        scale = group[0].scale

        feas_steps = [
            r.first_feasible_iteration
            for r in group
            if r.first_feasible_iteration is not None
        ]
        avg_feas_step = (
            f"{sum(feas_steps)/len(feas_steps):,.0f}" if feas_steps else "-"
        )

        print(
            f"{inst:12} {sel:8} {scale:8} {total_runs:5d} {feas_pct:>11} "
            f"{avg_hard:9.1f} {avg_soft:9.1f} {avg_feas_step:>16} {avg_time:12.1f}"
        )
    print("=" * 102 + "\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Eksperyment bazowy na 15 zróżnicowanych instancjach XHSTT."
    )
    parser.add_argument(
        "--instances",
        nargs="+",
        default=DEFAULT_INSTANCES,
        help="Lista ID instancji",
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
        help="Opcjonalny limit czasu w sekundach na run",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=os.cpu_count() or 4,
        help="Liczba rownoleglych procesow roboczych (domyslnie: liczba rdzeni CPU)",
    )
    parser.add_argument(
        "--selectors",
        nargs="+",
        default=DEFAULT_SELECTORS,
        choices=["ucb", "epsilon-greedy", "random"],
        help=f"Rodzaje selektorow (domyslnie: {' '.join(DEFAULT_SELECTORS)})",
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

    tasks: list[tuple[str, str, int]] = [
        (inst, sel, seed)
        for inst in args.instances
        for sel in args.selectors
        for seed in args.seeds
    ]
    total_tasks = len(tasks)

    if not args.no_logs:
        args.log_dir.mkdir(parents=True, exist_ok=True)

    print("=" * 80)
    print("START EKSPERYMENTU BAZOWEGO (XHSTT)")
    print("=" * 80)
    print(f"Instancje ({len(args.instances)}): {', '.join(args.instances)}")
    print(f"Selektory ({len(args.selectors)}): {', '.join(args.selectors)}")
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

    results: list[RunRecord] = []
    t_start_all = time.monotonic()
    done_count = 0

    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        futures = {
            pool.submit(
                _run_single_task,
                inst,
                sel,
                args.acceptance,
                seed,
                args.iterations,
                args.time_limit,
                args.log_dir / f"{inst}_{sel}_seed{seed:02d}.jsonl"
                if not args.no_logs
                else None,
                args.archive,
            ): (inst, sel, seed)
            for inst, sel, seed in tasks
        }

        for fut in as_completed(futures):
            inst, sel, seed = futures[fut]
            try:
                res = fut.result()
                results.append(res)
                done_count += 1
                _append_to_csv(args.output_csv, res)
                _print_run_log(done_count, total_tasks, res)
            except Exception as e:
                done_count += 1
                print(f"[BLAD] {inst} [{sel}] seed={seed}: {e}", file=sys.stderr)

    total_time = time.monotonic() - t_start_all
    min_time = total_time / 60.0
    print(f"\nWszystkie zadania ukonczone w {total_time:.1f}s ({min_time:.1f} min).")
    print(f"Wyniki zapisano do: {args.output_csv}")

    if results:
        _print_summary_table(results)


if __name__ == "__main__":
    main()

