#!/usr/bin/env python
"""Benchmark: full evaluation vs incremental (delta) evaluation -- Etap 3 DoD
("benchmark pokazujacy przyspieszenie"). Diagnostic script for the thesis
chapter, not a test: no assertions, just timings printed to stdout.

Three sections, because the delta pays off in three different places and by
very different factors:

1. Scoring one candidate (the LAHC acceptance test).
2. The three heuristics that score many candidates internally
   (move_best/repair_hard_violation/ruin_and_recreate used to pay one FULL
   evaluation per candidate time).
3. End-to-end LAHC throughput, which is the number that actually decides
   whether the incremental path is worth defaulting to.

Every measurement is the median of several repeats: an earlier run of this
benchmark was misread because single-run noise on this machine exceeded 20%.
"""

import random
import statistics
import sys
import time
from collections.abc import Callable
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.resolve()))

from src.construct import build_initial
from src.cost import evaluate_cost
from src.heuristics import MANUAL_HEURISTICS
from src.incremental import IncrementalEvaluator
from src.lahc import run_lahc
from src.model import Instance, Solution
from src.parser import parse_archive

ARCHIVE = Path(__file__).parent.parent / "data" / "xhstt2014" / "XHSTT-2014.xml"
INSTANCE_ID = "AU-BG-98"
LOCAL_HEURISTIC_IDS = {"move_random", "swap", "resource_reassign", "kempe_chain"}
CANDIDATES = 200
HEURISTIC_REPEATS = 3
LAHC_ITERATIONS = 400
LAHC_REPEATS = 5


def _load_instance() -> Instance:
    instances = parse_archive(ARCHIVE.read_text(encoding="utf-8-sig"))
    instance = next((i for i in instances if i.id == INSTANCE_ID), None)
    if instance is None:
        raise SystemExit(f"Nie znaleziono instancji {INSTANCE_ID!r} w {ARCHIVE}")
    return instance


def _median_seconds(run: Callable[[], object], repeats: int) -> float:
    timings = []
    for _ in range(repeats):
        start = time.perf_counter()
        run()
        timings.append(time.perf_counter() - start)
    return statistics.median(timings)


def _report(label: str, full: float, incremental: float, unit: str) -> None:
    print(f"\n=== {label} ===")
    print(f"  Pelna ewaluacja:       {full:8.3f}s ({unit})")
    print(f"  Ewaluacja przyrostowa: {incremental:8.3f}s ({unit})")
    print(f"  Przyspieszenie:        {full / incremental:8.1f}x")


def _benchmark_candidate_scoring(instance: Instance) -> None:
    rng = random.Random(0)
    solution = build_initial(instance, rng)
    pool = [h for h in MANUAL_HEURISTICS if h.id in LOCAL_HEURISTIC_IDS]
    candidates = []
    while len(candidates) < CANDIDATES:
        try:
            candidates.append(rng.choice(pool).apply(solution, instance, rng))
        except ValueError:
            continue

    evaluator = IncrementalEvaluator(instance, solution)
    full = _median_seconds(
        lambda: [evaluate_cost(instance, c) for c in candidates], HEURISTIC_REPEATS
    )
    incremental = _median_seconds(
        lambda: [evaluator.probe(c) for c in candidates], HEURISTIC_REPEATS
    )
    _report(
        f"Punktowanie {CANDIDATES} kandydatow (ruchy lokalne)",
        full,
        incremental,
        f"{CANDIDATES} kandydatow",
    )


def _benchmark_lahc(instance: Instance, initial: Solution) -> None:
    def run(mode: str) -> Callable[[], object]:
        return lambda: run_lahc(
            instance,
            initial,
            random.Random(1),
            max_iterations=LAHC_ITERATIONS,
            evaluation=mode,
        )

    full = _median_seconds(run("full"), LAHC_REPEATS)
    incremental = _median_seconds(run("incremental"), LAHC_REPEATS)
    _report(
        f"LAHC end-to-end ({LAHC_ITERATIONS} iteracji, pelna pula)",
        full,
        incremental,
        f"{LAHC_ITERATIONS / full:.0f} vs {LAHC_ITERATIONS / incremental:.0f} it/s",
    )


def main() -> None:
    print(f"Wczytywanie {INSTANCE_ID} z {ARCHIVE.name}...")
    instance = _load_instance()
    print(
        f"  Zdarzenia={len(instance.events)}  Czasy={len(instance.times)}  "
        f"Zasoby={len(instance.resources)}  Ograniczenia={len(instance.constraints)}"
    )
    initial = build_initial(instance, random.Random(0))

    _benchmark_candidate_scoring(instance)
    _benchmark_lahc(instance, initial)


if __name__ == "__main__":
    main()
