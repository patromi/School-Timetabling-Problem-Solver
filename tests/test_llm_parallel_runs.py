"""Tests for the fixes that make LLM runs comparable and safe in parallel:

- the candidates of one LLM call are separate samples, not cache hits of
  the first one, and a duplicate heuristic id never enters the pool twice;
- replay files are keyed by prompt variant and model as well;
- time in the LLM generator is left out of the search time limit and of
  the SA cooling schedule;
- appends to shared files go through one locked write.
"""

import random
import time
from pathlib import Path
from typing import Any, cast
from unittest.mock import patch

import pytest
from src.acceptance import AutoSAAcceptance
from src.construct import build_initial
from src.evaluator_ref import evaluate_constraint_costs
from src.lahc import RunStats, run_lahc
from src.llm import client as llm_client
from src.llm import replay as llm_replay
from src.llm.client import LLMResponse
from src.llm.filelock import append_line
from src.llm.pool import GenerationStats, maybe_generate
from src.model import Instance, Solution
from src.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"

# a valid heuristic; {marker} makes otherwise identical code distinct
_HEURISTIC_TEMPLATE = """\
def apply(solution, instance, rng):
    # {marker}
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


def _response(content: str) -> LLMResponse:
    return LLMResponse(
        content=content,
        model="mock",
        prompt_tokens=0,
        completion_tokens=0,
        elapsed_s=0.0,
        cached=False,
    )


@pytest.fixture()
def small_instance() -> Instance:
    text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    return parse_archive(text)[0]


@pytest.fixture()
def initial_solution(small_instance: Instance) -> Solution:
    return build_initial(small_instance, random.Random(0))


def _generate(instance: Instance, solution: Solution, n_candidates: int) -> list[Any]:
    new_heuristics, _, _ = maybe_generate(
        instance=instance,
        solution=solution,
        pool=[],
        selector=None,
        constraint_costs=evaluate_constraint_costs(instance, solution),
        infeasibility=1,
        objective=0,
        stagnant_iterations=100,
        rng=random.Random(0),
        stats=GenerationStats(),
        last_llm_step=-999,
        current_step=0,
        n_candidates=n_candidates,
        seed=None,
        trigger_idx=0,
    )
    return new_heuristics


# --- candidates of one call ---------------------------------------------


def test_cache_key_differs_per_sample_and_keeps_sample_zero() -> None:
    messages = [{"role": "user", "content": "x"}]
    keys = {llm_client._cache_key("m", messages, i) for i in range(3)}
    assert len(keys) == 3
    # sample 0 keeps the key used before sample_idx existed
    sample_zero = llm_client._cache_key("m", messages, 0)
    assert sample_zero == llm_client._cache_key("m", messages)


def test_candidates_are_separate_backend_calls(tmp_path: Path) -> None:
    """Five candidates for one prompt must be five LLM calls, not one call
    and four cache hits returning the same code."""
    messages = [{"role": "user", "content": "prompt"}]
    contents = iter(["code 0", "code 1", "code 2"])
    with (
        patch.object(llm_client, "_CACHE_FILE", tmp_path / "cache.jsonl"),
        patch.object(llm_client, "_CACHE", None),
        patch.object(llm_client, "LLM_BACKEND", "ollama"),
        patch.object(
            llm_client, "_call_ollama", side_effect=lambda *a: (next(contents), 1, 1)
        ) as backend,
    ):
        answers = [
            llm_client.call(messages, model="m", sample_idx=i).content for i in range(3)
        ]
        # the same sample again is a cache hit
        repeated = llm_client.call(messages, model="m", sample_idx=1)

    assert answers == ["code 0", "code 1", "code 2"]
    assert backend.call_count == 3
    assert repeated.cached and repeated.content == "code 1"


def test_maybe_generate_passes_distinct_sample_indices(
    small_instance: Instance, initial_solution: Solution
) -> None:
    seen: list[int] = []

    def fake_call(*_args: object, **kwargs: object) -> LLMResponse:
        seen.append(kwargs["sample_idx"])
        return _response(_HEURISTIC_TEMPLATE.format(marker=kwargs["sample_idx"]))

    with patch("src.llm.client.call", side_effect=fake_call):
        new_heuristics = _generate(small_instance, initial_solution, n_candidates=3)

    assert seen == [0, 1, 2]
    assert len({h.id for h in new_heuristics}) == len(new_heuristics)


def test_duplicate_code_enters_pool_once(
    small_instance: Instance, initial_solution: Solution
) -> None:
    same = _response(_HEURISTIC_TEMPLATE.format(marker="same"))
    with patch("src.llm.client.call", return_value=same):
        new_heuristics = _generate(small_instance, initial_solution, n_candidates=3)

    assert len(new_heuristics) <= 1


# --- replay key -------------------------------------------------------------


def test_replay_separates_prompt_variants_and_models(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(0, "inst", 0, 0, "h_a", "code", variant="a", model="m1")
        llm_replay.save_entry(0, "inst", 0, 0, "h_b", "code", variant="b", model="m1")
        llm_replay.save_entry(0, "inst", 0, 0, "h_m2", "code", variant="a", model="m2")

        a_m1 = llm_replay.load(0, "inst", variant="a", model="m1")
        b_m1 = llm_replay.load(0, "inst", variant="b", model="m1")
        a_m2 = llm_replay.load(0, "inst", variant="a", model="m2")

    assert [e["h_id"] for e in a_m1] == ["h_a"]
    assert [e["h_id"] for e in b_m1] == ["h_b"]
    assert [e["h_id"] for e in a_m2] == ["h_m2"]


def test_model_name_with_slash_stays_in_replay_dir(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(
            0, "inst", 0, 0, "h", "code", variant="a", model="Qwen/Qwen2.5-Coder-32B"
        )
    assert len(list(tmp_path.glob("*.jsonl"))) == 1


# --- LLM time outside the search budget ----------------------------------


def test_sa_pause_keeps_temperature() -> None:
    sa = AutoSAAcceptance(time_budget=100.0)
    sa.reset(10_000)
    before = sa.temperature
    sa.pause(50.0)  # as if 50 s had passed in the LLM generator
    # without pause() the schedule would be ~50% further along by now
    assert sa.temperature >= before * 0.99


def test_llm_time_does_not_count_against_time_limit(
    small_instance: Instance, initial_solution: Solution
) -> None:
    llm_seconds = 0.6
    search_seconds = 0.5

    def slow_generator(*_args: object, **kwargs: object) -> tuple[list[Any], int, int]:
        time.sleep(llm_seconds)
        return (
            [],
            cast(int, kwargs["current_step"]),
            cast(int, kwargs["trigger_idx"]) + 1,
        )

    stats = RunStats()
    started = time.monotonic()
    with patch("src.lahc.maybe_generate", side_effect=slow_generator) as generator:
        run_lahc(
            small_instance,
            initial_solution,
            rng=random.Random(0),
            max_iterations=10_000_000,
            max_seconds=search_seconds,
            idle_limit=1,
            llm_enabled=True,
            llm_trigger_multiplier=1,
            llm_stats=GenerationStats(),
            stats=stats,
        )
    wall = time.monotonic() - started

    assert generator.call_count >= 1
    assert stats.llm_s >= llm_seconds * generator.call_count * 0.9
    # the search itself still got (about) its whole budget
    assert stats.elapsed_s == pytest.approx(search_seconds, abs=0.2)
    assert wall >= search_seconds + llm_seconds * 0.9


# --- shared files -------------------------------------------------------------


def test_append_line_writes_whole_lines(tmp_path: Path) -> None:
    path = tmp_path / "sub" / "log.jsonl"
    for i in range(3):
        append_line(path, f'{{"i": {i}}}')
    assert path.read_text(encoding="utf-8").splitlines() == [
        '{"i": 0}',
        '{"i": 1}',
        '{"i": 2}',
    ]
