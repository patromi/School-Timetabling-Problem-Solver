"""LLM pool orchestrator — stagnation trigger, generation, synchronisation.

Called from lahc.py at the LLM stagnation point.  Responsibilities:
  - decide whether to invoke the LLM (budget gate, cooldown)
  - call prompt builder + Ollama client N times
  - run the 4-step validator on each candidate
  - add passing heuristics to the pool via selector.add_heuristic()
  - persist validated code to generated/<timestamp>_<id>.py with JSON metadata
  - log acceptance rates per validation step (E7-T6)

Nothing here modifies the solver loop directly — it returns a list of newly
added Heuristic objects; lahc.py appends them to its pool list.
"""

import hashlib
import json
import random
import time
from dataclasses import dataclass, field
from pathlib import Path

from src.heuristics import Heuristic
from src.llm import client as llm_client
from src.llm import validator as llm_validator
from src.llm.prompt import VARIANT_A, Diagnostic, build
from src.model import Instance, Solution
from src.selectors import UCBSelector

_GENERATED_DIR = Path("generated")

# Default number of candidates to request per LLM invocation.
DEFAULT_N_CANDIDATES = 3

# Minimum iterations between two LLM invocations (cooldown).
# Prevents repeated calls when the solver keeps stagnating quickly.
LLM_COOLDOWN_ITERATIONS = 200


@dataclass
class GenerationStats:
    """Cumulative acceptance-rate counters (E7-T6 logging requirement)."""

    total_calls: int = 0          # number of LLM invocations
    candidates_generated: int = 0 # total raw candidates from LLM
    passed_step: list[int] = field(default_factory=lambda: [0, 0, 0, 0])
    accepted: int = 0             # passed all 4 steps

    def record(self, results: "list[llm_validator.ValidationResult]") -> None:
        self.total_calls += 1
        self.candidates_generated += len(results)
        for r in results:
            if r.passed:
                for i in range(4):
                    self.passed_step[i] += 1
                self.accepted += 1
            elif r.step_failed is not None:
                for i in range(r.step_failed - 1):
                    self.passed_step[i] += 1

    def acceptance_rates(self) -> dict[str, float]:
        n = self.candidates_generated
        if n == 0:
            return {}
        return {
            f"step_{i + 1}_pass_rate": self.passed_step[i] / n
            for i in range(4)
        } | {"overall_acceptance_rate": self.accepted / n}


def _extract_code(content: str) -> str:
    """Strip markdown fences if the model wrapped the function in them."""
    lines = content.strip().splitlines()
    # Drop ```python / ``` fences
    cleaned: list[str] = []
    inside_fence = False
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("```"):
            inside_fence = not inside_fence
            continue
        if not inside_fence:
            cleaned.append(line)
    code = "\n".join(cleaned).strip()
    return code


def _save_generated(
    code: str,
    h_id: str,
    result: llm_validator.ValidationResult,
    diagnostic: Diagnostic,
    prompt_variant: str,
    model: str,
    elapsed_s: float,
) -> Path:
    """Persist code + JSON metadata sidecar to generated/."""
    _GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    timestamp = int(time.time())
    code_hash = hashlib.sha256(code.encode()).hexdigest()[:10]
    stem = f"{timestamp}_{h_id}_{code_hash}"

    code_path = _GENERATED_DIR / f"{stem}.py"
    meta_path = _GENERATED_DIR / f"{stem}.json"

    code_path.write_text(code, encoding="utf-8")
    meta: dict[str, object] = {
        "id": h_id,
        "timestamp": timestamp,
        "code_hash": code_hash,
        "passed": result.passed,
        "step_failed": result.step_failed,
        "error": result.error,
        "prompt_variant": prompt_variant,
        "model": model,
        "elapsed_llm_s": elapsed_s,
        "stagnant_iterations": diagnostic.stagnant_iterations,
        "infeasibility": diagnostic.infeasibility,
        "objective": diagnostic.objective,
    }
    meta_path.write_text(json.dumps(meta, indent=2), encoding="utf-8")
    return code_path


def maybe_generate(
    instance: Instance,
    solution: Solution,
    pool: list[Heuristic],
    selector: UCBSelector | None,
    constraint_costs: tuple[int, ...],
    infeasibility: int,
    objective: int,
    stagnant_iterations: int,
    rng: random.Random,
    stats: GenerationStats,
    last_llm_step: int,
    current_step: int,
    prompt_variant: str = VARIANT_A,
    n_candidates: int = DEFAULT_N_CANDIDATES,
    model: str = llm_client.OLLAMA_MODEL,
    temperature: float = 0.8,
    use_cache: bool = True,
) -> tuple[list[Heuristic], int]:
    """Try to generate and validate new heuristics via LLM.

    Returns:
        (new_heuristics, updated_last_llm_step)

    The caller (lahc.py) appends new_heuristics to its pool and calls
    selector.add_heuristic() for each one.
    """
    if current_step - last_llm_step < LLM_COOLDOWN_ITERATIONS:
        return [], last_llm_step

    diagnostic = Diagnostic(
        instance=instance,
        solution=solution,
        infeasibility=infeasibility,
        objective=objective,
        constraint_costs=constraint_costs,
        stagnant_iterations=stagnant_iterations,
        pool=pool,
        selector=selector,
    )
    messages = build(diagnostic, variant=prompt_variant)

    results: list[llm_validator.ValidationResult] = []
    new_heuristics: list[Heuristic] = []

    for i in range(n_candidates):
        h_id = f"llm_{int(time.time())}_{i}"
        try:
            llm_resp = llm_client.call(
                messages,
                model=model,
                temperature=temperature,
                use_cache=use_cache,
            )
            code = _extract_code(llm_resp.content)
        except Exception as exc:
            results.append(
                llm_validator.ValidationResult(
                    passed=False,
                    step_failed=1,
                    error=f"LLM call failed: {exc}",
                    code="",
                )
            )
            continue

        result, heuristic = llm_validator.validate(
            code=code,
            h_id=h_id,
            instance=instance,
            solution=solution,
            pool=pool,
            seed=rng.randint(0, 2**31),
        )
        results.append(result)

        _save_generated(
            code=code,
            h_id=h_id,
            result=result,
            diagnostic=diagnostic,
            prompt_variant=prompt_variant,
            model=model,
            elapsed_s=llm_resp.elapsed_s,
        )

        if heuristic is not None:
            new_heuristics.append(heuristic)
            if selector is not None:
                selector.add_heuristic(heuristic)

    stats.record(results)
    return new_heuristics, current_step
