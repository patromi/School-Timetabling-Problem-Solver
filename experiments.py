"""Experiment runner: compares selectors across instances and seeds.

Usage:
    uv run python experiments.py
    uv run python experiments.py --time-limit 300 --workers 4
    uv run python experiments.py --instances BR-SA-00 ZA-WD-09 --seeds 0 1 2
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

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
DEFAULT_INSTANCES = ["BR-SA-00", "ZA-WD-09", "AU-SA-96"]
DEFAULT_SELECTORS = ["random", "epsilon-greedy", "ucb"]
DEFAULT_SEEDS = [0, 1, 2]
DEFAULT_TIME_LIMIT = 300  # seconds
HISTORY_LENGTH = 30


@dataclass
class RunResult:
    instance_id: str
    selector: str
    seed: int
    infeasibility_before: int
    objective_before: int
    infeasibility_after: int
    objective_after: int
    elapsed_s: float
    iterations_done: int


def _run_one(
    instance_id: str,
    selector_name: str,
    seed: int,
    time_limit: float,
) -> RunResult:
    # Imports inside the worker so ProcessPoolExecutor can spawn cleanly
    from src.construct import build_initial
    from src.evaluator_ref import evaluate_cost_components
    from src.lahc import run_lahc
    from src.parser import parse_archive
    from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector

    archive_text = DEFAULT_ARCHIVE.read_text(encoding="utf-8-sig")
    instances = parse_archive(archive_text)
    instance = next(i for i in instances if i.id == instance_id)

    rng = random.Random(seed)
    initial = build_initial(instance, rng)
    inf0, obj0 = evaluate_cost_components(instance, initial)

    if selector_name == "epsilon-greedy":
        selector = EpsilonGreedySelector()
    elif selector_name == "ucb":
        selector = UCBSelector()
    else:
        selector = RandomSelector()

    t0 = time.monotonic()
    best, _ = run_lahc(
        instance,
        initial,
        rng,
        history_length=HISTORY_LENGTH,
        max_iterations=10_000_000,  # effectively unlimited — time-gated
        max_seconds=time_limit,
        selector=selector,
        evaluation="incremental",
    )
    elapsed = time.monotonic() - t0

    # Count iterations by re-checking cost (best is the result)
    inf1, obj1 = evaluate_cost_components(instance, best)

    return RunResult(
        instance_id=instance_id,
        selector=selector_name,
        seed=seed,
        infeasibility_before=inf0,
        objective_before=obj0,
        infeasibility_after=inf1,
        objective_after=obj1,
        elapsed_s=round(elapsed, 1),
        iterations_done=0,  # not tracked per-run to keep it simple
    )


def _print_table(results: list[RunResult]) -> None:
    # Group by (instance, selector), aggregate over seeds
    from collections import defaultdict

    groups: dict[tuple[str, str], list[RunResult]] = defaultdict(list)
    for r in results:
        groups[(r.instance_id, r.selector)].append(r)

    header = (
        f"{'Instancja':12} {'Selektor':15} {'Seeds':>5} "
        f"{'inf_avg':>9} {'inf_min':>9} "
        f"{'obj_avg':>9} {'obj_min':>9} "
        f"{'t_avg':>7}"
    )
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))

    for (inst, sel), runs in sorted(groups.items()):
        infs = [r.infeasibility_after for r in runs]
        objs = [r.objective_after for r in runs]
        ts = [r.elapsed_s for r in runs]
        print(
            f"{inst:12} {sel:15} {len(runs):>5} "
            f"{sum(infs)/len(infs):>9.1f} {min(infs):>9} "
            f"{sum(objs)/len(objs):>9.1f} {min(objs):>9} "
            f"{sum(ts)/len(ts):>7.1f}s"
        )
    print("=" * len(header))


def _save_csv(results: list[RunResult], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=[field.name for field in fields(RunResult)])
        writer.writeheader()
        for r in results:
            writer.writerow(r.__dict__)
    print(f"\nWyniki zapisane do: {path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Experiment: selector comparison on XHSTT.")
    parser.add_argument("--instances", nargs="+", default=DEFAULT_INSTANCES)
    parser.add_argument("--selectors", nargs="+", default=DEFAULT_SELECTORS)
    parser.add_argument("--seeds", nargs="+", type=int, default=DEFAULT_SEEDS)
    parser.add_argument("--time-limit", type=float, default=DEFAULT_TIME_LIMIT,
                        help="Max seconds per run (default 300)")
    parser.add_argument("--workers", type=int, default=min(os.cpu_count() or 4, 8),
                        help="Parallel workers (default: min(cpu_count, 8))")
    parser.add_argument("--output", type=Path, default=Path("data/results/experiment_selectors.csv"))
    args = parser.parse_args()

    jobs = [
        (inst, sel, seed)
        for inst in args.instances
        for sel in args.selectors
        for seed in args.seeds
    ]
    total = len(jobs)
    print(
        f"Eksperyment: {len(args.instances)} instancji × "
        f"{len(args.selectors)} selektorów × "
        f"{len(args.seeds)} seedów = {total} uruchomień"
    )
    print(f"  Limit czasu: {args.time_limit:.0f}s/run  Workers: {args.workers}")
    est_wall = args.time_limit * total / args.workers
    print(f"  Szacowany czas całkowity: ~{est_wall/60:.0f} min\n")

    results: list[RunResult] = []
    done = 0
    t_wall = time.monotonic()

    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        futures = {
            pool.submit(_run_one, inst, sel, seed, args.time_limit): (inst, sel, seed)
            for inst, sel, seed in jobs
        }
        for fut in as_completed(futures):
            inst, sel, seed = futures[fut]
            try:
                r = fut.result()
                results.append(r)
                done += 1
                print(
                    f"  [{done:>3}/{total}] {inst:12} {sel:15} seed={seed}  "
                    f"inf={r.infeasibility_after:>5}  obj={r.objective_after:>6}  "
                    f"{r.elapsed_s:.0f}s"
                )
            except Exception as e:
                done += 1
                print(f"  [{done:>3}/{total}] {inst} {sel} seed={seed}  BLAD: {e}", file=sys.stderr)

    print(f"\nWszystkie uruchomienia zakończone w {(time.monotonic()-t_wall)/60:.1f} min")
    _print_table(results)
    _save_csv(results, args.output)


if __name__ == "__main__":
    main()
