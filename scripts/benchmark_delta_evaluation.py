#!/usr/bin/env python
"""Benchmark: full evaluation (evaluate_cost from scratch) vs delta_cost
(incremental) on a real instance -- Etap 3 DoD ("benchmark pokazujacy
przyspieszenie"). Diagnostic script for the thesis chapter, not a test: no
assertions, just timings printed to stdout.

Reports two speedup ratios: one with the full MANUAL_HEURISTICS pool
(blended -- includes large_perturbation/ruin_and_recreate/move_best/
repair_hard_violation, which touch many events per move, so delta_cost has
little to skip), and one restricted to LOCAL_HEURISTIC_IDS (single- or
few-event moves -- the scenario delta_cost is designed for, matching
CLAUDE.md's Etap 3 wording "przesuniecie jednego zdarzenia"). Both numbers
are reported honestly, whatever they come out to."""

import random
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.resolve()))

from src.construct import build_initial
from src.cost import Cost, evaluate_cost
from src.delta import delta_cost
from src.heuristics import MANUAL_HEURISTICS, Heuristic
from src.model import Instance, Solution
from src.parser import parse_archive

ARCHIVE = Path(__file__).parent.parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
INSTANCE_ID = "AU-BG-98"
N_MOVES = 120
LOCAL_HEURISTIC_IDS = {"move_random", "swap", "resource_reassign", "kempe_chain"}


def _load_instance() -> Instance:
    instances = parse_archive(ARCHIVE.read_text(encoding="utf-8-sig"))
    instance = next((i for i in instances if i.id == INSTANCE_ID), None)
    if instance is None:
        raise SystemExit(f"Nie znaleziono instancji {INSTANCE_ID!r} w {ARCHIVE}")
    return instance


def _generate_move_chain(
    instance: Instance, heuristics: list[Heuristic], n: int, seed: int
) -> list[Solution]:
    rng = random.Random(seed)
    solutions = [build_initial(instance, rng)]
    while len(solutions) <= n:
        heuristic = rng.choice(heuristics)
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


def _run_benchmark(
    label: str, instance: Instance, heuristics: list[Heuristic], n: int, seed: int
) -> None:
    print(f"\n=== {label} ===")
    print(f"Generowanie lancucha {n} losowych ruchow...")
    solutions = _generate_move_chain(instance, heuristics, n, seed)
    costs = [evaluate_cost(instance, s) for s in solutions]

    print("Mierzenie pelnej ewaluacji (evaluate_cost od zera kazdorazowo)...")
    full_time = _time_full_evaluation(instance, solutions)

    print("Mierzenie ewaluacji przyrostowej (delta_cost)...")
    delta_time = _time_delta_evaluation(instance, solutions, costs)

    print(
        f"Pelna ewaluacja:       {full_time:.3f}s  "
        f"({(len(solutions) - 1) / full_time:.0f} it/s)"
    )
    print(
        f"Ewaluacja przyrostowa: {delta_time:.3f}s  "
        f"({(len(solutions) - 1) / delta_time:.0f} it/s)"
    )
    print(f"Przyspieszenie: {full_time / delta_time:.1f}x")


def main() -> None:
    print(f"Wczytywanie {INSTANCE_ID} z {ARCHIVE.name}...")
    instance = _load_instance()
    print(
        f"  Zdarzenia={len(instance.events)}  Czasy={len(instance.times)}  "
        f"Zasoby={len(instance.resources)}  Ograniczenia={len(instance.constraints)}"
    )

    local_heuristics = [h for h in MANUAL_HEURISTICS if h.id in LOCAL_HEURISTIC_IDS]

    _run_benchmark(
        "Pelna pula (8 heurystyk z MANUAL_HEURISTICS)",
        instance,
        MANUAL_HEURISTICS,
        N_MOVES,
        seed=0,
    )
    _run_benchmark(
        "Tylko ruchy lokalne (move_random, swap, resource_reassign, kempe_chain)",
        instance,
        local_heuristics,
        N_MOVES,
        seed=0,
    )


if __name__ == "__main__":
    main()
