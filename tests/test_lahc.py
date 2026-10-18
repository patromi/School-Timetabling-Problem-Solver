import random
from dataclasses import replace
from pathlib import Path

import pytest
from src.construct import build_initial
from src.evaluator_ref import total_cost
from src.heuristics import MANUAL_HEURISTICS
from src.acceptance import LAHCAcceptance
from src.lahc import run_lahc
from src.selectors import RandomSelector
from src.parser import parse_archive

FIXTURES = Path(__file__).parent / "fixtures"


def _load(name: str) -> str:
    return (FIXTURES / name).read_text(encoding="utf-8")


def test_lahc_never_returns_a_worse_solution_than_the_initial_one():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    initial_cost = total_cost(instance, initial)

    best, best_cost = run_lahc(
        instance, initial, random.Random(1), history_length=20, max_iterations=200
    )

    assert best_cost <= initial_cost
    assert total_cost(instance, best) == best_cost


def test_lahc_is_deterministic_given_the_same_seed():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))

    best_a, cost_a = run_lahc(instance, initial, random.Random(5), max_iterations=100)
    best_b, cost_b = run_lahc(instance, initial, random.Random(5), max_iterations=100)

    assert cost_a == cost_b
    assert best_a.events == best_b.events


def test_lahc_calls_on_progress_periodically_with_iteration_and_best_cost():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    calls = []

    run_lahc(
        instance,
        initial,
        random.Random(1),
        max_iterations=250,
        progress_every=100,
        on_progress=lambda iteration, best_cost: calls.append((iteration, best_cost)),
    )

    assert calls == [(100, calls[0][1]), (200, calls[1][1])]
    assert all(isinstance(cost, int) for _, cost in calls)


def test_lahc_calls_on_progress_when_time_threshold_elapses_even_with_few_iterations():
    # On a slow/large instance, waiting for `progress_every` iterations
    # could take a very long time with no feedback at all -- a time-based
    # trigger must fire regardless of iteration count.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    calls = []

    run_lahc(
        instance,
        initial,
        random.Random(1),
        max_iterations=5,
        progress_every=1_000_000,  # never fires by iteration count alone
        progress_seconds=0.0,  # fire on (almost) every iteration by time
        on_progress=lambda iteration, best_cost: calls.append(iteration),
    )

    assert len(calls) == 5


def test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget():
    # Sudoku4x4 is tiny (16 events, 4 times, 4 rooms) and every one of its
    # constraints is Required=true -- a working local search should drive
    # infeasibility to 0 given a reasonably generous iteration budget.
    # (Empirically tuned for the current default heuristic pool
    # (xhstt_core.heuristics.MANUAL_HEURISTICS): even with move_best/
    # repair_hard_violation's cost-aware placement, the pool has no notion
    # of the RT1..RT4 room subtyping PreferResourcesConstraint needs, so it
    # still wastes proposals on wrong-subtype rooms and needs a generous
    # iteration budget.)
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))

    best, best_cost = run_lahc(
        instance, initial, random.Random(2), history_length=30, max_iterations=40000
    )

    assert best_cost == 0


def test_lahc_accepts_an_explicit_heuristic_pool_override():
    # `heuristics=` (this change's renamed/retyped parameter, replacing the
    # old `moves=`) had zero coverage before this test. A single-entry
    # pool is the simplest way to prove the override is actually plumbed
    # through to the selection call, not silently ignored in favor of the
    # default MANUAL_HEURISTICS.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    move_random = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")

    best, best_cost = run_lahc(
        instance,
        initial,
        random.Random(3),
        max_iterations=50,
        heuristics=[move_random],
    )

    assert len(best.events) == len(initial.events)
    assert best_cost == total_cost(instance, best)


def test_lahc_defaults_to_manual_heuristics_when_no_pool_is_given():
    # Pins down that omitting `heuristics=` genuinely uses
    # xhstt_core.heuristics.MANUAL_HEURISTICS (not a stale local copy) --
    # regression guard for the default-pool wiring itself.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))

    best_default, cost_default = run_lahc(
        instance, initial, random.Random(3), max_iterations=50
    )
    best_explicit, cost_explicit = run_lahc(
        instance,
        initial,
        random.Random(3),
        max_iterations=50,
        heuristics=MANUAL_HEURISTICS,
    )

    assert best_default.events == best_explicit.events
    assert cost_default == cost_explicit


@pytest.mark.parametrize("fixture", ["ArtificialSudoku4x4.xml", "BrazilInstance1.xml"])
def test_incremental_and_full_evaluation_explore_an_identical_trajectory(fixture):
    # The strongest end-to-end guard on the delta path: same seed, same
    # pool, same acceptance decisions -> byte-identical result. Any drift in
    # a single candidate's cost changes an accept/reject decision and
    # diverges the two runs from that point on.
    instance = parse_archive(_load(fixture))[0]
    initial = build_initial(instance, random.Random(0))

    best_full, cost_full = run_lahc(
        instance, initial, random.Random(7), max_iterations=300, evaluation="full"
    )
    best_incremental, cost_incremental = run_lahc(
        instance,
        initial,
        random.Random(7),
        max_iterations=300,
        evaluation="incremental",
    )

    assert cost_incremental == cost_full
    assert best_incremental.events == best_full.events
    assert total_cost(instance, best_incremental) == cost_incremental


def test_verify_mode_checks_every_candidate_against_the_reference_evaluator():
    instance = parse_archive(_load("BrazilInstance1.xml"))[0]
    initial = build_initial(instance, random.Random(0))

    best, cost = run_lahc(
        instance, initial, random.Random(8), max_iterations=60, evaluation="verify"
    )

    assert total_cost(instance, best) == cost


def test_lahc_rejects_an_unknown_evaluation_mode():
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))

    with pytest.raises(ValueError, match="unknown evaluation mode"):
        run_lahc(instance, initial, random.Random(0), evaluation="delta")


def test_a_heuristic_not_marked_incremental_safe_is_costed_by_full_evaluation():
    # LLM-generated operators (Etap 7) register with the default
    # incremental_safe=False; the solver must still track their cost
    # exactly, including the state rebuild after an accepted one.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    move_random = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    untrusted = replace(move_random, id="untrusted", incremental_safe=False)

    best, cost = run_lahc(
        instance, initial, random.Random(9), max_iterations=200, heuristics=[untrusted]
    )

    assert total_cost(instance, best) == cost


class _RecordingSelector:
    """RandomSelector that records the state of every select/update call."""

    def __init__(self) -> None:
        self.calls: list[tuple[str, str, object]] = []
        self.rewards: list[float] = []

    def select(self, heuristics, rng, state=None):  # type: ignore[no-untyped-def]
        chosen = rng.choice(heuristics)
        self.calls.append(("select", chosen.id, state))
        return chosen

    def update(self, heuristic, reward, state=None):  # type: ignore[no-untyped-def]
        self.calls.append(("update", heuristic.id, state))
        self.rewards.append(reward)


def _always_fails(solution, instance, rng):  # type: ignore[no-untyped-def]
    raise ValueError("no applicable move")


def test_a_failed_move_is_reported_to_the_selector_in_the_current_state():
    # A contextual selector keeps per-state statistics; crediting a failed
    # move to some other state leaves its n == 0 in the current one, so it
    # is force-explored forever.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    move_random = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    failing = replace(move_random, id="always_fails", apply=_always_fails)
    selector = _RecordingSelector()

    run_lahc(
        instance,
        initial,
        random.Random(4),
        max_iterations=100,
        heuristics=[failing, move_random],
        selector=selector,
    )

    pairs = list(zip(selector.calls[::2], selector.calls[1::2], strict=True))
    failed = [(s, u) for s, u in pairs if s[1] == "always_fails"]
    assert failed
    for select_call, update_call in failed:
        assert update_call[0] == "update"
        assert update_call[2] == select_call[2]


def test_an_explicit_idle_limit_sets_the_stagnation_buckets():
    # With an explicit idle_limit the stagnation buckets no longer depend on
    # max_iterations: a solver that never improves walks through medium and
    # high stagnation within idle_limit steps.
    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    move_random = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    failing = replace(move_random, id="always_fails", apply=_always_fails)
    selector = _RecordingSelector()

    run_lahc(
        instance,
        initial,
        random.Random(4),
        max_iterations=10_000_000,
        max_seconds=5,
        idle_limit=30,
        heuristics=[failing],
        selector=selector,
    )

    states = [c[2] for c in selector.calls[:60:2]]
    assert states[0] is not None and states[0][1] == 0
    assert any(s is not None and s[1] == 1 for s in states)
    assert any(s is not None and s[1] == 2 for s in states)


def test_the_selector_reward_is_binary_improvement_plus_a_new_best_bonus():
    # A reward relative to the flattened cost vanishes (~1e-6) for soft
    # improvements while hard violations remain, leaving every Q ~0; the
    # reward is therefore 1 for any candidate better than the current
    # solution, plus a bonus when it is also a new global best.
    from src.lahc import _BEST_BONUS

    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    move_random = next(h for h in MANUAL_HEURISTICS if h.id == "move_random")
    selector = _RecordingSelector()

    run_lahc(
        instance,
        initial,
        random.Random(3),
        max_iterations=300,
        heuristics=[move_random],
        selector=selector,
    )

    assert set(selector.rewards) <= {0.0, 1.0, 1.0 + _BEST_BONUS}
    assert 1.0 + _BEST_BONUS in selector.rewards
    assert 0.0 in selector.rewards


def _perturbation_count(min_relative_improvement: float) -> int:
    import io
    import json

    from src.solve_log import SolveLogger

    instance = parse_archive(_load("ArtificialSudoku4x4.xml"))[0]
    initial = build_initial(instance, random.Random(0))
    out = io.StringIO()
    run_lahc(
        instance,
        initial,
        random.Random(6),
        max_iterations=3000,
        idle_limit=50,
        min_relative_improvement=min_relative_improvement,
        logger=SolveLogger(out),
        # pinned to the configuration the reference count (52) was taken with
        acceptance=LAHCAcceptance(30),
        selector=RandomSelector(),
    )
    events = [json.loads(line)["event"] for line in out.getvalue().splitlines()]
    return events.count("perturbation")


def test_small_improvements_do_not_reset_stagnation_when_a_minimum_is_set():
    # With a minimum relative improvement, a trickle of tiny new bests no
    # longer keeps the stagnation counter at zero, so the solver perturbs
    # at least as often as when every new best resets it.
    assert _perturbation_count(0.99) > _perturbation_count(0.0)


def test_a_zero_minimum_improvement_keeps_the_original_stagnation_rule():
    # 52 is the perturbation count of this seeded run before
    # min_relative_improvement existed (every strictly better new best reset
    # the counter); the default 0 must reproduce that trajectory exactly.
    assert _perturbation_count(0.0) == 52
