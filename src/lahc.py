import math
import random
import time
from collections.abc import Callable

from src.acceptance import AcceptanceCriterion, LAHCAcceptance
from src.solve_log import SolveLogger
from src.selectors import HeuristicSelector, RandomSelector
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
    max_seconds: float | None = None,
    heuristics: list[Heuristic] | None = None,
    selector: HeuristicSelector | None = None,
    on_progress: Callable[[int, int], None] | None = None,
    progress_every: int = 1000,
    progress_seconds: float = 2.0,
    evaluation: str = INCREMENTAL,
    verify_every: int = 1,
    logger: SolveLogger | None = None,
    acceptance: AcceptanceCriterion | None = None,
) -> tuple[Solution, int]:
    """Local search loop with a pluggable acceptance criterion.

    When `acceptance` is None, defaults to LAHCAcceptance(history_length).
    `history_length` is ignored when an explicit `acceptance` is supplied.

    If `on_progress` is given, it's called as `on_progress(iteration,
    best_cost)` whenever `progress_every` completed iterations have passed
    OR at least `progress_seconds` have elapsed since the last call.

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

    if selector is None:
        selector = RandomSelector()

    criterion: AcceptanceCriterion = (
        acceptance if acceptance is not None else LAHCAcceptance(history_length)
    )

    evaluator = None if evaluation == FULL else IncrementalEvaluator(instance, initial)
    current = initial
    current_cost = (
        evaluator.cost if evaluator is not None else evaluate_cost(instance, current)
    ).as_scalar()
    best, best_cost = current, current_cost
    criterion.reset(current_cost)
    last_progress_time = time.monotonic()
    t_start = time.monotonic()

    stagnant_iterations = 0
    perturbation_tier = 0
    idle_limit = max(500, max_iterations // 20)

    for step in range(max_iterations):
        if max_seconds is not None and time.monotonic() - t_start >= max_seconds:
            break
        if stagnant_iterations > idle_limit:
            p_name = "small_perturbation" if perturbation_tier == 0 else "large_perturbation"
            p_heuristic = next((h for h in pool if h.id == p_name), None)

            if p_heuristic is not None:
                try:
                    current = p_heuristic.apply(best, instance, rng)
                    if evaluator is not None:
                        evaluator.rebuild(current)
                        current_cost = evaluator.cost.as_scalar()
                    else:
                        current_cost = evaluate_cost(instance, current).as_scalar()
                    criterion.reset(current_cost)

                    stagnant_iterations = 0
                    perturbation_tier = 1 if perturbation_tier == 0 else 0
                    if logger is not None:
                        logger.log_perturbation(
                            step=step,
                            tier=0 if p_name == "small_perturbation" else 1,
                            heuristic_id=p_heuristic.id,
                            cost_before=best_cost,
                            cost_after=current_cost,
                        )
                    continue
                except ValueError:
                    pass

        heuristic = selector.select(pool, rng)
        move_failed = False
        try:
            candidate = heuristic.apply(current, instance, rng)
        except ValueError:
            selector.update(heuristic, 0.0)
            move_failed = True

        if not move_failed:
            candidate_cost, transaction = _score_candidate(
                instance,
                evaluator,
                heuristic,
                candidate,
                evaluation,
                check=step % verify_every == 0,
            )

            if candidate_cost < best_cost:
                stagnant_iterations = 0
                perturbation_tier = 0
                if logger is not None:
                    infeasibility = candidate_cost // 1_000_000
                    objective = candidate_cost % 1_000_000
                    logger.log_new_best(
                        step=step,
                        heuristic_id=heuristic.id,
                        old_cost=best_cost,
                        new_cost=candidate_cost,
                        infeasibility=infeasibility,
                        objective=objective,
                        elapsed=time.monotonic() - t_start,
                    )
            else:
                stagnant_iterations += 1

            accepted = criterion.accepts(current_cost, candidate_cost, rng)

            reward = 0.0
            if accepted:
                if candidate_cost < current_cost:
                    reward = math.log1p(current_cost - candidate_cost)
                elif candidate_cost == current_cost:
                    reward = 0.1
                else:
                    reward = 0.05

                current, current_cost = candidate, candidate_cost
                if current_cost < best_cost:
                    best, best_cost = current, current_cost

            criterion.after_step(current_cost)
            _finish(evaluator, transaction, accepted, current)
            selector.update(heuristic, reward)
            if logger is not None:
                logger.record_step(
                    step=step,
                    heuristic_id=heuristic.id,
                    reward=reward,
                    accepted=accepted,
                )

        if on_progress is not None:
            now = time.monotonic()
            if (
                step + 1
            ) % progress_every == 0 or now - last_progress_time >= progress_seconds:
                on_progress(step + 1, best_cost)
                last_progress_time = now

    return best, best_cost
