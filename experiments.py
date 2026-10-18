"""Experiment runner: compares selectors and acceptance criteria.

Usage:
    uv run python experiments.py
    uv run python experiments.py --time-limit 120 --workers 4
    uv run python experiments.py --instances BR-SA-00 ZA-WD-09 --seeds 0 1 2 3
    uv run python experiments.py --acceptance lahc sa
"""

import argparse
import csv
import os
import random
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, fields
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from src.acceptance import AcceptanceCriterion
    from src.selectors import HeuristicSelector

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
DEFAULT_INSTANCES = ["BR-SA-00", "ZA-WD-09", "AU-SA-96"]
DEFAULT_SELECTORS = ["random", "epsilon-greedy", "ucb"]
DEFAULT_ACCEPTANCE = ["sa"]
DEFAULT_SEEDS = list(range(10))
DEFAULT_TIME_LIMIT = 120  # seconds per run
HISTORY_LENGTH = 30
LOG_WINDOW_SIZE = 2000  # iterations per selector_window log record
# Iterations without a new best before perturbation; thirds of it are the
# low/medium/high stagnation buckets of the UCB state. Chosen from the gaps
# between new_best records in random-selector runs (BR-SA-00: p90 ~800,
# p99 ~7300 iterations).
STAGNATION_LIMIT = 3000
# A new best resets stagnation only if it improves the best cost at the last
# reset by at least this fraction (0 = every new best resets it). 0.05 was
# tried (experiment v3b vs v2, LAHC): perturbations then fired on ZA-WD-09,
# but UCB ended worse (mean hard 103.5 -> 112.3, 2:8 seeds, p = 0.020), so it
# is off.
MIN_RELATIVE_IMPROVEMENT = 0.0


@dataclass
class RunResult:
    instance_id: str
    selector: str
    acceptance: str
    seed: int
    infeasibility_before: int
    objective_before: int
    infeasibility_after: int
    objective_after: int
    elapsed_s: float
    iterations_done: int
    # full description of the acceptance criterion, e.g.
    # "sa_auto(t0_factor=0.01,t_end=1.0,budget=120s)": the short name "sa"
    # meant T0 = 1e-3 before 18.10.2026 and 1e-2 after, this does not drift
    acceptance_detail: str = ""


def _apply_preset(args: argparse.Namespace) -> None:
    """--preset bazowy: the configuration before the feasibility work
    (issue #82), so older results can be reproduced with one flag instead
    of five."""
    if args.preset != "bazowy":
        return
    args.acceptance = ["sa-t0=1e-3" if a == "sa" else a for a in args.acceptance]
    args.selectors = ["ucb-a=0.08" if s == "ucb" else s for s in args.selectors]
    args.construct = "naive"
    args.select_perturbations = True
    if "repair_clash" not in args.exclude_heuristic:
        args.exclude_heuristic.append("repair_clash")


def _make_selector(selector_name: str) -> "HeuristicSelector":
    from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector

    if selector_name == "epsilon-greedy":
        return EpsilonGreedySelector()
    if selector_name == "ucb":
        return UCBSelector()
    if selector_name == "ucb-2state":
        return UCBSelector(collapse_stagnation=True)
    if selector_name == "ucb-c03":
        return UCBSelector(c=0.3)
    if selector_name.startswith("ucb-a="):
        # e.g. "ucb-a=0.08" -- the Q forgetting rate (default 0.02)
        return UCBSelector(alpha=float(selector_name.removeprefix("ucb-a=")))
    if selector_name.startswith("ucb-c="):
        # e.g. "ucb-c=0.01" -- used to calibrate the exploration constant
        return UCBSelector(c=float(selector_name.removeprefix("ucb-c=")))
    return RandomSelector()


def _make_acceptance(acceptance_name: str, time_limit: float) -> "AcceptanceCriterion":
    from src.acceptance import AutoSAAcceptance, LAHCAcceptance

    if acceptance_name == "sa":
        return AutoSAAcceptance(time_budget=time_limit)
    if acceptance_name.startswith("sa-t0="):
        # e.g. "sa-t0=1e-4" -- the starting temperature as a fraction of the
        # initial cost, used to tune SA (the default "sa" is 1e-2; it was
        # 1e-3 before 18.10.2026)
        return AutoSAAcceptance(
            t0_factor=float(acceptance_name.removeprefix("sa-t0=")),
            time_budget=time_limit,
        )
    return LAHCAcceptance(HISTORY_LENGTH)


def _run_one(
    instance_id: str,
    selector_name: str,
    acceptance_name: str,
    seed: int,
    time_limit: float,
    log_path: "Path | None",
    construct: str = "greedy",
    select_perturbations: bool = False,
    time_aware_reward: bool = False,
    exclude_heuristics: tuple[str, ...] = (),
) -> RunResult:
    """Run one solver configuration. Imports inside so ProcessPoolExecutor
    can spawn cleanly without pickling the module-level state."""
    from pathlib import Path as _Path

    from src.construct import build_greedy, build_initial
    from src.cost import cost_by_constraint_type
    from src.evaluator_ref import evaluate_cost_components
    from src.heuristics import MANUAL_HEURISTICS
    from src.lahc import run_lahc
    from src.parser import parse_archive
    from src.solve_log import SolveLogger

    archive_text = DEFAULT_ARCHIVE.read_text(encoding="utf-8-sig")
    instances = parse_archive(archive_text)
    instance = next(i for i in instances if i.id == instance_id)

    rng = random.Random(seed)
    build = build_greedy if construct == "greedy" else build_initial
    initial = build(instance, rng)
    inf0, obj0 = evaluate_cost_components(instance, initial)

    selector = _make_selector(selector_name)
    acceptance = _make_acceptance(acceptance_name, time_limit)

    iters_done = [0]

    def _on_progress(it: int, _cost: int) -> None:
        iters_done[0] = it

    log_file = None
    logger = None
    if log_path is not None:
        log_file = _Path(log_path).open("w", encoding="utf-8")
        logger = SolveLogger(log_file, window_size=LOG_WINDOW_SIZE)

    t0 = time.monotonic()
    try:
        best, _ = run_lahc(
            instance,
            initial,
            rng,
            max_iterations=10_000_000,
            max_seconds=time_limit,
            selector=selector,
            acceptance=acceptance,
            evaluation="incremental",
            on_progress=_on_progress,
            progress_every=LOG_WINDOW_SIZE,
            logger=logger,
            idle_limit=STAGNATION_LIMIT,
            min_relative_improvement=MIN_RELATIVE_IMPROVEMENT,
            select_perturbations=select_perturbations,
            time_aware_reward=time_aware_reward,
            heuristics=[h for h in MANUAL_HEURISTICS if h.id not in exclude_heuristics],
        )
    finally:
        if logger is not None:
            logger.flush()
        if log_file is not None:
            log_file.close()

    elapsed = time.monotonic() - t0
    inf1, obj1 = evaluate_cost_components(instance, best)

    if logger is not None and log_path is not None:
        with _Path(log_path).open("a", encoding="utf-8") as f:
            import json

            f.write(
                json.dumps(
                    {
                        "event": "run_summary",
                        "instance_id": instance_id,
                        "selector": selector_name,
                        "acceptance": acceptance_name,
                        "acceptance_detail": acceptance.name,
                        "seed": seed,
                        "construct": construct,
                        "select_perturbations": select_perturbations,
                        "time_aware_reward": time_aware_reward,
                        "exclude_heuristics": list(exclude_heuristics),
                        "infeasibility_before": inf0,
                        "objective_before": obj0,
                        "infeasibility_after": inf1,
                        "objective_after": obj1,
                        # which constraint types the cost comes from, at the
                        # start and for the best solution found
                        "by_type_before": cost_by_constraint_type(instance, initial),
                        "by_type_after": cost_by_constraint_type(instance, best),
                        "iterations_done": iters_done[0],
                        "elapsed_s": round(elapsed, 1),
                    },
                    separators=(",", ":"),
                )
                + "\n"
            )

    return RunResult(
        instance_id=instance_id,
        selector=selector_name,
        acceptance=acceptance_name,
        seed=seed,
        infeasibility_before=inf0,
        objective_before=obj0,
        infeasibility_after=inf1,
        objective_after=obj1,
        elapsed_s=round(elapsed, 1),
        iterations_done=iters_done[0],
        acceptance_detail=acceptance.name,
    )


def _print_table(results: list[RunResult]) -> None:
    from collections import defaultdict

    groups: dict[tuple[str, str, str], list[RunResult]] = defaultdict(list)
    for r in results:
        groups[(r.instance_id, r.selector, r.acceptance)].append(r)

    header = (
        f"{'Instancja':12} {'Selektor':15} {'Akceptacja':8} {'Seeds':>5} "
        f"{'inf_avg':>9} {'inf_min':>9} "
        f"{'obj_avg':>9} {'obj_min':>9} "
        f"{'t_avg':>7}"
    )
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))

    for (inst, sel, acc), runs in sorted(groups.items()):
        infs = [r.infeasibility_after for r in runs]
        objs = [r.objective_after for r in runs]
        ts = [r.elapsed_s for r in runs]
        print(
            f"{inst:12} {sel:15} {acc:8} {len(runs):>5} "
            f"{sum(infs) / len(infs):>9.1f} {min(infs):>9} "
            f"{sum(objs) / len(objs):>9.1f} {min(objs):>9} "
            f"{sum(ts) / len(ts):>7.1f}s"
        )
    print("=" * len(header))


def _save_csv(results: list[RunResult], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        col_names = [field.name for field in fields(RunResult)]
        writer = csv.DictWriter(f, fieldnames=col_names)
        writer.writeheader()
        for r in results:
            writer.writerow(r.__dict__)
    print(f"\nWyniki zapisane do: {path}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Experiment: selector + acceptance comparison on XHSTT."
    )
    parser.add_argument("--instances", nargs="+", default=DEFAULT_INSTANCES)
    parser.add_argument("--selectors", nargs="+", default=DEFAULT_SELECTORS)
    parser.add_argument(
        "--acceptance",
        nargs="+",
        default=DEFAULT_ACCEPTANCE,
        help="Acceptance criteria: sa lahc (default: sa; lahc is historical)",
    )
    parser.add_argument("--seeds", nargs="+", type=int, default=DEFAULT_SEEDS)
    parser.add_argument(
        "--time-limit",
        type=float,
        default=DEFAULT_TIME_LIMIT,
        help=f"Max seconds per run (default {DEFAULT_TIME_LIMIT})",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=min(os.cpu_count() or 4, 8),
        help="Parallel workers (default: min(cpu_count, 8))",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/results/experiment_selectors.csv"),
    )
    parser.add_argument(
        "--log-dir",
        type=Path,
        default=Path("data/logs"),
        help="Directory for per-run JSONL logs (default: data/logs)",
    )
    parser.add_argument(
        "--construct",
        choices=["naive", "greedy"],
        default="greedy",
        help="Initial solution: greedy (clash-aware, default) or naive",
    )
    parser.add_argument(
        "--select-perturbations",
        action="store_true",
        help="Offer small/large_perturbation to the selector (pre-step-6)",
    )
    parser.add_argument(
        "--time-aware-reward",
        action="store_true",
        help="Scale the selector reward by mean move time / move time",
    )
    parser.add_argument(
        "--exclude-heuristic",
        action="append",
        default=[],
        help="Heuristic id to leave out of the pool (repeatable)",
    )
    parser.add_argument(
        "--no-logs",
        action="store_true",
        help="Disable per-run JSONL logging",
    )
    parser.add_argument(
        "--preset",
        choices=["domyslny", "bazowy"],
        default="domyslny",
        help=(
            "bazowy: every setting from before issue #82 at once -- SA with "
            "T0 1e-3, naive start, perturbations offered to the selector, "
            "UCB alpha 0.08, no repair_clash"
        ),
    )
    args = parser.parse_args()
    _apply_preset(args)

    log_dir: Path | None = None
    if not args.no_logs:
        log_dir = args.log_dir
        log_dir.mkdir(parents=True, exist_ok=True)

    jobs = [
        (inst, sel, acc, seed)
        for inst in args.instances
        for sel in args.selectors
        for acc in args.acceptance
        for seed in args.seeds
    ]
    total = len(jobs)
    print(
        f"Eksperyment: {len(args.instances)} instancji x "
        f"{len(args.selectors)} selektorow x "
        f"{len(args.acceptance)} kryteriow x "
        f"{len(args.seeds)} seedow = {total} uruchomien"
    )
    print(f"  Limit czasu: {args.time_limit:.0f}s/run  Workers: {args.workers}")
    est_wall = args.time_limit * total / args.workers
    print(f"  Szacowany czas całkowity: ~{est_wall / 60:.0f} min")
    if log_dir:
        print(f"  Logi JSONL: {log_dir}/\n")
    else:
        print()

    results: list[RunResult] = []
    done = 0
    t_wall = time.monotonic()

    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        futures = {
            pool.submit(
                _run_one,
                inst,
                sel,
                acc,
                seed,
                args.time_limit,
                log_dir / f"{inst}_{sel}_{acc}_seed{seed:02d}.jsonl"
                if log_dir
                else None,
                args.construct,
                args.select_perturbations,
                args.time_aware_reward,
                tuple(args.exclude_heuristic),
            ): (inst, sel, acc, seed)
            for inst, sel, acc, seed in jobs
        }
        for fut in as_completed(futures):
            inst, sel, acc, seed = futures[fut]
            try:
                r = fut.result()
                results.append(r)
                done += 1
                print(
                    f"  [{done:>3}/{total}] {inst:12} {sel:15} {acc:4} seed={seed}  "
                    f"inf={r.infeasibility_after:>5}  obj={r.objective_after:>6}  "
                    f"iter={r.iterations_done:>7}  {r.elapsed_s:.0f}s"
                )
            except Exception as e:
                done += 1
                print(
                    f"  [{done:>3}/{total}] {inst} {sel} {acc} seed={seed}  BLAD: {e}",
                    file=sys.stderr,
                )

    elapsed_min = (time.monotonic() - t_wall) / 60
    print(f"\nWszystkie uruchomienia zakonczone w {elapsed_min:.1f} min")
    _print_table(results)
    _save_csv(results, args.output)


if __name__ == "__main__":
    main()
