import random
import time
from collections.abc import Callable

from src.cost import evaluate_cost
from src.heuristics import MANUAL_HEURISTICS, Heuristic
from src.incremental import (
    IncrementalEvaluator,
    StructuralChangeError,
    Transaction,
)
from src.model import Instance, Solution

FULL = "full"
INCREMENTAL = "incremental"
VERIFY = "verify"


def _score_candidate(
    instance: Instance,
    evaluator: IncrementalEvaluator | None,
    heuristic: Heuristic,
    candidate: Solution,
    evaluation: str,
    check: bool,
) -> tuple[int, Transaction | None]:
    """Cost of `candidate` plus the transaction that has to be finished for
    it, or None when it was costed by full evaluation instead."""
    transaction: Transaction | None = None
    if evaluator is not None and heuristic.incremental_safe:
        try:
            transaction = evaluator.evaluate(candidate)
        except StructuralChangeError:
            transaction = None
    if transaction is None:
        return evaluate_cost(instance, candidate).as_scalar(), None

    cost = transaction.cost.as_scalar()
    if evaluation == VERIFY and check:
        expected = evaluate_cost(instance, candidate).as_scalar()
        if cost != expected:
            raise AssertionError(
                f"incremental cost {cost} != full cost {expected} "
                f"after heuristic {heuristic.id!r}"
            )
    return cost, transaction


def _finish(
    evaluator: IncrementalEvaluator | None,
    transaction: Transaction | None,
    accepted: bool,
    current: Solution,
) -> None:
    if evaluator is None:
        return
    if transaction is not None:
        if accepted:
            evaluator.accept(transaction)
        else:
            evaluator.rollback(transaction)
    elif accepted:
        # A heuristic that isn't incrementally safe (or that changed the
        # solution's shape) invalidates every cached aggregate.
        evaluator.rebuild(current)


def run_lahc(
    instance: Instance,
    initial: Solution,
    rng: random.Random,
    history_length: int = 30,
    max_iterations: int = 1000,
    heuristics: list[Heuristic] | None = None,
    on_progress: Callable[[int, int], None] | None = None,
    progress_every: int = 1000,
    progress_seconds: float = 2.0,
    evaluation: str = INCREMENTAL,
    verify_every: int = 1,
) -> tuple[Solution, int]:
    """Late Acceptance Hill Climbing (Burke & Bykov): a candidate move is
    accepted if it's no worse than the current solution OR no worse than
    the solution accepted `history_length` steps ago. Returns the best
    solution seen and its cost. A move step that cannot be applied (e.g.
    no reassignable resource in a tiny instance) is skipped.

    If `on_progress` is given, it's called as `on_progress(iteration,
    best_cost)` whenever `progress_every` completed iterations have passed
    OR at least `progress_seconds` have elapsed since the last call,
    whichever comes first -- on a large/slow instance, waiting for a fixed
    iteration count could mean minutes with no feedback at all, so a
    wall-clock heartbeat guarantees the caller hears something regularly
    regardless of instance size.

    `evaluation` selects how candidates are costed: INCREMENTAL keeps one
    IncrementalEvaluator and applies/undoes each candidate against it, FULL
    re-evaluates every candidate from scratch, and VERIFY does both and
    fails loudly if they ever disagree (diagnostic; slower than FULL by
    construction). All three explore the same trajectory for a given seed --
    the search draws from `rng` identically regardless of how costs are
    obtained.
    """
    if evaluation not in (FULL, INCREMENTAL, VERIFY):
        raise ValueError(f"unknown evaluation mode: {evaluation!r}")
    pool = heuristics if heuristics is not None else MANUAL_HEURISTICS

    evaluator = None if evaluation == FULL else IncrementalEvaluator(instance, initial)
    current = initial
    current_cost = (
        evaluator.cost if evaluator is not None else evaluate_cost(instance, current)
    ).as_scalar()
    best, best_cost = current, current_cost
    history = [current_cost] * history_length
    last_progress_time = time.monotonic()

    for step in range(max_iterations):
        heuristic = rng.choice(pool)
        try:
            candidate = heuristic.apply(current, instance, rng)
        except ValueError:
            continue

        candidate_cost, transaction = _score_candidate(
            instance,
            evaluator,
            heuristic,
            candidate,
            evaluation,
            check=step % verify_every == 0,
        )

        v = step % history_length
        accepted = candidate_cost <= current_cost or candidate_cost <= history[v]
        if accepted:
            current, current_cost = candidate, candidate_cost
            if current_cost < best_cost:
                best, best_cost = current, current_cost
        history[v] = current_cost

        _finish(evaluator, transaction, accepted, current)

        if on_progress is not None:
            now = time.monotonic()
            if (
                step + 1
            ) % progress_every == 0 or now - last_progress_time >= progress_seconds:
                on_progress(step + 1, best_cost)
                last_progress_time = now

    return best, best_cost
