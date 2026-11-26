"""Experiment runner: compares selectors, acceptance criteria, and LLM variants.

Usage:
    uv run python experiments.py
    uv run python experiments.py --time-limit 120 --workers 4
    uv run python experiments.py --instances BR-SA-00 ZA-WD-09 --seeds 0 1 2 3
    uv run python experiments.py --acceptance lahc sa

EXP-L1 — static pool vs LLM pool mode:
    uv run python experiments.py \\
        --selectors ucb \\
        --llm-variants none a \\
        --time-limit 300 --workers 96

    The time limit is search time: time spent in the LLM generator does not
    count against it and is reported separately (column llm_s). With a local
    Ollama (LLM_BACKEND=ollama) use --workers 1 so the runs do not compete
    for one GPU; with a vLLM server on another node (LLM_BACKEND=openai) the
    runs can be parallelised freely.
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

from src.lfs_sync import pull_generated as _lfs_pull

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
DEFAULT_INSTANCES = ["BR-SA-00", "ZA-WD-09", "AU-SA-96"]
DEFAULT_SELECTORS = ["random", "epsilon-greedy", "ucb"]
DEFAULT_ACCEPTANCE = ["sa"]
DEFAULT_SEEDS = list(range(10))
DEFAULT_TIME_LIMIT = 120  # seconds per run
# LLM prompt variants; "none" means LLM disabled (static pool).
DEFAULT_LLM_VARIANTS = ["none"]
DEFAULT_LLM_TRIGGER = 3  # idle_limit multiplier before LLM fires
HISTORY_LENGTH = 30
LOG_WINDOW_SIZE = 2000  # iterations per selector_window log record
# Iterations without a new best before perturbation; thirds of it are the
# low/medium/high stagnation buckets of the UCB state. Perturbations are off
# by default (see below), so it now only sets those buckets (and when the LLM
# generator fires). 6000 won the tuning (scripts/tune_selector.py, rounds 1
# and 2, docs/wnioski_strojenie_2026-11-24.md); it was 3000 before, chosen
# from the gaps between new_best records (BR-SA-00: p90 ~800, p99 ~7300).
STAGNATION_LIMIT = 6000
STAGNATION_LIMIT_BEFORE_TUNING = 3000
# SA starting temperature as a fraction of the starting cost, per selector,
# from the second tuning round (sa2final). Values in 0.01-0.05 ranked almost
# the same, so the split matters little; selectors not listed use the
# default 0.01.
TUNED_SA_T0 = {
    "random": 0.01,
    "epsilon-greedy": 0.01,
    "ucb": 0.02,
    "ucb-preprocessing": 0.02,
    "ucb-lex": 0.02,
}
DEFAULT_SA_T0 = 0.01
# A new best resets stagnation only if it improves the best cost at the last
# reset by at least this fraction (0 = every new best resets it). 0.05 was
# tried (experiment v3b vs v2, LAHC): perturbations then fired on ZA-WD-09,
# but UCB ended worse (mean hard 103.5 -> 112.3, 2:8 seeds, p = 0.020), so it
# is off.
MIN_RELATIVE_IMPROVEMENT = 0.0
# Iteration cap of one run. Runs are bounded by --time-limit; the cap only
# has to be out of reach. It used to be 10 million, which a fast instance
# (GR-PA-08 does ~3000 it/s) hits after ~1 h and then stops early.
MAX_ITERATIONS = 1_000_000_000


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
    # wall-clock time of the whole run, search plus LLM (llm_s)
    elapsed_s: float
    iterations_done: int
    # full description of the acceptance criterion, e.g.
    # "sa_auto(t0_factor=0.01,t_end=1.0,budget=120s)": the short name "sa"
    # meant T0 = 1e-3 before 18.10.2026 and 1e-2 after, this does not drift
    acceptance_detail: str = ""
    # LLM fields — "none" / "a" / "b" / "rag"; rest are 0 when LLM disabled.
    llm_variant: str = "none"
    llm_trigger: int = DEFAULT_LLM_TRIGGER
    llm_calls: int = 0
    llm_candidates: int = 0
    llm_accepted: int = 0
    # wall-clock time in the LLM generator, outside the search time limit
    llm_s: float = 0.0
    # direct LLM operator variant: none / 1 / 2 / 3 / all
    llm_op: str = "none"


def _apply_preset(args: argparse.Namespace) -> None:
    """Older configurations with one flag instead of several.

    przed_strojeniem: the defaults from issue #82 until the tuning of
    24.11.2026 (experiment 1163195, data/results/static_v2): SA T0 = 0.01 for
    every selector, re-heated over the full budget after each perturbation,
    perturbations after 3000 idle iterations.
    bazowy: additionally everything from before issue #82."""
    if args.preset not in ("przed_strojeniem", "bazowy"):
        return
    args.untuned_sa = True
    args.reheat = "full"
    args.perturbations = True
    args.idle_limit = STAGNATION_LIMIT_BEFORE_TUNING
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
    if selector_name == "epsilon-greedy-e02":
        return EpsilonGreedySelector(epsilon=0.2)
    if selector_name in ("ucb", "ucb-lex"):
        return UCBSelector()
    if selector_name == "ucb-2state":
        return UCBSelector(collapse_stagnation=True)
    if selector_name == "ucb-c03":
        return UCBSelector(c=0.3)
    if selector_name == "ucb-c2":
        return UCBSelector(c=2.0)
    if selector_name == "ucb-alpha005":
        return UCBSelector(alpha=0.05)
    if selector_name == "ucb-alpha015":
        return UCBSelector(alpha=0.15)
    if selector_name == "ucb-2state-c03":
        return UCBSelector(c=0.3, collapse_stagnation=True)
    if selector_name.startswith("ucb-a="):
        return UCBSelector(alpha=float(selector_name.removeprefix("ucb-a=")))
    if selector_name.startswith("ucb-alpha="):
        return UCBSelector(alpha=float(selector_name.removeprefix("ucb-alpha=")))
    if selector_name.startswith("ucb-c="):
        return UCBSelector(c=float(selector_name.removeprefix("ucb-c=")))
    # ucb-preprocessing is handled in _run_one (needs instance access)
    return RandomSelector()


def _sa_t0(selector_name: str, untuned: bool = False) -> float:
    """T0 factor of plain "sa": tuned per selector, or 0.01 for all."""
    if untuned:
        return DEFAULT_SA_T0
    return TUNED_SA_T0.get(selector_name, DEFAULT_SA_T0)


def _make_acceptance(
    acceptance_name: str,
    time_limit: float,
    iteration_budget: int | None = None,
    selector_name: str = "",
    reheat: str = "remaining",
    untuned_sa: bool = False,
) -> "AcceptanceCriterion":
    from src.acceptance import AutoSAAcceptance, LAHCAcceptance

    if acceptance_name == "lahc-L10":
        return LAHCAcceptance(10)
    if acceptance_name == "lahc-L100":
        return LAHCAcceptance(100)
    if acceptance_name == "lahc-L300":
        return LAHCAcceptance(300)
    t0_factor = {
        "sa": _sa_t0(selector_name, untuned_sa),
        "sa-t0005": 0.0005,
        "sa-t005": 0.005,
    }.get(acceptance_name)
    if t0_factor is None and acceptance_name.startswith("sa-t0="):
        t0_factor = float(acceptance_name.removeprefix("sa-t0="))
    if t0_factor is not None:
        return AutoSAAcceptance(
            t0_factor=t0_factor,
            time_budget=time_limit,
            iteration_budget=iteration_budget,
            reheat=reheat,
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
    perturbations: bool = False,
    llm_variant: str = "none",
    llm_trigger: int = DEFAULT_LLM_TRIGGER,
    iteration_budget: int | None = None,
    idle_limit: int = STAGNATION_LIMIT,
    reheat: str = "remaining",
    untuned_sa: bool = False,
    llm_op: str = "none",
) -> RunResult:
    """Run one solver configuration. Imports inside so ProcessPoolExecutor
    can spawn cleanly without pickling the module-level state."""
    import json
    from pathlib import Path as _Path

    from src.construct import build_greedy, build_initial
    from src.cost import cost_by_constraint_type
    from src.evaluator_ref import evaluate_cost_components
    from src.heuristics import MANUAL_HEURISTICS
    from src.lahc import RunStats, run_lahc
    from src.llm.pool import GenerationStats
    from src.parser import parse_archive
    from src.solve_log import SolveLogger

    archive_text = DEFAULT_ARCHIVE.read_text(encoding="utf-8-sig")
    instances = parse_archive(archive_text)
    instance = next(i for i in instances if i.id == instance_id)

    rng = random.Random(seed)
    build = build_greedy if construct == "greedy" else build_initial
    initial = build(instance, rng)
    inf0, obj0 = evaluate_cost_components(instance, initial)

    base_pool = [h for h in MANUAL_HEURISTICS if h.id not in exclude_heuristics]

    if selector_name == "ucb-preprocessing":
        from src.preprocessing import analyze_instance, filter_heuristic_pool, get_initial_q_map
        from src.selectors import UCBSelector

        profile = analyze_instance(instance)
        q_map = get_initial_q_map(profile)
        selector = UCBSelector(initial_q_map=q_map)
        base_pool = filter_heuristic_pool(profile, base_pool)
    else:
        selector = _make_selector(selector_name)

    acceptance = _make_acceptance(
        acceptance_name, time_limit, iteration_budget, selector_name, reheat, untuned_sa
    )

    llm_enabled = llm_variant != "none"
    llm_stats = GenerationStats() if llm_enabled else None

    llm_operators = None
    if llm_op != "none":
        from src.llm.operator import LLM_OPERATOR_HEURISTICS
        op_map = {"1": [0], "2": [1], "3": [2], "all": [0, 1, 2]}
        llm_operators = [LLM_OPERATOR_HEURISTICS[i] for i in op_map[llm_op]]

    iters_done = [0]
    run_stats = RunStats()

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
            max_iterations=iteration_budget if iteration_budget is not None else MAX_ITERATIONS,
            max_seconds=None if iteration_budget is not None else time_limit,
            selector=selector,
            acceptance=acceptance,
            evaluation="incremental",
            on_progress=_on_progress,
            progress_every=LOG_WINDOW_SIZE,
            logger=logger,
            idle_limit=idle_limit,
            min_relative_improvement=MIN_RELATIVE_IMPROVEMENT,
            select_perturbations=select_perturbations,
            time_aware_reward=time_aware_reward,
            # ucb-lex: UCB with the lexicographic reward (src.lahc.selector_reward)
            reward_mode="lex" if selector_name == "ucb-lex" else "binary",
            heuristics=base_pool,
            perturbations=perturbations,
            llm_enabled=llm_enabled,
            llm_prompt_variant=llm_variant if llm_enabled else "a",
            llm_trigger_multiplier=llm_trigger,
            llm_stats=llm_stats,
            llm_operators=llm_operators,
            seed=seed,
            stats=run_stats,
        )
    finally:
        if logger is not None:
            logger.flush()
        if log_file is not None:
            log_file.close()

    elapsed = time.monotonic() - t0
    inf1, obj1 = evaluate_cost_components(instance, best)

    llm_calls = llm_stats.total_calls if llm_stats else 0
    llm_candidates_gen = llm_stats.candidates_generated if llm_stats else 0
    llm_accepted = llm_stats.accepted if llm_stats else 0

    if logger is not None and log_path is not None:
        with _Path(log_path).open("a", encoding="utf-8") as f:
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
                        "perturbations": perturbations,
                        "time_aware_reward": time_aware_reward,
                        "exclude_heuristics": list(exclude_heuristics),
                        "llm_variant": llm_variant,
                        "llm_trigger": llm_trigger,
                        "llm_calls": llm_calls,
                        "llm_candidates": llm_candidates_gen,
                        "llm_accepted": llm_accepted,
                        "infeasibility_before": inf0,
                        "objective_before": obj0,
                        "infeasibility_after": inf1,
                        "objective_after": obj1,
                        "by_type_before": cost_by_constraint_type(instance, initial),
                        "by_type_after": cost_by_constraint_type(instance, best),
                        "iterations_done": iters_done[0],
                        "elapsed_s": round(elapsed, 1),
                        "llm_s": round(run_stats.llm_s, 1),
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
        llm_variant=llm_variant,
        llm_trigger=llm_trigger,
        llm_calls=llm_calls,
        llm_candidates=llm_candidates_gen,
        llm_accepted=llm_accepted,
        llm_s=round(run_stats.llm_s, 1),
        llm_op=llm_op,
    )


def _print_table(results: list[RunResult]) -> None:
    from collections import defaultdict

    groups: dict[tuple[str, str, str, str], list[RunResult]] = defaultdict(list)
    for r in results:
        groups[(r.instance_id, r.selector, r.acceptance, r.llm_variant)].append(r)

    header = (
        f"{'Instancja':12} {'Selektor':15} {'Acc':4} {'LLM':5} {'N':>3} "
        f"{'inf_avg':>9} {'inf_min':>7} "
        f"{'obj_avg':>9} {'obj_min':>7} "
        f"{'t_avg':>7} {'llm_acc':>7}"
    )
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))

    for (inst, sel, acc, llm), runs in sorted(groups.items()):
        infs = [r.infeasibility_after for r in runs]
        objs = [r.objective_after for r in runs]
        ts = [r.elapsed_s for r in runs]
        llm_acc_total = sum(r.llm_accepted for r in runs)
        llm_cand_total = sum(r.llm_candidates for r in runs)
        llm_rate = f"{llm_acc_total}/{llm_cand_total}" if llm_cand_total > 0 else "-"
        print(
            f"{inst:12} {sel:15} {acc:4} {llm:5} {len(runs):>3} "
            f"{sum(infs) / len(infs):>9.1f} {min(infs):>7} "
            f"{sum(objs) / len(objs):>9.1f} {min(objs):>7} "
            f"{sum(ts) / len(ts):>7.1f}s {llm_rate:>7}"
        )
    print("=" * len(header))


def _append_csv(result: RunResult, path: Path) -> None:
    """Append one finished run to the results CSV (header on first write),
    so a long experiment killed half-way keeps every run it finished."""
    path.parent.mkdir(parents=True, exist_ok=True)
    new_file = not path.exists()
    col_names = [field.name for field in fields(RunResult)]
    with path.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=col_names)
        if new_file:
            writer.writeheader()
        writer.writerow(result.__dict__)


def _done_runs(path: Path) -> set[tuple[str, str, str, int, str, str]]:
    """Keys (instance, selector, acceptance, seed, llm_variant, llm_op) of the runs
    already in a results CSV, for --resume."""
    if not path.exists():
        return set()
    with path.open(newline="", encoding="utf-8") as f:
        return {
            (
                row["instance_id"],
                row["selector"],
                row["acceptance"],
                int(row["seed"]),
                row.get("llm_variant") or "none",
                row.get("llm_op") or "none",
            )
            for row in csv.DictReader(f)
        }


def _pending_jobs(
    jobs: list[tuple[str, str, str, int, str, str]], output: Path, resume: bool
) -> list[tuple[str, str, str, int, str, str]]:
    """With `resume`, the jobs not yet in `output`; without it all of them,
    and an existing `output` is removed so the new run starts it afresh."""
    if not resume:
        output.unlink(missing_ok=True)
        return jobs
    done_runs = _done_runs(output)
    print(f"--resume: {len(done_runs)} przebiegow juz w {output}, pomijam je")
    return [job for job in jobs if job not in done_runs]


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
        "--iterations",
        type=int,
        default=None,
        help=(
            "Max iterations per run; overrides --time-limit when set. "
            "SA cooling switches to iteration-budget mode."
        ),
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
        "--resume",
        action="store_true",
        help=(
            "Keep the runs already in --output and run only the missing ones "
            "(without it an existing --output is overwritten)"
        ),
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
        "--perturbations",
        action="store_true",
        help=(
            "Perturb after --idle-limit iterations without a new best "
            "(off by default since the tuning of 24.11.2026)"
        ),
    )
    parser.add_argument(
        "--no-perturbations",
        action="store_true",
        help="Never perturb on stagnation (the default now; kept for old scripts)",
    )
    parser.add_argument(
        "--idle-limit",
        type=int,
        default=STAGNATION_LIMIT,
        help=(
            "Iterations without a new best before a perturbation; thirds of it "
            f"are the UCB stagnation states (default {STAGNATION_LIMIT})"
        ),
    )
    parser.add_argument(
        "--reheat",
        choices=["remaining", "full"],
        default="remaining",
        help=(
            "SA after a perturbation cools over the rest of the budget "
            "(remaining, default) or over the full budget again (full, before "
            "24.11.2026)"
        ),
    )
    parser.add_argument(
        "--untuned-sa",
        action="store_true",
        help=f"SA T0 = {DEFAULT_SA_T0} for every selector instead of TUNED_SA_T0",
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
        choices=["domyslny", "przed_strojeniem", "bazowy"],
        default="domyslny",
        help=(
            "przed_strojeniem: the defaults before 24.11.2026 (SA T0 0.01 "
            "for all, reheat full, perturbations after 3000 idle iterations); "
            "bazowy: that plus every setting from before issue #82 -- SA with "
            "T0 1e-3, naive start, perturbations offered to the selector, "
            "UCB alpha 0.08, no repair_clash"
        ),
    )
    parser.add_argument(
        "--llm-variants",
        nargs="+",
        default=DEFAULT_LLM_VARIANTS,
        metavar="VARIANT",
        help=(
            "LLM prompt variants to include: none a b rag "
            "(default: none — static pool only). "
            "With a local Ollama use --workers 1 when any variant != none."
        ),
    )
    parser.add_argument(
        "--llm-trigger",
        type=int,
        default=DEFAULT_LLM_TRIGGER,
        help=f"idle_limit multiplier before LLM fires (default {DEFAULT_LLM_TRIGGER})",
    )
    parser.add_argument(
        "--llm-operator",
        choices=["none", "1", "2", "3", "all"],
        default="none",
        help=(
            "Direct LLM operator called at stagnation (independent of --llm-variants): "
            "1=JSON moves, 2=event filter+best-slot, 3=chain-of-thought, "
            "all=all three in sequence (default: none)"
        ),
    )
    args = parser.parse_args()
    _apply_preset(args)

    llm_variants: list[str] = args.llm_variants
    has_llm = any(v != "none" for v in llm_variants)

    if has_llm:
        _lfs_pull(verbose=True)
    if (
        has_llm
        and args.workers > 1
        and os.environ.get("LLM_BACKEND", "ollama") == "ollama"
    ):
        print(
            "UWAGA: --llm-variants zawiera warianty LLM, --workers > 1 i lokalna "
            "Ollama. Wiele procesow moze rywalizowac o GPU. Zalecane --workers 1.",
            file=sys.stderr,
        )

    log_dir: Path | None = None
    if not args.no_logs:
        log_dir = args.log_dir
        log_dir.mkdir(parents=True, exist_ok=True)

    jobs = [
        (inst, sel, acc, seed, llm, args.llm_operator)
        for inst in args.instances
        for sel in args.selectors
        for acc in args.acceptance
        for seed in args.seeds
        for llm in llm_variants
    ]
    jobs = _pending_jobs(jobs, args.output, args.resume)
    total = len(jobs)
    print(
        f"Eksperyment: {len(args.instances)} instancji x "
        f"{len(args.selectors)} selektorow x "
        f"{len(args.acceptance)} kryteriow x "
        f"{len(args.seeds)} seedow x "
        f"{len(llm_variants)} LLM-wariantow = {total} uruchomien"
    )
    if args.iterations is not None:
        print(f"  Limit iteracji: {args.iterations}/run  Workers: {args.workers}")
        print(f"  Szacowany czas całkowity: nieznany (tryb iteracyjny)")
    else:
        print(f"  Limit czasu: {args.time_limit:.0f}s/run  Workers: {args.workers}")
        est_wall = args.time_limit * total / args.workers
        print(f"  Szacowany czas całkowity: ~{est_wall / 60:.0f} min")
    if log_dir:
        print(f"  Logi JSONL: {log_dir}/\n")
    else:
        print()

    def _log_name(inst: str, sel: str, acc: str, seed: int, llm: str) -> "Path | None":
        if log_dir is None:
            return None
        llm_tag = f"_llm-{llm}" if llm != "none" else ""
        return log_dir / f"{inst}_{sel}{llm_tag}_{acc}_seed{seed:02d}.jsonl"

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
                _log_name(inst, sel, acc, seed, llm),
                args.construct,
                args.select_perturbations,
                args.time_aware_reward,
                tuple(args.exclude_heuristic),
                args.perturbations and not args.no_perturbations,
                llm,
                args.llm_trigger,
                args.iterations,
                args.idle_limit,
                args.reheat,
                args.untuned_sa,
                llm_op,
            ): (inst, sel, acc, seed, llm, llm_op)
            for inst, sel, acc, seed, llm, llm_op in jobs
        }
        for fut in as_completed(futures):
            inst, sel, acc, seed, llm, llm_op = futures[fut]
            try:
                r = fut.result()
                results.append(r)
                _append_csv(r, args.output)
                done += 1
                llm_info = f"  llm={llm} acc={r.llm_accepted}/{r.llm_candidates}" if llm != "none" else ""
                op_info = f"  op={llm_op}" if llm_op != "none" else ""
                print(
                    f"  [{done:>3}/{total}] {inst:12} {sel:15} {acc:4} seed={seed}  "
                    f"inf={r.infeasibility_after:>5}  obj={r.objective_after:>6}  "
                    f"iter={r.iterations_done:>7}  {r.elapsed_s:.0f}s{llm_info}{op_info}"
                )
            except Exception as e:
                done += 1
                print(
                    f"  [{done:>3}/{total}] {inst} {sel} {acc} seed={seed} llm={llm} op={llm_op}  BLAD: {e}",
                    file=sys.stderr,
                )

    elapsed_min = (time.monotonic() - t_wall) / 60
    print(f"\nWszystkie uruchomienia zakonczone w {elapsed_min:.1f} min")
    _print_table(results)
    print(f"\nWyniki zapisane do: {args.output}")


if __name__ == "__main__":
    main()
