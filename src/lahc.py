import random
import time
from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

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

# Operators the solver itself fires on stagnation (see the idle_limit
# branch). By default the selector is not offered them as ordinary moves:
# on their own they almost never improve the current solution, so as
# arms they only take calls from the useful operators.
PERTURBATION_IDS = frozenset({"small_perturbation", "large_perturbation"})


@dataclass
class RunStats:
    """What a run did, as opposed to what it was configured to do: filled in
    by run_lahc when passed as `stats`, for the caller's run summary.

    Every field is updated only on rare events (a new best, a perturbation)
    or once at the end, so collecting them costs nothing per step."""

    steps_done: int = 0
    # search time: wall-clock time without the LLM generator (llm_s)
    elapsed_s: float = 0.0
    # wall-clock time spent in the LLM generator (LLM calls and validation
    # of their code); it does not count against max_seconds
    llm_s: float = 0.0
    # first new best with no hard violation; None if the run never got there
    first_feasible_step: int | None = None
    first_feasible_elapsed_s: float | None = None
    last_improvement_step: int | None = None
    last_improvement_elapsed_s: float | None = None
    small_perturbations: int = 0
    large_perturbations: int = 0


@dataclass
class _OpenPerturbation:
    """A perturbation whose outcome is still being watched -- until the
    next perturbation or the end of the run, when it is logged."""

    step: int
    heuristic_id: str
    best_before: int
    cost_after: int
    # first step at which the current cost was back at best_before
    recovered_step: int | None = None
    # best cost reached since the perturbation, if it beat best_before
    new_best: int | None = None


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
    seed: int | None,
    trigger_idx: int,
) -> tuple[list[Heuristic], int, int]:
    """Fire the LLM generator and return (new_heuristics, updated_last_step, updated_trigger_idx)."""
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
        seed=seed,
        trigger_idx=trigger_idx,
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
    select_perturbations: bool = False,
    time_aware_reward: bool = False,
    snapshot_every: int = 5000,
    stats: RunStats | None = None,
    perturbations: bool = True,
    llm_enabled: bool = False,
    llm_trigger_multiplier: int = _LLM_TRIGGER_MULTIPLIER,
    llm_prompt_variant: str = VARIANT_A,
    llm_n_candidates: int = DEFAULT_N_CANDIDATES,
    llm_stats: GenerationStats | None = None,
    llm_operators: list[Heuristic] | None = None,
    seed: int | None = None,
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
    on stagnation -- unless `perturbations` is False, which turns the
    restarts off altogether (on the large instances plain SA did best,
    18.10.2026); `idle_limit` then only sets the stagnation buckets. The
    selector chooses among the rest of `heuristics`
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
    (selector_state). Each perturbation's outcome -- whether and when the
    current cost got back to the best cost it started from, and whether it
    led to a new best -- is logged when the next perturbation fires or the
    run ends (perturbation_outcome).

    A `stats` object, if given, is filled in with what the run actually did
    (steps done, first feasible step, perturbation counts...; see RunStats).

    LLM integration (Etap 7):
    When `llm_enabled=True` the solver calls `llm.pool.maybe_generate` at a
    separate stagnation counter that resets only on a new best solution
    (not on perturbation).  The trigger fires after `llm_trigger_multiplier *
    idle_limit` iterations without improvement.  Newly validated heuristics
    are appended to the pool and registered with the UCB selector.
    `llm_stats` is updated in-place so the caller can inspect acceptance
    rates after the run.

    Time spent in the LLM generator (the LLM calls and the validation of
    the code they return) is left out of the search clock: `max_seconds`,
    every logged `elapsed_s` and the cooling schedule of a time-keyed
    acceptance criterion (its `pause()` method) count search time only, so
    a run with the LLM gets the same search budget as one without it.  The
    LLM time is reported separately in `stats.llm_s`, as the project spec
    requires.
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
    selectable: list[Heuristic] = (
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
    # running mean of one move's wall time (apply + scoring), for time_aware_reward
    mean_move_time: float | None = None
    # best cost when stagnant_iterations was last reset
    progress_reference = best_cost
    perturbation_tier = 0
    run_stats = stats if stats is not None else RunStats()
    if best_cost // INFEASIBILITY_WEIGHT == 0:
        run_stats.first_feasible_step = 0
        run_stats.first_feasible_elapsed_s = 0.0
    open_perturbation: _OpenPerturbation | None = None

    def close_perturbation() -> None:
        if logger is None or open_perturbation is None:
            return
        p = open_perturbation
        logger.log_perturbation_outcome(
            step=p.step,
            heuristic_id=p.heuristic_id,
            best_before=p.best_before,
            cost_after=p.cost_after,
            recovered=p.recovered_step is not None,
            steps_to_recover=None
            if p.recovered_step is None
            else p.recovered_step - p.step,
            new_best=p.new_best is not None,
            best_after=p.new_best if p.new_best is not None else p.best_before,
        )

    if idle_limit is None:
        idle_limit = max(500, max_iterations // 20)
    # Stagnation thresholds for the contextual UCB state:
    # bucket 0 = low  [0, idle_limit/3)
    # bucket 1 = medium  [idle_limit/3, 2*idle_limit/3)
    # bucket 2 = high  [2*idle_limit/3, ∞)
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
    # counts actual LLM invocations (past cooldown); used as replay key
    _llm_trigger_idx = 0

    def solver_state_now() -> SolverState:
        hard_bucket = 1 if current_cost // INFEASIBILITY_WEIGHT == 0 else 0
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
            fired_on_stagnation=sorted(h.id for h in pool if h.id in PERTURBATION_IDS)
            if perturbations
            else [],
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

        # --- perturbation (resets stagnant_iterations, NOT _llm_stagnant) ---
        if perturbations and stagnant_iterations > idle_limit:
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
                    if p_name == "small_perturbation":
                        run_stats.small_perturbations += 1
                    else:
                        run_stats.large_perturbations += 1
                    close_perturbation()
                    open_perturbation = _OpenPerturbation(
                        step=step,
                        heuristic_id=p_heuristic.id,
                        best_before=best_cost,
                        cost_after=current_cost,
                    )
                    if logger is not None:
                        logger.log_perturbation(
                            step=step,
                            tier=0 if p_name == "small_perturbation" else 1,
                            heuristic_id=p_heuristic.id,
                            cost_before=best_cost,
                            cost_after=current_cost,
                            temperature=_temperature(criterion, step),
                        )
                    log_selector_state(step, "perturbation")
                    steps_done = step + 1
                    continue
                except ValueError:
                    stagnant_iterations = 0
                    perturbation_tier = 1 if perturbation_tier == 0 else 0

        # --- LLM trigger (fires independently of perturbation) ---
        if llm_enabled and _llm_stagnant >= _llm_trigger:
            llm_started = time.monotonic()
            new_heuristics, _llm_last_step, _llm_trigger_idx = _run_llm_trigger(
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
                seed=seed,
                trigger_idx=_llm_trigger_idx,
            )
            # stop the search clock for the time the generator took
            llm_spent = time.monotonic() - llm_started
            t_start += llm_spent
            run_stats.llm_s = round(run_stats.llm_s + llm_spent, 3)
            pause = getattr(criterion, "pause", None)
            if callable(pause):
                pause(llm_spent)
            pool.extend(new_heuristics)
            # LLM heuristics are non-perturbation; add them to selectable
            if not select_perturbations:
                selectable.extend(new_heuristics)
            if logger is not None:
                for h in new_heuristics:
                    logger.log_llm_pool_add(
                        step=step,
                        h_id=h.id,
                        source=h.source,
                        trigger_idx=_llm_trigger_idx - 1,
                    )

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
            move_time = time.perf_counter() - move_start

            stagnant_iterations += 1
            _llm_stagnant += 1

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
                    old_best_cost = best_cost
                    best, best_cost = current, current_cost
                    run_stats.last_improvement_step = step
                    run_stats.last_improvement_elapsed_s = round(
                        time.monotonic() - t_start, 3
                    )
                    if (
                        run_stats.first_feasible_step is None
                        and best_cost // INFEASIBILITY_WEIGHT == 0
                    ):
                        run_stats.first_feasible_step = step
                        run_stats.first_feasible_elapsed_s = (
                            run_stats.last_improvement_elapsed_s
                        )
                    if logger is not None:
                        logger.log_new_best(
                            step=step,
                            heuristic_id=heuristic.id,
                            old_cost=old_best_cost,
                            new_cost=best_cost,
                            infeasibility=best_cost // INFEASIBILITY_WEIGHT,
                            objective=best_cost % INFEASIBILITY_WEIGHT,
                            elapsed=time.monotonic() - t_start,
                        )
                    if best_cost <= progress_reference * (1 - min_relative_improvement):
                        stagnant_iterations = 0
                        _llm_stagnant = 0
                        progress_reference = best_cost
                        perturbation_tier = 0
                    if open_perturbation is not None:
                        open_perturbation.new_best = best_cost
                # the only per-step addition for the logs: one comparison,
                # and only while a perturbation's recovery is still unseen
                if (
                    open_perturbation is not None
                    and open_perturbation.recovered_step is None
                    and current_cost <= open_perturbation.best_before
                ):
                    open_perturbation.recovered_step = step

            criterion.after_step(current_cost, step)
            _finish(evaluator, transaction, accepted, current)
            selector.update(heuristic, reward, state=solver_state)
            if isinstance(selector, UCBSelector) and len(pool) > selector.pool_limit:
                to_remove = selector.try_remove_weakest(pool)
                if to_remove is not None:
                    pool.remove(to_remove)
                    if to_remove in selectable:
                        selectable.remove(to_remove)
            if logger is not None:
                logger.record_step(
                    step=step,
                    heuristic_id=heuristic.id,
                    reward=reward,
                    accepted=accepted,
                    cost_change=cost_change,
                    current_inf=current_cost // INFEASIBILITY_WEIGHT,
                    current_obj=current_cost % INFEASIBILITY_WEIGHT,
                    best_inf=best_cost // INFEASIBILITY_WEIGHT,
                    best_obj=best_cost % INFEASIBILITY_WEIGHT,
                    temperature=_temperature(criterion, step),
                    stagnation=stagnant_iterations,
                    elapsed_s=time.monotonic() - t_start,
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

    close_perturbation()
    run_stats.steps_done = steps_done
    run_stats.elapsed_s = round(time.monotonic() - t_start, 3)
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
