"""Resumable, iteration-bounded research campaigns (Linux/macOS/WSL)."""

from __future__ import annotations

import argparse
import csv
import fcntl
import hashlib
import json
import math
import multiprocessing
import os
import platform
import random
import signal
import statistics
import subprocess
import sys
import traceback
from collections import defaultdict
from collections.abc import Iterator, Mapping, Sequence
from contextlib import contextmanager
from dataclasses import asdict, dataclass, replace
from multiprocessing.connection import wait
from multiprocessing.process import BaseProcess
from pathlib import Path
from types import FrameType
from typing import Any

from experiments import DEFAULT_ARCHIVE, RunResult, _run_one

ROOT = Path(__file__).resolve().parent
# "resource" instances leave event resources for the solver to assign
# (resource_domains.has_resource_choices); the rest only need start times.
# The two tracks differ in cost per iteration and in the search space, so they
# are scheduled, budgeted and reported separately.
TRACKS = ("time", "resource")
TRACK_INSTANCES = {
    "time": ("BR-SA-00", "GR-H1-97", "GR-PA-08", "ZA-WD-09", "KS-PR-11"),
    "resource": ("ES-SS-08", "US-WS-09", "DK-FG-12", "DK-HG-12", "DK-VG-09"),
}
DEFAULT_INSTANCES = TRACK_INSTANCES["time"]
# 15-30 ms/iteration on the large resource instances: 1M would take ~6 h a run.
TRACK_BUDGETS = {"time": (50_000, 300_000), "resource": (50_000,)}


@dataclass(frozen=True)
class Variant:
    name: str
    selector: str = "ucb"
    construct: str = "greedy"
    perturbations: bool = True
    reheat: str = "remaining"
    idle_limit: int | None = None
    escape_every: int | None = 25
    exclude: tuple[str, ...] = ()
    tracks: tuple[str, ...] = TRACKS


REFERENCE = Variant("reference")
RESOURCE_OPERATORS = ("resource_swap", "repair_resource_clash")
VARIANTS = {
    v.name: v
    for v in (
        REFERENCE,
        replace(REFERENCE, name="random", selector="random"),
        replace(REFERENCE, name="epsilon-greedy", selector="epsilon-greedy"),
        replace(REFERENCE, name="ucb-2state", selector="ucb-2state"),
        replace(REFERENCE, name="naive-start", construct="naive"),
        replace(
            REFERENCE,
            name="no-goal",
            exclude=("event_block_swap", "reassign_resource_times"),
        ),
        replace(REFERENCE, name="no-block-swap", exclude=("event_block_swap",)),
        replace(REFERENCE, name="no-rrt", exclude=("reassign_resource_times",)),
        replace(REFERENCE, name="rrt-selector", escape_every=None),
        replace(REFERENCE, name="no-perturbations", perturbations=False),
        replace(REFERENCE, name="fixed-idle", idle_limit=3000),
        replace(REFERENCE, name="legacy-perturbations", idle_limit=3000, reheat="full"),
        replace(
            REFERENCE,
            name="no-resource-ops",
            exclude=RESOURCE_OPERATORS,
            tracks=("resource",),
        ),
        # Resources stay as the constructor chose them; only times are searched.
        replace(
            REFERENCE,
            name="no-resource-moves",
            exclude=("resource_reassign", *RESOURCE_OPERATORS),
            tracks=("resource",),
        ),
    )
}
# The campaign studies the UCB selector only; random and epsilon-greedy stay
# selectable with --variants for a selector comparison.
NON_UCB_VARIANTS = ("random", "epsilon-greedy")
DEFAULT_VARIANTS = {
    "time": [
        v
        for v in VARIANTS
        if v not in NON_UCB_VARIANTS and "time" in VARIANTS[v].tracks
    ],
    "resource": [
        "reference",
        "ucb-2state",
        "naive-start",
        "no-goal",
        "no-resource-ops",
        "no-resource-moves",
    ],
}


@dataclass(frozen=True)
class Job:
    instance_id: str
    variant: Variant
    seed: int
    iterations: int
    track: str = "time"

    @property
    def key(self) -> str:
        return (
            f"{self.instance_id}_{self.variant.name}"
            f"_it{self.iterations}_seed{self.seed}"
        )


def _per_track[T](
    value: Sequence[T] | Mapping[str, Sequence[T]], track: str
) -> Sequence[T]:
    return value[track] if isinstance(value, Mapping) else value


def make_jobs(
    instances: Sequence[str],
    variants: Sequence[str] | Mapping[str, Sequence[str]],
    seeds: Sequence[int],
    budgets: Sequence[int] | Mapping[str, Sequence[int]],
    order_seed: int,
    tracks: Mapping[str, str] | None = None,
) -> list[Job]:
    """`tracks` maps instance -> track (default: all "time"). `variants` and
    `budgets` may be given per track; a variant is skipped on tracks it does
    not apply to (see Variant.tracks)."""
    jobs = []
    for instance in instances:
        track = tracks[instance] if tracks else "time"
        for variant in _per_track(variants, track):
            if track not in VARIANTS[variant].tracks:
                continue
            for seed in seeds:
                for budget in _per_track(budgets, track):
                    jobs.append(Job(instance, VARIANTS[variant], seed, budget, track))
    random.Random(order_seed).shuffle(jobs)
    return jobs


def source_identity() -> dict[str, str]:
    """Hash actual solver sources, including uncommitted edits."""
    paths = [
        ROOT / "experiments.py",
        Path(__file__),
        ROOT / "pyproject.toml",
        ROOT / "uv.lock",
        ROOT / ".python-version",
        DEFAULT_ARCHIVE,
        *sorted((ROOT / "src").rglob("*.py")),
    ]
    return {
        str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in paths
    }


def atomic_json(path: Path, data: object) -> None:
    temporary = path.with_suffix(".tmp")
    with temporary.open("w", encoding="utf-8") as stream:
        json.dump(data, stream, ensure_ascii=False, indent=2, allow_nan=False)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
    temporary.replace(path)


@contextmanager
def campaign_lock(directory: Path) -> Iterator[None]:
    directory.mkdir(parents=True, exist_ok=True)
    with (directory / ".lock").open("a", encoding="utf-8") as stream:
        try:
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise RuntimeError(
                "Ten katalog jest uzywany przez inna kampanie"
            ) from error
        try:
            yield
        finally:
            fcntl.flock(stream, fcntl.LOCK_UN)


def prepare_manifest(directory: Path, specification: dict[str, Any]) -> None:
    specification = json.loads(json.dumps(specification))
    path = directory / "manifest.json"
    if path.exists():
        existing = json.loads(path.read_text(encoding="utf-8"))
        if existing["specification"] != specification:
            raise ValueError(
                "Zmiana kodu, Pythona lub konfiguracji: uzyj nowego katalogu wynikow"
            )
        return
    if any((directory / "runs").glob("*")):
        raise ValueError("Katalog zawiera przebiegi bez manifestu")
    commit = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    atomic_json(
        path,
        {
            "specification": specification,
            "environment": {
                "git_commit": commit,
                "platform": platform.platform(),
                "cpu_count": os.cpu_count(),
                "command": sys.argv,
            },
        },
    )


def pending_jobs(directory: Path, jobs: Sequence[Job], retry_failed: bool) -> list[Job]:
    pending = []
    for job in jobs:
        run_dir = directory / "runs" / job.key
        result_path = run_dir / "result.json"
        if result_path.exists():
            result = json.loads(result_path.read_text(encoding="utf-8"))
            if result["iterations_done"] != job.iterations:
                raise ValueError(f"Niepelny wynik: {result_path}")
            for name in ("solution.xml", "solver.jsonl"):
                if not (run_dir / name).is_file():
                    raise ValueError(f"Brak artefaktu: {run_dir / name}")
        elif retry_failed or not (run_dir / "error.json").exists():
            pending.append(job)
    return pending


@dataclass(frozen=True)
class Work:
    job: Job
    directory: Path
    sources: dict[str, str]
    verify_every: int
    hseval: bool


def execute(work: Work) -> tuple[str, bool]:
    """Persist failures as failures; the parent continues other independent runs."""
    job = work.job
    directory = work.directory / "runs" / job.key
    directory.mkdir(parents=True, exist_ok=True)
    try:
        if source_identity() != work.sources:
            raise RuntimeError("Kod lub archiwum zmienily sie podczas kampanii")
        result = _run_one(
            job.instance_id,
            job.variant.selector,
            "sa",
            job.seed,
            120.0,  # ignored by iteration-keyed SA
            directory / "solver.jsonl",
            construct=job.variant.construct,
            exclude_heuristics=job.variant.exclude,
            perturbations=job.variant.perturbations,
            escape_every=job.variant.escape_every,
            iteration_limit=job.iterations,
            idle_limit=job.variant.idle_limit,
            reheat=job.variant.reheat,
            solution_path=directory / "solution.xml",
            verify_every=work.verify_every,
        )
        if result.iterations_done != job.iterations:
            raise AssertionError("Solver did not exhaust the iteration budget")
        if work.hseval:
            from src.validator import KHEValidator

            validation = KHEValidator(backend="local").validate_file(
                directory / "solution.xml"
            )
            atomic_json(directory / "hseval.json", asdict(validation))
            if not validation.is_valid or validation.total_cost_lex != (
                result.infeasibility_after,
                result.objective_after,
            ):
                raise AssertionError(f"HSEval mismatch: {validation}")
        if source_identity() != work.sources:
            raise RuntimeError("Kod lub archiwum zmienily sie podczas przebiegu")
        atomic_json(directory / "result.json", asdict(result))
        (directory / "error.json").unlink(missing_ok=True)
        return job.key, True
    except Exception:
        error = traceback.format_exc()
        atomic_json(directory / "error.json", {"job": asdict(job), "traceback": error})
        return f"{job.key}\n{error}", False


def worker_entry(work: Work) -> None:
    _, success = execute(work)
    raise SystemExit(0 if success else 1)


def finish_worker(work: Work, process: BaseProcess) -> None:
    directory = work.directory / "runs" / work.job.key
    if process.exitcode == 0 and (directory / "result.json").is_file():
        print(f"OK {work.job.key}", flush=True)
        return
    directory.mkdir(parents=True, exist_ok=True)
    error_path = directory / "error.json"
    if not error_path.exists():
        atomic_json(
            error_path,
            {
                "job": asdict(work.job),
                "traceback": f"Worker zakonczony z kodem {process.exitcode}",
            },
        )
    print(
        f"BLAD {work.job.key}: {error_path.read_text(encoding='utf-8')}",
        file=sys.stderr,
        flush=True,
    )


def run_workers(work: Sequence[Work], workers: int) -> None:
    """Watch process sentinels so a killed worker cannot leave a lost future."""
    context = multiprocessing.get_context("spawn")
    active: dict[str, tuple[Work, BaseProcess]] = {}
    queue = iter(work)
    completed = 0
    try:
        while True:
            while len(active) < workers:
                item = next(queue, None)
                if item is None:
                    break
                spawned = context.Process(target=worker_entry, args=(item,))
                spawned.start()
                active[item.job.key] = (item, spawned)
            if not active:
                break
            ready = wait([process.sentinel for _, process in active.values()])
            for key, (item, process) in list(active.items()):
                if process.sentinel in ready:
                    process.join()
                    finish_worker(item, process)
                    process.close()
                    del active[key]
                    completed += 1
                    print(f"[{completed}/{len(work)}]", flush=True)
    finally:
        stop_workers([process for _, process in active.values()])


def stop_workers(processes: Sequence[BaseProcess]) -> None:
    for process in processes:
        if process.is_alive():
            process.terminate()
    for process in processes:
        process.join(timeout=5)
        if process.is_alive():
            process.kill()
            process.join()
        process.close()


def sign_test(wins: int, losses: int) -> float:
    """Exact two-sided paired sign test: lexicographic outcomes, ties omitted."""
    if wins < 0 or losses < 0:
        raise ValueError("Counts cannot be negative")
    n = wins + losses
    if n == 0:
        return 1.0
    tail = sum(math.comb(n, k) for k in range(min(wins, losses) + 1))
    return min(1.0, 2 * tail / (1 << n))


def holm_adjust(pvalues: Sequence[float]) -> list[float]:
    adjusted = [1.0] * len(pvalues)
    previous = 0.0
    for rank, index in enumerate(sorted(range(len(pvalues)), key=pvalues.__getitem__)):
        previous = max(previous, min(1.0, pvalues[index] * (len(pvalues) - rank)))
        adjusted[index] = previous
    return adjusted


def write_csv(path: Path, rows: Sequence[dict[str, Any]]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def read_diagnostics(directory: Path, job: Job, result: RunResult) -> dict[str, Any]:
    """Stream logs; never retain per-step or all-campaign logs in memory."""
    costs = [(0, result.infeasibility_before, result.objective_before)]
    usage: dict[str, dict[str, float]] = defaultdict(
        lambda: {"calls": 0, "better": 0, "equal": 0, "worse": 0, "new_best": 0}
    )
    outcomes = []
    with (directory / "solver.jsonl").open(encoding="utf-8") as stream:
        for line in stream:
            record = json.loads(line)
            if record["event"] == "new_best":
                costs.append(
                    (record["step"] + 1, record["infeasibility"], record["objective"])
                )
                usage[record["heuristic_id"]]["new_best"] += 1
            elif record["event"] == "selector_window":
                for name, counts in record["heuristics"].items():
                    totals = usage[name]
                    totals["calls"] += counts["calls"]
                    for change in ("better", "equal", "worse"):
                        totals[change] += counts[f"accepted_{change}"]
            elif record["event"] == "perturbation_outcome":
                outcomes.append(record)
    checkpoints = []
    index = 0
    for percent in range(0, 101, 10):
        step = job.iterations * percent // 100
        while index + 1 < len(costs) and costs[index + 1][0] <= step:
            index += 1
        checkpoints.append(
            {
                "percent": percent,
                "step": step,
                "infeasibility": costs[index][1],
                "objective": costs[index][2],
            }
        )
    return {"checkpoints": checkpoints, "usage": dict(usage), "outcomes": outcomes}


def comparisons(rows: Sequence[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[str, int, str], dict[int, tuple[int, int]]] = defaultdict(dict)
    track_of = {row["instance_id"]: row["track"] for row in rows}
    for row in rows:
        groups[row["instance_id"], row["budget"], row["variant"]][row["seed"]] = (
            row["infeasibility_after"],
            row["objective_after"],
        )
    tests: list[dict[str, Any]] = []
    for (instance, budget, variant), costs in sorted(groups.items()):
        if variant == "reference":
            continue
        reference = groups.get((instance, budget, "reference"), {})
        seeds = sorted(costs.keys() & reference.keys())
        wins = sum(costs[s] < reference[s] for s in seeds)
        losses = sum(costs[s] > reference[s] for s in seeds)
        tests.append(
            {
                "track": track_of[instance],
                "instance_id": instance,
                "budget": budget,
                "variant": variant,
                "pairs": len(seeds),
                "unpaired": len(costs.keys() ^ reference.keys()),
                "wins": wins,
                "ties": len(seeds) - wins - losses,
                "losses": losses,
                "p_sign": sign_test(wins, losses) if seeds else None,
            }
        )
    paired = [test for test in tests if test["pairs"]]
    adjusted = holm_adjust([test["p_sign"] for test in paired])
    for test, pvalue in zip(paired, adjusted, strict=True):
        test["p_holm"] = pvalue
    for test in tests:
        test.setdefault("p_holm", None)
    return tests


def summarize(rows: Sequence[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[str, int, str], list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        groups[row["instance_id"], row["budget"], row["variant"]].append(row)
    summaries = []
    for (instance, budget, variant), runs in sorted(groups.items()):
        feasible = [r for r in runs if r["infeasibility_after"] == 0]
        summaries.append(
            {
                "track": runs[0]["track"],
                "instance_id": instance,
                "budget": budget,
                "variant": variant,
                "runs": len(runs),
                "feasible": len(feasible),
                "median_hard": statistics.median(
                    r["infeasibility_after"] for r in runs
                ),
                "median_soft_feasible": statistics.median(
                    r["objective_after"] for r in feasible
                )
                if feasible
                else None,
                "median_elapsed_s": statistics.median(r["elapsed_s"] for r in runs),
            }
        )
    return summaries


def build_report(directory: Path, jobs: Sequence[Job]) -> int:
    rows: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    checkpoints: list[dict[str, Any]] = []
    usage: list[dict[str, Any]] = []
    perturbations: list[dict[str, Any]] = []
    for job in jobs:
        run_dir = directory / "runs" / job.key
        metadata = {
            "track": job.track,
            "instance_id": job.instance_id,
            "budget": job.iterations,
            "variant": job.variant.name,
            "seed": job.seed,
        }
        if not (run_dir / "result.json").exists():
            failures.append(
                {
                    **metadata,
                    "status": "failed"
                    if (run_dir / "error.json").exists()
                    else "pending",
                }
            )
            continue
        result = RunResult(**json.loads((run_dir / "result.json").read_text()))
        rows.append({**metadata, **asdict(result)})
        diagnostics = read_diagnostics(run_dir, job, result)
        checkpoints.extend({**metadata, **c} for c in diagnostics["checkpoints"])
        usage.extend(
            {**metadata, "heuristic": h, **counts}
            for h, counts in diagnostics["usage"].items()
        )
        perturbations.extend({**metadata, **p} for p in diagnostics["outcomes"])
    report = directory / "report"
    report.mkdir(exist_ok=True)
    for name, data in (
        ("runs", rows),
        ("incomplete", failures),
        ("summary", summarize(rows)),
        ("comparisons", comparisons(rows)),
        ("convergence", checkpoints),
        ("heuristics", usage),
        ("perturbations", perturbations),
    ):
        write_csv(report / f"{name}.csv", data)
    plot_convergence(report, checkpoints)
    (report / "README.txt").write_text(
        f"Ukonczono {len(rows)}/{len(jobs)} przebiegow. "
        f"Nieukonczone: {len(failures)}.\n"
        "track: time = tylko czasy startu, resource = solver przypisuje tez "
        "zasoby (osobna sciezka: inna przestrzen przeszukiwania i koszt "
        "iteracji, nie lacz wynikow obu sciezek).\n"
        "summary.csv: koszt miekki tylko wsrod planow wykonalnych "
        "(warunkowa mediana).\n"
        "comparisons.csv: wariant vs reference na wspolnych seedach; wygrana = "
        "mniejsza para (hard, soft), bez splaszczania kosztu.\n"
        "p_sign: dwustronny dokladny test znakow, remisy pominiete; "
        "p_holm: korekta Holma przez wszystkie porownania tego raportu.\n"
        "Brak istotnosci nie dowodzi rownowaznosci. Nie interpretuj niepelnej "
        "kampanii jako wyniku koncowego. Potwierdz hipotezy na nowych seedach.\n"
        "PNG: mediana kosztu twardego i odsetek wykonalnych planow w funkcji "
        "wykorzystanego budzetu. Krotszy budzet ma inny harmonogram SA.\n",
        encoding="utf-8",
    )
    return len(failures)


def plot_convergence(report: Path, checkpoints: Sequence[dict[str, Any]]) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    groups: dict[tuple[str, int, str, int], list[int]] = defaultdict(list)
    for row in checkpoints:
        groups[
            row["instance_id"], row["budget"], row["variant"], row["percent"]
        ].append(row["infeasibility"])
    for instance, budget in sorted({(k[0], k[1]) for k in groups}):
        figure, axes = plt.subplots(1, 2, figsize=(13, 5))
        variants = sorted({k[2] for k in groups if k[:2] == (instance, budget)})
        for variant in variants:
            percentages = list(range(0, 101, 10))
            costs = [groups[instance, budget, variant, p] for p in percentages]
            axes[0].plot(
                percentages, [statistics.median(c) for c in costs], label=variant
            )
            axes[1].plot(
                percentages,
                [sum(x == 0 for x in c) / len(c) for c in costs],
                label=variant,
            )
        axes[0].set_ylabel("Mediana naruszen twardych")
        axes[0].set_yscale("symlog", linthresh=1)
        axes[1].set_ylabel("Odsetek planow wykonalnych")
        axes[1].set_ylim(-0.02, 1.02)
        for axis in axes:
            axis.set_xlabel("Wykorzystany budzet iteracji [%]")
            axis.grid(alpha=0.3)
        axes[1].legend(fontsize=7)
        figure.suptitle(f"{instance}: {budget:,} iteracji")
        figure.tight_layout()
        figure.savefig(report / f"{instance}_it{budget}.png", dpi=150)
        plt.close(figure)


def positive(value: str) -> int:
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError("Wartosc musi byc dodatnia")
    return number


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Odtwarzalna kampania ablacyjna; wznowienie ta sama komenda."
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--group",
        choices=(*TRACKS, "all"),
        default="all",
        help="domyslne instancje: time (tylko czasy), resource (przypisywanie "
        "zasobow) albo all; ignorowane przy --instances",
    )
    parser.add_argument("--instances", nargs="+")
    parser.add_argument(
        "--variants",
        nargs="+",
        choices=VARIANTS,
        help="domyslnie zestaw per sciezka, tylko selektor UCB",
    )
    parser.add_argument("--seeds", nargs="+", type=int, default=list(range(10)))
    parser.add_argument(
        "--budgets",
        nargs="+",
        type=positive,
        help="domyslnie per sciezka (TRACK_BUDGETS)",
    )
    parser.add_argument("--workers", type=positive, default=4)
    parser.add_argument("--order-seed", type=int, default=20261101)
    parser.add_argument("--verify-every", type=positive, default=1000)
    parser.add_argument("--hseval", action="store_true")
    parser.add_argument("--retry-failed", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--report-only", action="store_true")
    args = parser.parse_args(argv)
    if args.instances is None:
        args.instances = [
            instance
            for track in TRACKS
            if args.group in (track, "all")
            for instance in TRACK_INSTANCES[track]
        ]
    for name in ("instances", "variants", "seeds", "budgets"):
        values = getattr(args, name)
        if values is not None and len(set(values)) != len(values):
            parser.error(f"--{name}: powtorzone wartosci")
    if args.variants is not None and "reference" not in args.variants:
        parser.error("--variants musi zawierac reference do porownan sparowanych")
    return args


def specification(args: argparse.Namespace) -> tuple[dict[str, Any], list[Job]]:
    from src.parser import parse_archive
    from src.resource_domains import has_resource_choices

    instances = {
        i.id: i for i in parse_archive(DEFAULT_ARCHIVE.read_text(encoding="utf-8-sig"))
    }
    unknown = set(args.instances) - instances.keys()
    if unknown:
        raise ValueError(f"Nieznane instancje: {sorted(unknown)}")
    tracks = {
        name: "resource" if has_resource_choices(instances[name]) else "time"
        for name in args.instances
    }
    for name, track in tracks.items():
        if track == "resource" and any(
            c.type == "LinkEventsConstraint" for c in instances[name].constraints
        ):
            raise ValueError(f"{name}: LinkEvents poza zakresem (zob. CLAUDE.md)")
    jobs = make_jobs(
        args.instances,
        args.variants or DEFAULT_VARIANTS,
        args.seeds,
        args.budgets or TRACK_BUDGETS,
        args.order_seed,
        tracks,
    )
    profiles = {}
    for name in args.instances:
        instance = instances[name]
        profiles[name] = {
            "track": tracks[name],
            "events": len(instance.events),
            "resources": len(instance.resources),
            "times": len(instance.times),
            "unassigned_resource_slots": sum(
                r.resource_ref is None for e in instance.events for r in e.resources
            ),
            "multi_duration_events": sum(e.duration > 1 for e in instance.events),
            "constraint_types": sorted({c.type for c in instance.constraints}),
        }
    hseval_identity = None
    if args.hseval:
        from src.validator import KHEValidator

        binary = KHEValidator(backend="local").hseval_path
        if not binary.is_file() or not os.access(binary, os.X_OK):
            raise ValueError("Brak lokalnego HSEval. Zbuduj go i ustaw KHE_HSEVAL_PATH")
        hseval_identity = hashlib.sha256(binary.read_bytes()).hexdigest()
    return (
        {
            "schema": 2,
            "sources": source_identity(),
            "python": platform.python_version(),
            "jobs": [asdict(job) for job in jobs],
            "profiles": profiles,
            "verify_every": args.verify_every,
            "hseval": args.hseval,
            "hseval_sha256": hseval_identity,
        },
        jobs,
    )


def run_campaign(args: argparse.Namespace) -> int:
    spec, jobs = specification(args)
    print(
        f"{len(jobs)} przebiegow, {sum(j.iterations for j in jobs):,} iteracji, "
        f"{args.workers} procesy. SA sterowane iteracjami.",
        flush=True,
    )
    for track in TRACKS:
        track_jobs = [j for j in jobs if j.track == track]
        if track_jobs:
            print(
                f"  sciezka {track}: {len(track_jobs)} przebiegow, "
                f"{sum(j.iterations for j in track_jobs):,} iteracji",
                flush=True,
            )
    if args.dry_run:
        return 0
    with campaign_lock(args.output):
        prepare_manifest(args.output, spec)
        if args.report_only:
            return int(build_report(args.output, jobs) > 0)
        pending = pending_jobs(args.output, jobs, args.retry_failed)
        print(f"Pozostalo: {len(pending)}. Wyniki: {args.output}", flush=True)
        work = [
            Work(j, args.output, spec["sources"], args.verify_every, args.hseval)
            for j in pending
        ]
        run_workers(work, args.workers)
        incomplete = build_report(args.output, jobs)
        print(
            f"Raport: {args.output / 'report'}; nieukonczone: {incomplete}", flush=True
        )
        return int(incomplete > 0)


def interrupt(_signal: int, _frame: FrameType | None) -> None:
    raise KeyboardInterrupt


def main() -> None:
    signal.signal(signal.SIGTERM, interrupt)
    try:
        status = run_campaign(parse_args())
    except KeyboardInterrupt:
        print("\nPrzerwano. Uzyj tej samej komendy, aby wznowic.", file=sys.stderr)
        status = 130
    except (ValueError, RuntimeError, OSError) as error:
        print(f"BLAD: {error}", file=sys.stderr)
        status = 1
    raise SystemExit(status)


if __name__ == "__main__":
    main()
