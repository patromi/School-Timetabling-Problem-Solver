"""CLI: uruchamia solver LAHC na instancji XHSTT i pokazuje wynik.

Przyklady:
    python run_solver.py --list
    python run_solver.py AU-BG-98
    python run_solver.py BR-SA-00 --iterations 50000 --seed 1
"""

import argparse
import functools
import io
import random
import sys
import time
from dataclasses import dataclass
from pathlib import Path

from src.construct import build_initial
from src.evaluator_ref import evaluate_cost_components, resolve_occurrences
from src.html_report import render_timetable_page
from src.lahc import run_lahc
from src.model import Instance, Solution, SolutionGroup
from src.parser import parse_archive
from src.xml_writer import (
    extract_instance_archive,
    render_archive_with_solution_groups,
)

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"


@dataclass
class SolveResult:
    best: Solution
    infeasibility_before: int
    objective_before: int
    infeasibility_after: int
    objective_after: int
    elapsed_seconds: float
    iterations: int


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
) -> SolveResult:
    """Buduje rozwiazanie poczatkowe i uruchamia LAHC; zwraca spakowany wynik."""
    initial = build_initial(instance, rng)
    infeasibility_0, objective_0 = evaluate_cost_components(instance, initial)
    print(f"  Rozwiazanie poczatkowe -> infeasibility={infeasibility_0}  objective={objective_0}\n")

    t_start = time.time()
    on_progress = functools.partial(_report_progress, total=iterations, t_start=t_start)

    # progress_seconds gives a live heartbeat every ~2s regardless of
    # instance size -- large/slow instances can drop to a few it/s, making
    # a purely iteration-count trigger mean minutes of silence.
    best, _ = run_lahc(
        instance,
        initial,
        rng,
        history_length=history_length,
        max_iterations=iterations,
        on_progress=on_progress,
        progress_every=max(1, iterations // 20),
        progress_seconds=2.0,
        evaluation=evaluation,
    )
    elapsed = time.time() - t_start
    infeasibility_1, objective_1 = evaluate_cost_components(instance, best)

    return SolveResult(
        best=best,
        infeasibility_before=infeasibility_0,
        objective_before=objective_0,
        infeasibility_after=infeasibility_1,
        objective_after=objective_1,
        elapsed_seconds=elapsed,
        iterations=iterations,
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
    parser.add_argument("--iterations", type=int, default=30_000)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--history", type=int, default=30)
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

    print(
        f"LAHC: {args.iterations} iteracji, seed={args.seed}, "
        f"history={args.history}, ewaluacja={args.evaluation}"
    )
    result = solve(
        instance,
        rng=random.Random(args.seed),
        iterations=args.iterations,
        history_length=args.history,
        evaluation=args.evaluation,
    )

    rate = result.iterations / result.elapsed_seconds
    print(f"\nZakonczono w {result.elapsed_seconds:.1f}s ({rate:.0f} it/s)")
    print(f"  Przed:  infeasibility={result.infeasibility_before:>6}  objective={result.objective_before:>6}")
    print(f"  Po:     infeasibility={result.infeasibility_after:>6}  objective={result.objective_after:>6}")
    print(
        f"  Zmiana: infeasibility={result.infeasibility_after - result.infeasibility_before:+d}  "
        f"objective={result.objective_after - result.objective_before:+d}"
    )

    output_path = args.output or Path("output") / f"{instance.id}_solution.xml"
    xml_path, html_path = write_outputs(instance, archive_text, result, output_path, args.seed)
    print(f"\nRozwiazanie zapisane do: {xml_path}")
    print(f"Plan zajec (HTML) zapisany do: {html_path}")


if __name__ == "__main__":
    main()
