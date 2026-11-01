"""4-step validation pipeline for LLM-generated heuristic code.

Step 1 — static analysis   (sandbox.static_check)
Step 2 — subprocess isolation (sandbox.subprocess_run)
Step 3 — semantic check    (same event_refs, correct Solution structure)
Step 4 — effectiveness     (M random runs; candidate must not be worse than
                            move_random on average — an anti-harm gate, not
                            a quality gate; UCB handles quality over time)

A ValidationResult is returned for every candidate so the caller can log the
acceptance rate at each step, as required by E7-T6.
"""

import dataclasses as _dataclasses
import itertools
import math
import random
from collections import Counter, defaultdict
from dataclasses import dataclass
from typing import Any

from src.cost import evaluate_cost
from src.evaluator_ref import valid_start_time_ids
from src.heuristics import Heuristic
from src.llm import sandbox
from src.model import Instance, Solution, SolutionEvent, SolutionEventResource

_EFFECTIVENESS_RUNS = 30
_EFFECTIVENESS_BASELINE_ID = "move_random"


@dataclass
class ValidationResult:
    """Outcome of the 4-step pipeline for one candidate."""

    passed: bool
    step_failed: int | None  # 1-4, or None if all passed
    error: str
    code: str


def _build_heuristic(code: str, h_id: str) -> Heuristic:
    """Compile validated code into a Heuristic object.

    Injects the same globals the subprocess sandbox exposes so that helpers
    like valid_start_time_ids and dataclasses work both in the effectiveness
    check (step 4) and in the live solver loop.
    """
    ns: dict[str, Any] = {
        "math": math,
        "dataclasses": _dataclasses,
        "itertools": itertools,
        "defaultdict": defaultdict,
        "Counter": Counter,
        "valid_start_time_ids": valid_start_time_ids,
        "Solution": Solution,
        "SolutionEvent": SolutionEvent,
        "SolutionEventResource": SolutionEventResource,
        "Instance": Instance,
    }
    exec(compile(code, f"<llm:{h_id}>", "exec"), ns)
    apply_fn = ns["apply"]

    return Heuristic(
        id=h_id,
        name=f"LLM-generated: {h_id}",
        protected=False,
        apply=apply_fn,
        incremental_safe=False,
    )


def _baseline_cost(
    instance: Instance,
    solution: Solution,
    baseline_heuristic: Heuristic,
    n_runs: int,
    rng: random.Random,
) -> float:
    total = 0.0
    for _ in range(n_runs):
        try:
            candidate = baseline_heuristic.apply(solution, instance, rng)
            total += evaluate_cost(instance, candidate).as_scalar()
        except ValueError:
            total += evaluate_cost(instance, solution).as_scalar()
    return total / n_runs


def _candidate_cost(
    instance: Instance,
    solution: Solution,
    heuristic: Heuristic,
    n_runs: int,
    rng: random.Random,
) -> float:
    total = 0.0
    for _ in range(n_runs):
        try:
            candidate = heuristic.apply(solution, instance, rng)
            total += evaluate_cost(instance, candidate).as_scalar()
        except ValueError:
            total += evaluate_cost(instance, solution).as_scalar()
    return total / n_runs


def validate(
    code: str,
    h_id: str,
    instance: Instance,
    solution: Solution,
    pool: list[Heuristic],
    seed: int = 0,
) -> tuple[ValidationResult, Heuristic | None]:
    """Run all 4 validation steps.

    Returns:
        (ValidationResult, Heuristic) if all steps pass, else (result, None).
    """
    # Step 1 — static analysis
    static = sandbox.static_check(code)
    if not static["ok"]:
        return ValidationResult(
            passed=False, step_failed=1, error=static["error"], code=code
        ), None

    # Step 2 — subprocess isolation
    sub = sandbox.subprocess_run(code, instance, solution)
    if not sub["ok"]:
        return ValidationResult(
            passed=False, step_failed=2, error=sub["error"], code=code
        ), None

    # Step 3 — semantic check: same event_refs, correct types
    expected_refs = sorted(e.event_ref for e in solution.events)
    returned_refs = sorted(sub.get("event_refs") or [])
    if expected_refs != returned_refs:
        return ValidationResult(
            passed=False,
            step_failed=3,
            error=(
                f"event_refs mismatch: expected {len(expected_refs)} events, "
                f"got {len(returned_refs)}"
            ),
            code=code,
        ), None

    # Step 4 — effectiveness: must not be systematically worse than move_random
    try:
        heuristic = _build_heuristic(code, h_id)
    except Exception as exc:
        return ValidationResult(
            passed=False, step_failed=4, error=f"compile error: {exc}", code=code
        ), None

    baseline = next((h for h in pool if h.id == _EFFECTIVENESS_BASELINE_ID), None)
    if baseline is None:
        # No baseline in pool — skip effectiveness test
        return (
            ValidationResult(passed=True, step_failed=None, error="", code=code),
            heuristic,
        )

    rng = random.Random(seed)
    base_avg = _baseline_cost(instance, solution, baseline, _EFFECTIVENESS_RUNS, rng)
    rng = random.Random(seed)
    cand_avg = _candidate_cost(instance, solution, heuristic, _EFFECTIVENESS_RUNS, rng)

    # Allow up to 5% worse than baseline — the gate rejects only clear duds
    if cand_avg > base_avg * 1.05:
        return ValidationResult(
            passed=False,
            step_failed=4,
            error=(
                f"effectiveness gate failed: candidate avg cost {cand_avg:.0f} "
                f"> baseline {base_avg:.0f} * 1.05"
            ),
            code=code,
        ), None

    return (
        ValidationResult(passed=True, step_failed=None, error="", code=code),
        heuristic,
    )
