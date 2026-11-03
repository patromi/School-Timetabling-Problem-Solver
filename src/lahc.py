import random
import time
from collections.abc import Callable

from src.acceptance import AcceptanceCriterion, SimulatedAnnealingAcceptance
from src.cost import evaluate_cost
from src.evaluator_ref import INFEASIBILITY_WEIGHT, evaluate_constraint_costs
from src.heuristics import MANUAL_HEURISTICS, Heuristic
from src.incremental import (
    IncrementalEvaluator,
    StructuralChangeError,
    Transaction,
)
from src.llm.pool import (
    DEFAULT_N_CANDIDATES,
    LLM_COOLDOWN_ITERATIONS,
    GenerationStats,
    maybe_generate,
)
from src.llm.prompt import VARIANT_A
from src.model import Instance, Solution
from src.selectors import HeuristicSelector, SolverState, UCBSelector
from src.solve_log import SolveLogger

FULL = "full"
INCREMENTAL = "incremental"
VERIFY = "verify"

_BEST_BONUS = 0.1

# LLM fires after this many idle_limit multiples without a new best.
_LLM_TRIGGER_MULTIPLIER = 3


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


def _run_perturbation(
    instance: Instance,
    best: Solution,
    best_cost: int,
    pool: list[Heuristic],
    evaluator: IncrementalEvaluator | None,
    criterion: AcceptanceCriterion,
    rng: random.Random,
    perturbation_tier: int,
    step: int,
    logger: SolveLogger | None,
) -> tuple[Solution, int, int, bool]:
    """Apply small or large perturbation from best solution.

    Returns (new_current, new_current_cost, new_perturbation_tier, fired).
    `fired` is False when no perturbation heuristic is in the pool or it
    raises ValueError — caller should not `continue` the loop in that case.
    """
    p_name = "small_perturbation" if perturbation_tier == 0 else "large_perturbation"
    p_heuristic = next((h for h in pool if h.id == p_name), None)
    if p_heuristic is None:
        return best, best_cost, perturbation_tier, False
    try:
        current = p_heuristic.apply(best, instance, rng)
    except ValueError:
        return best, best_cost, perturbation_tier, False

    if evaluator is not None:
        evaluator.rebuild(current)
        current_cost = evaluator.cost.as_scalar()
    else:
        current_cost = evaluate_cost(instance, current).as_scalar()
    criterion.reset(current_cost)
    new_tier = 1 if perturbation_tier == 0 else 0
    if logger is not None:
        logger.log_perturbation(
            step=step,
            tier=0 if p_name == "small_perturbation" else 1,
            heuristic_id=p_heuristic.id,
            cost_before=best_cost,
            cost_after=current_cost,
        )
    return current, current_cost, new_tier, True


def _run_llm_trigger(
    instance: Instance,
    current: Solution,
    current_cost: int,
    pool: list[Heuristic],
    selector: HeuristicSelector,
    evaluator: IncrementalEvaluator | None,
    rng: random.Random,
    step: int,
    llm_last_step: int,
    llm_stagnant: int,
    llm_prompt_variant: str,
    llm_n_candidates: int,
    llm_stats: GenerationStats,
) -> tuple[list[Heuristic], int]:
    """Fire the LLM generator and return (new_heuristics, updated_last_step)."""
    c_costs = (
        evaluator.constraint_costs()
        if evaluator is not None
        else evaluate_constraint_costs(instance, current)
    )
    infeas = current_cost // INFEASIBILITY_WEIGHT
    obj = current_cost % INFEASIBILITY_WEIGHT
    ucb_sel = selector if isinstance(selector, UCBSelector) else None

    return maybe_generate(
        instance=instance,
        solution=current,
        pool=pool,
        selector=ucb_sel,
        constraint_costs=c_costs,
        infeasibility=infeas,
        objective=obj,
        stagnant_iterations=llm_stagnant,
        rng=rng,
        stats=llm_stats,
        last_llm_step=llm_last_step,
        current_step=step,
        prompt_variant=llm_prompt_variant,
        n_candidates=llm_n_candidates,
    )


def _run_llm_operators(
    operators: list[Heuristic],
    instance: Instance,
    current: Solution,
    current_cost: int,
    best: Solution,
    best_cost: int,
    evaluator: IncrementalEvaluator | None,
    rng: random.Random,
) -> tuple[Solution, int, Solution, int]:
    """Try each LLM operator in turn; keep any improvement.

    Operators are applied sequentially — each sees the result of the
    previous one if it improved the solution.  Returns
    (current, current_cost, best, best_cost).
    """
    for op in operators:
        try:
            candidate = op.apply(current, instance, rng)
        except ValueError:
            continue
        c_cost = evaluate_cost(instance, candidate).as_scalar()
        if c_cost < current_cost:
            current, current_cost = candidate, c_cost
            if evaluator is not None:
                evaluator.rebuild(current)
            if c_cost < best_cost:
                best, best_cost = current, c_cost
    return current, current_cost, best, best_cost


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


def run_lahc(  # noqa: C901 — solver loop; complexity is inherent to the algorithm
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
    idle_limit: int | None = None,
    min_relative_improvement: float = 0.0,
    llm_enabled: bool = False,
    llm_trigger_multiplier: int = _LLM_TRIGGER_MULTIPLIER,
    llm_prompt_variant: str = VARIANT_A,
    llm_n_candidates: int = DEFAULT_N_CANDIDATES,
    llm_stats: GenerationStats | None = None,
    llm_operators: list[Heuristic] | None = None,
) -> tuple[Solution, int]:
    """Local search loop with a pluggable acceptance criterion.

    Defaults follow the project's choice (CLAUDE.md, 18.10.2026): when
    `acceptance` is None, simulated annealing (SimulatedAnnealingAcceptance,
    cooled per iteration so a run stays reproducible from its seed); when
    `selector` is None, the contextual UCBSelector. LAHC is still available
    by passing LAHCAcceptance explicitly. `history_length` is unused and kept
    only so older callers do not break.

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

    `idle_limit` is the number of iterations without a new best solution
    after which the solver perturbs the best solution; it also sets the
    stagnation buckets of the selector state (thirds of `idle_limit`). When
    None it is derived from `max_iterations`, which only makes sense for
    iteration-bounded runs -- time-bounded runs should pass it explicitly.

    `min_relative_improvement` is how much a new best has to improve on the
    best cost at the last stagnation reset to reset it again: a new best
    counts as progress only if cost <= reference * (1 - min_relative_
    improvement). With the default 0 every new best is progress; a positive
    value makes a trickle of tiny improvements count as stagnation, so the
    solver also perturbs when its progress has merely slowed down.

    LLM integration (Etap 7):
    When `llm_enabled=True` the solver calls `llm.pool.maybe_generate` at a
    separate stagnation counter that resets only on a new best solution
    (not on perturbation).  The trigger fires after `llm_trigger_multiplier *
    idle_limit` iterations without improvement.  Newly validated heuristics
    are appended to the pool and registered with the UCB selector.
    `llm_stats` is updated in-place so the caller can inspect acceptance
    rates after the run.  LLM wall time is included in elapsed time but
    the iteration counter is unaffected (pauzy nie psuja kryterium akceptacji).
    """
    if evaluation not in (FULL, INCREMENTAL, VERIFY):
        raise ValueError(f"unknown evaluation mode: {evaluation!r}")

    # Copy so LLM-generated heuristics can be appended without mutating
    # MANUAL_HEURISTICS or the caller's list.
    pool: list[Heuristic] = list(
        heuristics if heuristics is not None else MANUAL_HEURISTICS
    )

    if selector is None:
        selector = UCBSelector()

    criterion: AcceptanceCriterion = (
        acceptance if acceptance is not None else SimulatedAnnealingAcceptance()
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
    progress_reference = best_cost
    perturbation_tier = 0
    if idle_limit is None:
        idle_limit = max(500, max_iterations // 20)
    _stag_low = idle_limit // 3
    _stag_med = (2 * idle_limit) // 3

    # LLM state — independent of perturbation resets
    _llm_stats: GenerationStats = (
        llm_stats if llm_stats is not None else GenerationStats()
    )
    _llm_stagnant = 0
    _llm_trigger = llm_trigger_multiplier * idle_limit
    # initialise far enough back that the first trigger is not blocked by cooldown
    _llm_last_step = -LLM_COOLDOWN_ITERATIONS

    for step in range(max_iterations):
        if max_seconds is not None and time.monotonic() - t_start >= max_seconds:
            break

        # --- perturbation (resets stagnant_iterations, NOT _llm_stagnant) ---
        if stagnant_iterations > idle_limit:
            current, current_cost, perturbation_tier, fired = _run_perturbation(
                instance=instance,
                best=best,
                best_cost=best_cost,
                pool=pool,
                evaluator=evaluator,
                criterion=criterion,
                rng=rng,
                perturbation_tier=perturbation_tier,
                step=step,
                logger=logger,
            )
            if fired:
                stagnant_iterations = 0
                progress_reference = best_cost
                continue

        # --- LLM trigger (fires independently of perturbation) ---
        if llm_enabled and _llm_stagnant >= _llm_trigger:
            new_heuristics, _llm_last_step = _run_llm_trigger(
                instance=instance,
                current=current,
                current_cost=current_cost,
                pool=pool,
                selector=selector,
                evaluator=evaluator,
                rng=rng,
                step=step,
                llm_last_step=_llm_last_step,
                llm_stagnant=_llm_stagnant,
                llm_prompt_variant=llm_prompt_variant,
                llm_n_candidates=llm_n_candidates,
                llm_stats=_llm_stats,
            )
            pool.extend(new_heuristics)

            if llm_operators:
                prev_best = best_cost
                current, current_cost, best, best_cost = _run_llm_operators(
                    llm_operators, instance, current, current_cost,
                    best, best_cost, evaluator, rng,
                )
                if best_cost < prev_best:
                    stagnant_iterations = 0

            _llm_stagnant = 0

        # --- main step ---
        hard_bucket = 1 if current_cost < INFEASIBILITY_WEIGHT else 0
        stagnation_bucket = (
            0
            if stagnant_iterations < _stag_low
            else 1
            if stagnant_iterations < _stag_med
            else 2
        )
        solver_state: SolverState = (hard_bucket, stagnation_bucket)

        heuristic = selector.select(pool, rng, state=solver_state)
        move_failed = False
        try:
            candidate = heuristic.apply(current, instance, rng)
        except ValueError:
            selector.update(heuristic, 0.0, state=solver_state)
            stagnant_iterations += 1
            _llm_stagnant += 1
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

            if candidate_cost < best_cost and candidate_cost <= progress_reference * (
                1 - min_relative_improvement
            ):
                stagnant_iterations = 0
                _llm_stagnant = 0
                progress_reference = candidate_cost
                perturbation_tier = 0
            else:
                stagnant_iterations += 1
                _llm_stagnant += 1

            if candidate_cost < best_cost:
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

            accepted = criterion.accepts(current_cost, candidate_cost, rng, step)
            cost_change = candidate_cost - current_cost

            improved = 1.0 if candidate_cost < current_cost else 0.0
            best_bonus = _BEST_BONUS if candidate_cost < best_cost else 0.0
            reward = improved + best_bonus

            if accepted:
                current, current_cost = candidate, candidate_cost
                if current_cost < best_cost:
                    best, best_cost = current, current_cost

            criterion.after_step(current_cost, step)
            _finish(evaluator, transaction, accepted, current)
            selector.update(heuristic, reward, state=solver_state)
            if isinstance(selector, UCBSelector):
                to_remove = selector.try_remove_weakest(pool)
                if to_remove is not None:
                    pool.remove(to_remove)
            if logger is not None:
                logger.record_step(
                    step=step,
                    heuristic_id=heuristic.id,
                    reward=reward,
                    accepted=accepted,
                    cost_change=cost_change,
                )

        if on_progress is not None:
            now = time.monotonic()
            if (
                step + 1
            ) % progress_every == 0 or now - last_progress_time >= progress_seconds:
                on_progress(step + 1, best_cost)
                last_progress_time = now

    return best, best_cost
