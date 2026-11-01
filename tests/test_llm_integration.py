"""End-to-end integration test for LLM heuristic generation (Etap 7 DoD).

Mocks the Ollama client so the GPU is not required; the rest of the pipeline
(static check, subprocess sandbox, semantic + effectiveness validation, pool
insertion) runs for real.
"""

import random
from pathlib import Path
from unittest.mock import patch

from src.construct import build_initial
from src.lahc import run_lahc
from src.llm.client import LLMResponse
from src.llm.pool import GenerationStats
from src.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"

# A minimal valid heuristic: picks a random event and reassigns it to a
# different valid start time.  Uses only globals injected by the subprocess
# sandbox (dataclasses, valid_start_time_ids, rng, solution, instance).
_MOCK_HEURISTIC_CODE = """\
def apply(solution, instance, rng):
    candidates = [
        i for i, se in enumerate(solution.events)
        if se.time_ref is not None and se.duration is not None
    ]
    if not candidates:
        raise ValueError("no movable events")
    index = rng.choice(candidates)
    event = solution.events[index]
    other_times = [
        t for t in valid_start_time_ids(instance, event.duration)
        if t != event.time_ref
    ]
    if not other_times:
        raise ValueError("no alternative times")
    new_events = list(solution.events)
    new_events[index] = dataclasses.replace(event, time_ref=rng.choice(other_times))
    return dataclasses.replace(solution, events=new_events)
"""


def _mock_llm_response() -> LLMResponse:
    return LLMResponse(
        content=_MOCK_HEURISTIC_CODE,
        model="mock",
        prompt_tokens=0,
        completion_tokens=0,
        elapsed_s=0.0,
        cached=False,
    )


def test_llm_e2e_stagnation_generates_and_accepts_heuristic() -> None:
    """Stagnation triggers LLM → 4-step validation → ≥1 heuristic accepted."""
    text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    instance = parse_archive(text)[0]
    initial = build_initial(instance, random.Random(42))

    stats = GenerationStats()

    with patch("src.llm.client.call", return_value=_mock_llm_response()):
        run_lahc(
            instance,
            initial,
            rng=random.Random(1),
            max_iterations=500,
            idle_limit=10,
            llm_enabled=True,
            llm_trigger_multiplier=1,
            llm_n_candidates=1,
            llm_stats=stats,
        )

    assert stats.total_calls >= 1, "LLM was never triggered"
    assert stats.candidates_generated >= 1, "no candidates generated"
    assert stats.accepted >= 1, (
        f"no heuristic passed validation — pass rates: {stats.acceptance_rates()}"
    )
