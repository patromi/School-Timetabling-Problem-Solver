"""CLI: uruchamia solver LAHC na instancji XHSTT i pokazuje wynik.

Przyklady:
    python run_solver.py --list
    python run_solver.py AU-BG-98
    python run_solver.py BR-SA-00 --iterations 50000 --seed 1
"""

import argparse
import random
import sys
import time
from pathlib import Path

from src.construct import build_initial
from src.evaluator_ref import evaluate_constraint, resolve_occurrences
from src.html_report import render_timetable_page
from src.lahc import run_lahc
from src.model import Instance, Solution, SolutionGroup
from src.parser import parse_archive
from src.xml_writer import (
    extract_instance_archive,
    render_archive_with_solution_groups,
)

DEFAULT_ARCHIVE = Path(__file__).parent / "data" / "xhstt2014" / "XHSTT-2014.xml"


def load_instances(archive_path: Path) -> list[Instance]:
    return parse_archive(archive_path.read_text(encoding="utf-8-sig"))


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


def cost_breakdown(instance: Instance, solution: Solution) -> tuple[int, int]:
    occurrences = resolve_occurrences(instance, solution)
    infeasibility = sum(
        evaluate_constraint(instance, occurrences, c)
        for c in instance.constraints
        if c.required
    )
    objective = sum(
        evaluate_constraint(instance, occurrences, c)
        for c in instance.constraints
        if not c.required
    )
    return infeasibility, objective


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
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> None:
    # Line-buffer stdout so progress lines show up immediately instead of
    # sitting in a full buffer (matters when output is redirected/piped,
    # or just for a responsive-feeling long run).
    sys.stdout.reconfigure(line_buffering=True)
    args = _parse_args(argv)

    if not args.archive.exists():
        raise SystemExit(f"Nie znaleziono archiwum: {args.archive}")

    print(f"Wczytywanie {args.archive.name}...")
    t0 = time.time()
    archive_text = args.archive.read_text(encoding="utf-8-sig")
    instances = parse_archive(archive_text)
    print(f"  {len(instances)} instancji wczytanych w {time.time() - t0:.2f}s\n")

    if args.list or (not args.instance_id and len(instances) != 1):
        print(format_instance_table(instances))
        if not args.instance_id:
            print(
                "\nUzycie: python run_solver.py <ID_INSTANCJI> [--iterations N] [--seed N]"
            )
        return

    instance_id = args.instance_id or instances[0].id
    instance = find_instance(instances, instance_id)
    print(f"Instancja: {instance.id} ({instance.name})")
    print(
        f"  Zdarzenia={len(instance.events)}  Czasy={len(instance.times)}  "
        f"Zasoby={len(instance.resources)}  Ograniczenia={len(instance.constraints)}\n"
    )

    rng = random.Random(args.seed)
    print("Budowanie rozwiazania poczatkowego...")
    t0 = time.time()
    initial = build_initial(instance, rng)
    infeasibility_0, objective_0 = cost_breakdown(instance, initial)
    print(
        f"  gotowe w {time.time() - t0:.2f}s -> "
        f"infeasibility={infeasibility_0}  objective={objective_0}\n"
    )

    print(
        f"Uruchamianie LAHC ({args.iterations} iteracji, seed={args.seed}, history={args.history})..."
    )
    t_start = time.time()

    def on_progress(iteration: int, best_cost: int) -> None:
        elapsed = time.time() - t_start
        rate = iteration / elapsed if elapsed > 0 else 0.0
        remaining = (args.iterations - iteration) / rate if rate > 0 else float("inf")
        pct = 100 * iteration / args.iterations
        print(
            f"  [{iteration:>7}/{args.iterations} {pct:5.1f}%] koszt={best_cost:>14,}  "
            f"{rate:6.1f} it/s  pozostalo ~{remaining:.0f}s"
        )

    # progress_seconds gives a live heartbeat every ~2s regardless of
    # instance size -- large/slow instances (hundreds of events, dozens of
    # constraints) can drop to a few iterations/s, where a purely
    # iteration-count-based trigger could mean minutes of silence.
    best, _ = run_lahc(
        instance,
        initial,
        rng,
        history_length=args.history,
        max_iterations=args.iterations,
        on_progress=on_progress,
        progress_every=max(1, args.iterations // 20),
        progress_seconds=2.0,
    )
    elapsed = time.time() - t_start
    infeasibility_1, objective_1 = cost_breakdown(instance, best)

    print(f"\nZakonczono w {elapsed:.1f}s ({args.iterations / elapsed:.0f} it/s)")
    print(f"  Przed:  infeasibility={infeasibility_0:>6}  objective={objective_0:>6}")
    print(f"  Po:     infeasibility={infeasibility_1:>6}  objective={objective_1:>6}")
    print(
        f"  Zmiana: infeasibility={infeasibility_1 - infeasibility_0:+d}  "
        f"objective={objective_1 - objective_0:+d}"
    )

    output_path = args.output or Path("output") / f"{instance.id}_solution.xml"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    single_instance_xml = extract_instance_archive(archive_text, instance.id)
    group = SolutionGroup(id=f"LAHC_{instance.id}_seed{args.seed}", solutions=[best])
    output_path.write_text(
        render_archive_with_solution_groups(single_instance_xml, [group]),
        encoding="utf-8",
    )
    print(f"\nRozwiazanie zapisane do: {output_path}")

    best_occurrences = resolve_occurrences(instance, best)
    html_path = output_path.with_suffix(".html").with_stem(
        f"{output_path.stem.removesuffix('_solution')}_timetable"
    )
    html_path.write_text(
        render_timetable_page(instance, best_occurrences, infeasibility_1, objective_1),
        encoding="utf-8",
    )
    print(f"Plan zajec (HTML) zapisany do: {html_path}")


if __name__ == "__main__":
    main()
