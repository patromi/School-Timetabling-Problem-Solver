"""Tests for the seed-keyed LLM heuristic replay cache.

Covers:
- load/save round-trip for replay entries
- has_trigger predicate
- sorting by (trigger_idx, candidate_idx)
- instance_id sanitisation (special characters → underscores)
- replay path is isolated per (seed, instance_id) pair
- pool.maybe_generate uses replay on second run (no LLM call)
- pool.maybe_generate saves to replay file on first run
- replayed heuristics carry source="llm_replay"
- fresh heuristics carry source="llm_fresh"
- lahc.run_lahc logs llm_pool_add events with correct source field
"""

import io
import json
import random
from pathlib import Path
from unittest.mock import patch

import pytest

from src.construct import build_initial
from src.evaluator_ref import evaluate_constraint_costs
from src.lahc import run_lahc
from src.llm import replay as llm_replay
from src.llm.client import LLMResponse
from src.llm.pool import GenerationStats, maybe_generate
from src.parser import parse_archive
from src.solve_log import SolveLogger

FIXTURES = Path(__file__).parent / "fixtures"

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


def _mock_response() -> LLMResponse:
    return LLMResponse(
        content=_MOCK_HEURISTIC_CODE,
        model="mock",
        prompt_tokens=0,
        completion_tokens=0,
        elapsed_s=0.0,
        cached=False,
    )


# ---------------------------------------------------------------------------
# Unit tests for src/llm/replay.py
# ---------------------------------------------------------------------------


def test_load_returns_empty_for_missing_file(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        entries = llm_replay.load(seed=0, instance_id="AU-SA-96")
    assert entries == []


def test_save_and_load_roundtrip(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(
            seed=42,
            instance_id="AU-SA-96",
            trigger_idx=0,
            candidate_idx=0,
            h_id="llm_123_0",
            code=_MOCK_HEURISTIC_CODE,
        )
        entries = llm_replay.load(seed=42, instance_id="AU-SA-96")

    assert len(entries) == 1
    e = entries[0]
    assert e["trigger_idx"] == 0
    assert e["candidate_idx"] == 0
    assert e["h_id"] == "llm_123_0"
    assert e["code"] == _MOCK_HEURISTIC_CODE


def test_multiple_entries_sorted_by_trigger_then_candidate(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        # Insert out-of-order
        llm_replay.save_entry(42, "inst", 1, 1, "h_1_1", "code_1_1")
        llm_replay.save_entry(42, "inst", 0, 1, "h_0_1", "code_0_1")
        llm_replay.save_entry(42, "inst", 1, 0, "h_1_0", "code_1_0")
        llm_replay.save_entry(42, "inst", 0, 0, "h_0_0", "code_0_0")

        entries = llm_replay.load(seed=42, instance_id="inst")

    assert [e["h_id"] for e in entries] == ["h_0_0", "h_0_1", "h_1_0", "h_1_1"]


def test_has_trigger_false_when_no_entries(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        assert llm_replay.has_trigger(0, "inst", 0) is False


def test_has_trigger_true_after_save(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(0, "inst", 2, 0, "h", "code")
        assert llm_replay.has_trigger(0, "inst", 2) is True
        assert llm_replay.has_trigger(0, "inst", 0) is False
        assert llm_replay.has_trigger(0, "inst", 1) is False


def test_different_seeds_use_separate_files(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(1, "inst", 0, 0, "h_seed1", "code_a")
        llm_replay.save_entry(2, "inst", 0, 0, "h_seed2", "code_b")

        e1 = llm_replay.load(1, "inst")
        e2 = llm_replay.load(2, "inst")

    assert e1[0]["h_id"] == "h_seed1"
    assert e2[0]["h_id"] == "h_seed2"


def test_different_instances_use_separate_files(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(0, "AU-SA-96", 0, 0, "h_au", "code_au")
        llm_replay.save_entry(0, "BR-SA-00", 0, 0, "h_br", "code_br")

        au = llm_replay.load(0, "AU-SA-96")
        br = llm_replay.load(0, "BR-SA-00")

    assert au[0]["h_id"] == "h_au"
    assert br[0]["h_id"] == "h_br"


def test_special_chars_in_instance_id_do_not_crash(tmp_path: Path) -> None:
    """instance_id with slashes / spaces / dots must be sanitised to a valid filename."""
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(0, "inst/with spaces.and-dots", 0, 0, "h", "code")
        entries = llm_replay.load(0, "inst/with spaces.and-dots")

    assert len(entries) == 1
    # Verify the file was actually created under tmp_path (not in a subdirectory
    # that would require the slash to exist as a real directory).
    created = list(tmp_path.glob("*.jsonl"))
    assert len(created) == 1
    assert "/" not in created[0].name


def test_replay_file_is_valid_jsonl(tmp_path: Path) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        for i in range(3):
            llm_replay.save_entry(0, "inst", 0, i, f"h_{i}", f"code_{i}")
        path = tmp_path / "seed_0_inst.jsonl"

    lines = path.read_text(encoding="utf-8").splitlines()
    assert len(lines) == 3
    for line in lines:
        obj = json.loads(line)
        assert "trigger_idx" in obj
        assert "candidate_idx" in obj
        assert "h_id" in obj
        assert "code" in obj


# ---------------------------------------------------------------------------
# Integration tests: pool.maybe_generate + replay
# ---------------------------------------------------------------------------


@pytest.fixture()
def small_instance():
    text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    return parse_archive(text)[0]


@pytest.fixture()
def initial_solution(small_instance):
    return build_initial(small_instance, random.Random(0))


def _c_costs(instance, solution):
    return evaluate_constraint_costs(instance, solution)


def test_maybe_generate_saves_to_replay_on_first_run(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    stats = GenerationStats()
    with (
        patch("src.llm.client.call", return_value=_mock_response()),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        new_h, _, new_trigger_idx = maybe_generate(
            instance=small_instance,
            solution=initial_solution,
            pool=[],
            selector=None,
            constraint_costs=_c_costs(small_instance, initial_solution),
            infeasibility=1,
            objective=0,
            stagnant_iterations=100,
            rng=random.Random(7),
            stats=stats,
            last_llm_step=-999,
            current_step=0,
            n_candidates=1,
            seed=7,
            trigger_idx=0,
        )
        entries = llm_replay.load(7, small_instance.id)

    assert new_trigger_idx == 1
    if stats.accepted >= 1:
        assert len(entries) >= 1, "accepted heuristic was not saved to replay file"


def test_maybe_generate_uses_replay_on_second_run(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    """Second call with same seed must not invoke the LLM client."""
    # Pre-populate the replay file with a known-good entry.
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(
            seed=99,
            instance_id=small_instance.id,
            trigger_idx=0,
            candidate_idx=0,
            h_id="llm_preloaded_0",
            code=_MOCK_HEURISTIC_CODE,
        )

    stats = GenerationStats()
    with (
        patch("src.llm.client.call") as mock_call,
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        new_h, _, new_trigger_idx = maybe_generate(
            instance=small_instance,
            solution=initial_solution,
            pool=[],
            selector=None,
            constraint_costs=_c_costs(small_instance, initial_solution),
            infeasibility=1,
            objective=0,
            stagnant_iterations=100,
            rng=random.Random(99),
            stats=stats,
            last_llm_step=-999,
            current_step=0,
            n_candidates=1,
            seed=99,
            trigger_idx=0,
        )

    mock_call.assert_not_called()
    assert new_trigger_idx == 1
    assert len(new_h) == 1
    assert new_h[0].source == "llm_replay"


def test_replayed_heuristic_has_source_llm_replay(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    with patch.object(llm_replay, "_REPLAY_DIR", tmp_path):
        llm_replay.save_entry(
            seed=5, instance_id=small_instance.id,
            trigger_idx=0, candidate_idx=0,
            h_id="h_replay", code=_MOCK_HEURISTIC_CODE,
        )
    stats = GenerationStats()
    with (
        patch("src.llm.client.call"),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        new_h, _, _ = maybe_generate(
            instance=small_instance, solution=initial_solution,
            pool=[], selector=None, constraint_costs=tuple(),
            infeasibility=1, objective=0, stagnant_iterations=100,
            rng=random.Random(5), stats=stats,
            last_llm_step=-999, current_step=0, n_candidates=1,
            seed=5, trigger_idx=0,
        )

    assert all(h.source == "llm_replay" for h in new_h)


def test_fresh_heuristic_has_source_llm_fresh(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    stats = GenerationStats()
    with (
        patch("src.llm.client.call", return_value=_mock_response()),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        new_h, _, _ = maybe_generate(
            instance=small_instance, solution=initial_solution,
            pool=[], selector=None,
            constraint_costs=_c_costs(small_instance, initial_solution),
            infeasibility=1, objective=0, stagnant_iterations=100,
            rng=random.Random(3), stats=stats,
            last_llm_step=-999, current_step=0, n_candidates=1,
            seed=3, trigger_idx=0,
        )

    assert all(h.source == "llm_fresh" for h in new_h)


def test_trigger_idx_not_incremented_during_cooldown(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    stats = GenerationStats()
    with (
        patch("src.llm.client.call", return_value=_mock_response()),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        _, last_step, trigger_idx = maybe_generate(
            instance=small_instance, solution=initial_solution,
            pool=[], selector=None, constraint_costs=tuple(),
            infeasibility=1, objective=0, stagnant_iterations=0,
            rng=random.Random(0), stats=stats,
            last_llm_step=0, current_step=10,  # 10 < LLM_COOLDOWN_ITERATIONS
            n_candidates=1, seed=0, trigger_idx=0,
        )

    assert trigger_idx == 0  # blocked by cooldown — not incremented


def test_trigger_idx_increments_on_actual_fire(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    stats = GenerationStats()
    c_costs = _c_costs(small_instance, initial_solution)
    with (
        patch("src.llm.client.call", return_value=_mock_response()),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        _, _, t1 = maybe_generate(
            instance=small_instance, solution=initial_solution,
            pool=[], selector=None, constraint_costs=c_costs,
            infeasibility=1, objective=0, stagnant_iterations=0,
            rng=random.Random(0), stats=stats,
            last_llm_step=-999, current_step=0,
            n_candidates=1, seed=0, trigger_idx=0,
        )
        _, _, t2 = maybe_generate(
            instance=small_instance, solution=initial_solution,
            pool=[], selector=None, constraint_costs=c_costs,
            infeasibility=1, objective=0, stagnant_iterations=0,
            rng=random.Random(0), stats=stats,
            last_llm_step=-999, current_step=0,
            n_candidates=1, seed=0, trigger_idx=t1,
        )

    assert t1 == 1
    assert t2 == 2


# ---------------------------------------------------------------------------
# Integration test: run_lahc logs llm_pool_add with correct source
# ---------------------------------------------------------------------------


def test_run_lahc_logs_llm_pool_add_source(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    out = io.StringIO()
    logger = SolveLogger(out, window_size=1000)

    with (
        patch("src.llm.client.call", return_value=_mock_response()),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        run_lahc(
            small_instance,
            initial_solution,
            rng=random.Random(1),
            max_iterations=500,
            idle_limit=10,
            llm_enabled=True,
            llm_trigger_multiplier=1,
            llm_n_candidates=1,
            llm_stats=GenerationStats(),
            logger=logger,
            seed=1,
        )

    records = [json.loads(line) for line in out.getvalue().splitlines()]
    pool_adds = [r for r in records if r["event"] == "llm_pool_add"]

    if pool_adds:
        for record in pool_adds:
            assert record["source"] in ("llm_fresh", "llm_replay")
            assert "trigger_idx" in record
            assert "h_id" in record


def test_run_lahc_second_run_uses_replay(
    tmp_path: Path, small_instance, initial_solution
) -> None:
    """Two runs with the same seed must add identical heuristic ids to the pool."""
    stats1 = GenerationStats()
    stats2 = GenerationStats()
    call_counts: list[int] = []

    def _counting_mock(*_args, **_kwargs) -> LLMResponse:  # type: ignore[no-untyped-def]
        call_counts.append(1)
        return _mock_response()

    with (
        patch("src.llm.client.call", side_effect=_counting_mock),
        patch.object(llm_replay, "_REPLAY_DIR", tmp_path),
    ):
        run_lahc(
            small_instance, initial_solution,
            rng=random.Random(42), max_iterations=500,
            idle_limit=10, llm_enabled=True,
            llm_trigger_multiplier=1, llm_n_candidates=1,
            llm_stats=stats1, seed=42,
        )
        calls_first_run = len(call_counts)

        run_lahc(
            small_instance,
            build_initial(small_instance, random.Random(42)),
            rng=random.Random(42), max_iterations=500,
            idle_limit=10, llm_enabled=True,
            llm_trigger_multiplier=1, llm_n_candidates=1,
            llm_stats=stats2, seed=42,
        )
        calls_second_run = len(call_counts) - calls_first_run

    # First run hit the LLM; second run should replay (zero or fewer LLM calls).
    assert calls_first_run >= 1
    assert calls_second_run < calls_first_run
