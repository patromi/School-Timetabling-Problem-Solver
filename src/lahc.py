import random
import time
from collections.abc import Callable
from typing import Any

from src.acceptance import AcceptanceCriterion, SimulatedAnnealingAcceptance
from src.cost import evaluate_cost
from src.evaluator_ref import INFEASIBILITY_WEIGHT
from src.heuristics import MANUAL_HEURISTICS, Heuristic
from src.incremental import (
    IncrementalEvaluator,
    StructuralChangeError,
    Transaction,
)
from src.model import Instance, Solution
from src.selectors import HeuristicSelector, SolverState, UCBSelector
from src.solve_log import SolveLogger

FULL = "full"
INCREMENTAL = "incremental"
VERIFY = "verify"

_BEST_BONUS = 0.1

# Operators the solver itself fires on stagnation (see the idle_limit
# branch). By default the selector is not offered them as ordinary moves:
# on their own they almost never improve the current solution, so as
# arms they only take calls from the useful operators.
PERTURBATION_IDS = frozenset({"small_perturbation", "large_perturbation"})


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
    idle_limit: int | None = None,
    min_relative_improvement: float = 0.0,
    select_perturbations: bool = False,
    time_aware_reward: bool = False,
    snapshot_every: int = 5000,
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

    The perturbation operators (PERTURBATION_IDS) are fired by the solver
    on stagnation; the selector chooses among the rest of `heuristics`
    unless `select_perturbations` is True (the behaviour before
    18.10.2026, kept for reproducing older runs).

    With `time_aware_reward` the selector's reward is scaled by
    min(1, mean move time / this move's time): a move slower than the
    average move earns proportionally less, so an expensive heuristic has to
    improve more often per second, not just per call, to be preferred. It
    reads the wall clock, so a run is no longer reproducible step by step
    from its seed -- meant for time-bounded experiments, which already
    depend on machine speed; off by default.

    With a `logger`, the run's parameters are logged once at the start
    (run_config) and the selector's learned statistics every
    `snapshot_every` steps, at each perturbation and at the end
    (selector_state).
    """
    if evaluation not in (FULL, INCREMENTAL, VERIFY):
        raise ValueError(f"unknown evaluation mode: {evaluation!r}")
    pool = heuristics if heuristics is not None else MANUAL_HEURISTICS

    if selector is None:
        selector = UCBSelector()
    selectable = (
        pool
        if select_perturbations
        else [h for h in pool if h.id not in PERTURBATION_IDS]
    )

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
    # running mean of one move's wall time (apply + scoring), for
    # time_aware_reward
    mean_move_time: float | None = None
    # best cost when stagnant_iterations was last reset
    progress_reference = best_cost
    perturbation_tier = 0
    if idle_limit is None:
        idle_limit = max(500, max_iterations // 20)
    # Stagnation thresholds for the contextual UCB state:
    # bucket 0 = low  [0, idle_limit/3)
    # bucket 1 = medium  [idle_limit/3, 2*idle_limit/3)
    # bucket 2 = high  [2*idle_limit/3, ∞)
    _stag_low = idle_limit // 3
    _stag_med = (2 * idle_limit) // 3

    def solver_state_now() -> SolverState:
        hard_bucket = 1 if current_cost < INFEASIBILITY_WEIGHT else 0
        stagnation_bucket = (
            0
            if stagnant_iterations < _stag_low
            else 1
            if stagnant_iterations < _stag_med
            else 2
        )
        return (hard_bucket, stagnation_bucket)

    def log_selector_state(step: int, reason: str) -> None:
        if logger is None:
            return
        logger.log_selector_state(
            step=step,
            reason=reason,
            final=reason == "end",
            elapsed_s=round(time.monotonic() - t_start, 3),
            state=list(solver_state_now()),
            stagnant_iterations=stagnant_iterations,
            temperature=_temperature(criterion, step),
            current_cost=current_cost,
            best_cost=best_cost,
            selector=_snapshot(selector),
        )

    if logger is not None:
        logger.log_run_config(
            acceptance=getattr(criterion, "name", type(criterion).__name__),
            selector=_snapshot(selector),
            pool=[h.id for h in selectable],
            fired_on_stagnation=sorted(h.id for h in pool if h.id in PERTURBATION_IDS),
            idle_limit=idle_limit,
            stagnation_thresholds=[_stag_low, _stag_med],
            min_relative_improvement=min_relative_improvement,
            best_bonus=_BEST_BONUS,
            time_aware_reward=time_aware_reward,
            evaluation=evaluation,
            max_iterations=max_iterations,
            max_seconds=max_seconds,
            snapshot_every=snapshot_every,
            initial_cost=current_cost,
        )

    steps_done = 0
    for step in range(max_iterations):
        if max_seconds is not None and time.monotonic() - t_start >= max_seconds:
            break
        if stagnant_iterations > idle_limit:
            p_name = (
                "small_perturbation" if perturbation_tier == 0 else "large_perturbation"
            )
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
                    progress_reference = best_cost
                    perturbation_tier = 1 if perturbation_tier == 0 else 0
                    if logger is not None:
                        logger.log_perturbation(
                            step=step,
                            tier=0 if p_name == "small_perturbation" else 1,
                            heuristic_id=p_heuristic.id,
                            cost_before=best_cost,
                            cost_after=current_cost,
                        )
                    log_selector_state(step, "perturbation")
                    steps_done = step + 1
                    continue
                except ValueError:
                    pass

        solver_state = solver_state_now()

        heuristic = selector.select(selectable, rng, state=solver_state)
        move_failed = False
        move_start = time.perf_counter()
        try:
            if evaluator is not None and heuristic.apply_with_evaluator is not None:
                # the evaluator is positioned on `current` here
                candidate = heuristic.apply_with_evaluator(
                    current, instance, rng, evaluator
                )
            else:
                candidate = heuristic.apply(current, instance, rng)
        except ValueError:
            selector.update(heuristic, 0.0, state=solver_state)
            stagnant_iterations += 1
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
            move_time = time.perf_counter() - move_start

            if candidate_cost < best_cost and candidate_cost <= progress_reference * (
                1 - min_relative_improvement
            ):
                stagnant_iterations = 0
                progress_reference = candidate_cost
                perturbation_tier = 0
            else:
                stagnant_iterations += 1

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

            # Reward: 1 if the candidate improves on the current solution
            # (regardless of acceptance), plus a bonus when it also beats the
            # global best. Binary rather than relative to the flattened cost:
            # with INFEASIBILITY_WEIGHT in the denominator a soft improvement
            # is worth ~1e-6 while any hard violation remains, which left
            # every selector Q at ~0 and UCB choosing round-robin.
            improved = 1.0 if candidate_cost < current_cost else 0.0
            best_bonus = _BEST_BONUS if candidate_cost < best_cost else 0.0
            reward = improved + best_bonus
            if time_aware_reward:
                if mean_move_time is not None and move_time > 0:
                    reward *= min(1.0, mean_move_time / move_time)
                mean_move_time = (
                    move_time
                    if mean_move_time is None
                    else 0.99 * mean_move_time + 0.01 * move_time
                )

            if accepted:
                current, current_cost = candidate, candidate_cost
                if current_cost < best_cost:
                    best, best_cost = current, current_cost

            criterion.after_step(current_cost, step)
            _finish(evaluator, transaction, accepted, current)
            selector.update(heuristic, reward, state=solver_state)
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

        steps_done = step + 1
        if snapshot_every > 0 and steps_done % snapshot_every == 0:
            log_selector_state(step, "periodic")

    log_selector_state(steps_done, "end")
    return best, best_cost


def _snapshot(selector: HeuristicSelector) -> dict[str, Any]:
    """The selector's own snapshot() when it has one, else just its name."""
    snapshot = getattr(selector, "snapshot", None)
    if callable(snapshot):
        result: dict[str, Any] = snapshot()
        return result
    return {"type": type(selector).__name__}


def _temperature(criterion: AcceptanceCriterion, step: int) -> float | None:
    """The acceptance temperature at `step`, or None for LAHC."""
    temperature_for = getattr(criterion, "temperature_for", None)
    if callable(temperature_for):
        return round(float(temperature_for(step)), 3)
    temperature = getattr(criterion, "temperature", None)
    return round(float(temperature), 3) if temperature is not None else None
