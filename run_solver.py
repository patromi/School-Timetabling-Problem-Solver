"""CLI: uruchamia solver LAHC na instancji XHSTT i pokazuje wynik.

Przyklady:
    python run_solver.py --list
    python run_solver.py AU-BG-98
    python run_solver.py BR-SA-00 --iterations 50000 --seed 1
"""

import argparse
import functools
import io
import os
import platform
import random
import subprocess
import sys
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path

from src.acceptance import AcceptanceCriterion, AutoSAAcceptance, LAHCAcceptance
from src.construct import build_greedy, build_initial
from src.evaluator_ref import evaluate_constraint_costs, resolve_occurrences
from src.html_report import render_timetable_page
from src.lahc import RunStats, run_lahc
from src.selectors import EpsilonGreedySelector, RandomSelector, UCBSelector
from src.solve_log import SolveLogger
from src.model import Instance, Solution, SolutionGroup
from src.parser import parse_archive
from src.xml_writer import (
    extract_instance_archive,
    render_archive_with_solution_groups,
)

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"

# Iteracje bez nowego najlepszego wyniku przed perturbacja -- ta sama wartosc
# co STAGNATION_LIMIT w experiments.py. Podawana jawnie, bo domyslny prog
# run_lahc (max_iterations // 20) przy --time-limit z duzym --iterations
# rosl do milionow krokow i perturbacje nigdy sie nie uruchamialy.
DEFAULT_IDLE_LIMIT = 3000


@dataclass
class SolveResult:
    best: Solution
    infeasibility_before: int
    objective_before: int
    infeasibility_after: int
    objective_after: int
    elapsed_seconds: float
    iterations: int
    stats: RunStats = field(default_factory=RunStats)
    construct_seconds: float = 0.0
    cost_by_type_before: dict[str, dict[str, int]] = field(default_factory=dict)
    cost_by_type_after: dict[str, dict[str, int]] = field(default_factory=dict)


CostByType = dict[str, dict[str, int]]


def cost_by_type(instance: Instance, solution: Solution) -> CostByType:
    """Koszt rozbity na typy ograniczen, osobno twarde ("hard") i miekkie
    ("soft"); pomija typy z zerowym kosztem. Sumy obu czesci to para
    (infeasibility, objective)."""
    out: CostByType = {"hard": {}, "soft": {}}
    costs = evaluate_constraint_costs(instance, solution)
    for c, cost in zip(instance.constraints, costs, strict=True):
        if cost:
            part = out["hard" if c.required else "soft"]
            part[c.type] = part.get(c.type, 0) + cost
    return out


def run_environment(argv: list[str] | None) -> dict[str, object]:
    """Wersja kodu i srodowisko przebiegu -- to, czego trzeba do odtworzenia
    wyniku. Brak gita nie przerywa przebiegu (pola sa wtedy None)."""
    repo = Path(__file__).parent

    def git(*args: str) -> str | None:
        try:
            out = subprocess.run(
                ["git", *args], cwd=repo, capture_output=True, text=True, timeout=10
            )
        except (OSError, subprocess.SubprocessError):
            return None
        return out.stdout.strip() if out.returncode == 0 else None

    status = git("status", "--porcelain", "--untracked-files=no")
    return {
        "git_commit": git("rev-parse", "HEAD"),
        "git_branch": git("rev-parse", "--abbrev-ref", "HEAD"),
        "git_dirty": None if status is None else bool(status),
        "argv": list(sys.argv[1:] if argv is None else argv),
        "python": platform.python_version(),
        "platform": platform.platform(),
        "cpu_count": os.cpu_count(),
    }


def load_archive(path: Path) -> tuple[str, list[Instance]]:
    """Zwraca (surowy XML, sparsowane instancje) — oba potrzebne dalej."""
    text = path.read_text(encoding="utf-8-sig")
    return text, parse_archive(text)


def find_instance(instances: list[Instance], instance_id: str) -> Instance:
    for inst in instances:
        if inst.id == instance_id:
            return inst
    available = ", ".join(sorted(i.id for i in instances))
    raise SystemExit(f"Nieznana instancja {instance_id!r}.\nDostepne: {available}")


def format_instance_table(instances: list[Instance]) -> str:
    header = (
        f"{'Id':10} {'Nazwa':22} {'Zdarzenia':>10} {'Czasy':>7} "
        f"{'Zasoby':>7} {'Ograniczenia':>13}"
    )
    lines = [header, "-" * len(header)]
    for inst in sorted(instances, key=lambda i: i.id):
        lines.append(
            f"{inst.id:10} {inst.name[:22]:22} {len(inst.events):>10} "
            f"{len(inst.times):>7} {len(inst.resources):>7} {len(inst.constraints):>13}"
        )
    return "\n".join(lines)


def _report_progress(
    iteration: int, best_cost: int, *, total: int, t_start: float
) -> None:
    elapsed = time.time() - t_start
    rate = iteration / elapsed if elapsed > 0 else 0.0
    remaining = (total - iteration) / rate if rate > 0 else float("inf")
    pct = 100 * iteration / total
    print(
        f"  [{iteration:>7}/{total} {pct:5.1f}%] koszt={best_cost:>14,}  "
        f"{rate:6.1f} it/s  pozostalo ~{remaining:.0f}s"
    )


def solve(
    instance: Instance,
    rng: random.Random,
    iterations: int,
    history_length: int,
    evaluation: str,
    selector: RandomSelector | EpsilonGreedySelector | UCBSelector,
    logger: SolveLogger | None = None,
    time_limit: float | None = None,
    acceptance: AcceptanceCriterion | None = None,
    construct: str = "greedy",
    idle_limit: int | None = DEFAULT_IDLE_LIMIT,
) -> SolveResult:
    """Buduje rozwiazanie poczatkowe i uruchamia solver; zwraca spakowany wynik."""
    build = build_greedy if construct == "greedy" else build_initial
    t_construct = time.time()
    initial = build(instance, rng)
    construct_seconds = time.time() - t_construct
    by_type_0 = cost_by_type(instance, initial)
    infeasibility_0 = sum(by_type_0["hard"].values())
    objective_0 = sum(by_type_0["soft"].values())
    print(f"  Rozwiazanie poczatkowe -> infeasibility={infeasibility_0}  objective={objective_0}\n")

    t_start = time.time()
    on_progress = functools.partial(_report_progress, total=iterations, t_start=t_start)
    stats = RunStats()

    best, _ = run_lahc(
        instance,
        initial,
        rng,
        history_length=history_length,
        selector=selector,
        max_iterations=iterations,
        max_seconds=time_limit,
        on_progress=on_progress,
        progress_every=max(1, iterations // 20),
        progress_seconds=2.0,
        evaluation=evaluation,
        logger=logger,
        acceptance=acceptance,
        idle_limit=idle_limit,
        stats=stats,
    )
    elapsed = time.time() - t_start
    by_type_1 = cost_by_type(instance, best)

    return SolveResult(
        best=best,
        infeasibility_before=infeasibility_0,
        objective_before=objective_0,
        infeasibility_after=sum(by_type_1["hard"].values()),
        objective_after=sum(by_type_1["soft"].values()),
        elapsed_seconds=elapsed,
        iterations=iterations,
        stats=stats,
        construct_seconds=construct_seconds,
        cost_by_type_before=by_type_0,
        cost_by_type_after=by_type_1,
    )


def write_outputs(
    instance: Instance,
    archive_text: str,
    result: SolveResult,
    output_path: Path,
    seed: int,
) -> tuple[Path, Path]:
    """Zapisuje XML rozwiazania i HTML planu; zwraca obie sciezki."""
    output_path.parent.mkdir(parents=True, exist_ok=True)

    single_instance_xml = extract_instance_archive(archive_text, instance.id)
    group = SolutionGroup(id=f"LAHC_{instance.id}_seed{seed}", solutions=[result.best])
    output_path.write_text(
        render_archive_with_solution_groups(single_instance_xml, [group]),
        encoding="utf-8",
    )

    html_path = output_path.with_suffix(".html").with_stem(
        f"{output_path.stem.removesuffix('_solution')}_timetable"
    )
    occurrences = resolve_occurrences(instance, result.best)
    html_path.write_text(
        render_timetable_page(instance, occurrences, result.infeasibility_after, result.objective_after),
        encoding="utf-8",
    )
    return output_path, html_path


def _parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Uruchamia solver LAHC na instancji XHSTT."
    )
    parser.add_argument(
        "instance_id",
        nargs="?",
        help="Id instancji (np. AU-BG-98). Pomin, by wypisac liste.",
    )
    parser.add_argument("--archive", type=Path, default=DEFAULT_ARCHIVE)
    parser.add_argument("--selector", type=str, choices=["random", "epsilon-greedy", "ucb"], default="ucb", help="Heuristic selector type (domyslnie ucb)")
    parser.add_argument("--acceptance", type=str, choices=["lahc", "sa"], default="sa",
                        help="Kryterium akceptacji: Simulated Annealing (domyslnie) lub Late Acceptance HC (opcja historyczna)")
    parser.add_argument("--iterations", type=int, default=30_000)
    parser.add_argument("--time-limit", type=float, default=None,
                        help="Max wall-clock seconds (overrides --iterations as a stopping criterion)")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument(
        "--construct",
        choices=["greedy", "naive"],
        default="greedy",
        help="Rozwiazanie poczatkowe: greedy (unika kolizji, domyslnie) lub naive (losowe)",
    )
    parser.add_argument("--history", type=int, default=30,
                        help="Dlugosc historii LAHC (ignorowane dla --acceptance sa)")
    parser.add_argument("--sa-t0-factor", type=float, default=0.01,
                        help="SA: temperatura startowa jako ulamek kosztu startowego "
                        "(domyslnie 0.01, dostrojone w issue #82)")
    parser.add_argument("--idle-limit", type=int, default=DEFAULT_IDLE_LIMIT,
                        help="Iteracje bez nowego najlepszego wyniku przed "
                        "perturbacja; tercje wyznaczaja stan stagnacji selektora "
                        f"(domyslnie {DEFAULT_IDLE_LIMIT}, jak w experiments.py)")
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Gdzie zapisac wynikowy XML (domyslnie output/<id>_solution.xml)",
    )
    parser.add_argument(
        "--list", action="store_true", help="Wypisz dostepne instancje i zakoncz."
    )
    parser.add_argument(
        "--evaluation",
        choices=["incremental", "full", "verify"],
        default="incremental",
        help=(
            "Sposob liczenia kosztu kandydatow: przyrostowo (domyslnie), "
            "pelna ewaluacja, albo obie z porownaniem (diagnostyka)."
        ),
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> None:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(line_buffering=True)
    args = _parse_args(argv)

    if not args.archive.exists():
        raise SystemExit(f"Nie znaleziono archiwum: {args.archive}")

    print(f"Wczytywanie {args.archive.name}...")
    archive_text, instances = load_archive(args.archive)
    print(f"  {len(instances)} instancji wczytanych\n")

    if args.list or (not args.instance_id and len(instances) != 1):
        print(format_instance_table(instances))
        if not args.instance_id:
            print("\nUzycie: python run_solver.py <ID_INSTANCJI> [--iterations N] [--seed N]")
        return

    instance_id = args.instance_id or instances[0].id
    instance = find_instance(instances, instance_id)
    print(f"Instancja: {instance.id} ({instance.name})")
    print(
        f"  Zdarzenia={len(instance.events)}  Czasy={len(instance.times)}  "
        f"Zasoby={len(instance.resources)}  Ograniczenia={len(instance.constraints)}\n"
    )

    if args.acceptance == "sa":
        # SA z temperatura startowa wzgledem kosztu startowego, jak w
        # eksperymentach. Z limitem czasu stygnie wedlug zegara, bez niego
        # wedlug numeru kroku -- wtedy przebieg jest powtarzalny z seeda.
        acceptance_obj: AcceptanceCriterion = (
            AutoSAAcceptance(t0_factor=args.sa_t0_factor, time_budget=args.time_limit)
            if args.time_limit is not None
            else AutoSAAcceptance(
                t0_factor=args.sa_t0_factor, iteration_budget=args.iterations
            )
        )
    else:
        acceptance_obj = LAHCAcceptance(history_length=args.history)

    print(
        f"Solver: {args.iterations} iteracji, seed={args.seed}, "
        f"akceptacja={acceptance_obj.name}, ewaluacja={args.evaluation}"
    )
    selector_obj: RandomSelector | EpsilonGreedySelector | UCBSelector
    if args.selector == "epsilon-greedy":
        selector_obj = EpsilonGreedySelector()
    elif args.selector == "ucb":
        selector_obj = UCBSelector()
    else:
        selector_obj = RandomSelector()

    output_path = args.output or Path("output") / f"{instance.id}_solution.xml"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    log_path = output_path.with_suffix(".jsonl")

    with open(log_path, "w", encoding="utf-8") as log_file:
        logger = SolveLogger(log_file)
        logger.log_run_environment(
            **run_environment(argv),
            # SA cooled by the clock (--time-limit) depends on machine speed;
            # cooled by the step number, a run is reproducible from its seed
            reproducible_from_seed=args.time_limit is None,
        )

        result = solve(
            instance,
            rng=random.Random(args.seed),
            iterations=args.iterations,
            history_length=args.history,
            selector=selector_obj,
            evaluation=args.evaluation,
            logger=logger,
            time_limit=args.time_limit,
            acceptance=acceptance_obj,
            construct=args.construct,
            idle_limit=args.idle_limit,
        )
        rate = (
            result.stats.steps_done / result.elapsed_seconds
            if result.elapsed_seconds > 0
            else 0.0
        )

        logger.log_run_summary(
            instance_id=instance.id,
            seed=args.seed,
            iterations=args.iterations,
            history_length=args.history,
            selector=args.selector,
            evaluation=args.evaluation,
            infeasibility_before=result.infeasibility_before,
            objective_before=result.objective_before,
            infeasibility_after=result.infeasibility_after,
            objective_after=result.objective_after,
            elapsed_seconds=result.elapsed_seconds,
            # `iterations` above is the budget; these are what the run did
            **asdict(result.stats),
            iterations_per_second=round(rate, 1),
            construct_seconds=round(result.construct_seconds, 3),
            cost_by_type_before=result.cost_by_type_before,
            cost_by_type_after=result.cost_by_type_after,
        )
        logger.flush()

    print(
        f"\nZakonczono w {result.elapsed_seconds:.1f}s "
        f"({result.stats.steps_done} iteracji, {rate:.0f} it/s)"
    )
    print(f"  Przed:  infeasibility={result.infeasibility_before:>6}  objective={result.objective_before:>6}")
    print(f"  Po:     infeasibility={result.infeasibility_after:>6}  objective={result.objective_after:>6}")
    print(
        f"  Zmiana: infeasibility={result.infeasibility_after - result.infeasibility_before:+d}  "
        f"objective={result.objective_after - result.objective_before:+d}"
    )

    xml_path, html_path = write_outputs(instance, archive_text, result, output_path, args.seed)
    print(f"\nRozwiazanie zapisane do: {xml_path}")
    print(f"Plan zajec (HTML) zapisany do: {html_path}")
    print(f"Log (JSONL) zapisany do: {log_path}")


if __name__ == "__main__":
    main()
