#!/usr/bin/env python
"""Benchmark: full evaluation (evaluate_cost from scratch) vs delta_cost
(incremental) on a real instance -- Etap 3 DoD ("benchmark pokazujacy
przyspieszenie"). Diagnostic script for the thesis chapter, not a test: no
assertions, just timings printed to stdout."""

import random
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.resolve()))

from xhstt_core.construct import build_initial
from xhstt_core.cost import Cost, evaluate_cost
from xhstt_core.delta import delta_cost
from xhstt_core.heuristics import MANUAL_HEURISTICS
from xhstt_core.model import Instance, Solution
from xhstt_core.parser import parse_archive

ARCHIVE = Path(__file__).parent.parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
INSTANCE_ID = "AU-BG-98"
N_MOVES = 2000


def _load_instance() -> Instance:
    instances = parse_archive(ARCHIVE.read_text(encoding="utf-8-sig"))
    instance = next((i for i in instances if i.id == INSTANCE_ID), None)
    if instance is None:
        raise SystemExit(f"Nie znaleziono instancji {INSTANCE_ID!r} w {ARCHIVE}")
    return instance


def _generate_move_chain(instance: Instance, n: int, seed: int) -> list[Solution]:
    rng = random.Random(seed)
    solutions = [build_initial(instance, rng)]
    while len(solutions) <= n:
        heuristic = rng.choice(MANUAL_HEURISTICS)
        try:
            solutions.append(heuristic.apply(solutions[-1], instance, rng))
        except ValueError:
            continue
    return solutions


def _time_full_evaluation(instance: Instance, solutions: list[Solution]) -> float:
    start = time.perf_counter()
    for solution in solutions[1:]:
        evaluate_cost(instance, solution)
    return time.perf_counter() - start


def _time_delta_evaluation(
    instance: Instance, solutions: list[Solution], costs: list[Cost]
) -> float:
    start = time.perf_counter()
    for i in range(1, len(solutions)):
        delta_cost(instance, solutions[i - 1], costs[i - 1], solutions[i])
    return time.perf_counter() - start


def main() -> None:
    print(f"Wczytywanie {INSTANCE_ID} z {ARCHIVE.name}...")
    instance = _load_instance()
    print(
        f"  Zdarzenia={len(instance.events)}  Czasy={len(instance.times)}  "
        f"Zasoby={len(instance.resources)}  Ograniczenia={len(instance.constraints)}\n"
    )

    print(f"Generowanie lancucha {N_MOVES} losowych ruchow...")
    solutions = _generate_move_chain(instance, N_MOVES, seed=0)
    costs = [evaluate_cost(instance, s) for s in solutions]

    print("Mierzenie pelnej ewaluacji (evaluate_cost od zera kazdorazowo)...")
    full_time = _time_full_evaluation(instance, solutions)

    print("Mierzenie ewaluacji przyrostowej (delta_cost)...")
    delta_time = _time_delta_evaluation(instance, solutions, costs)

    print(
        f"\nPelna ewaluacja:       {full_time:.3f}s  "
        f"({len(solutions) / full_time:.0f} it/s)"
    )
    print(
        f"Ewaluacja przyrostowa: {delta_time:.3f}s  "
        f"({len(solutions) / delta_time:.0f} it/s)"
    )
    print(f"Przyspieszenie: {full_time / delta_time:.1f}x")


if __name__ == "__main__":
    main()
