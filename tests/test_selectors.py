"""Tests for selectors.py — RandomSelector, EpsilonGreedySelector, UCBSelector."""

import random

import pytest
from src.heuristics import Heuristic
from src.selectors import (
    EpsilonGreedySelector,
    RandomSelector,
    SolverState,
    UCBSelector,
)


def _dummy_heuristic(h_id: str, protected: bool = True) -> Heuristic:
    return Heuristic(
        id=h_id,
        name=h_id,
        protected=protected,
        apply=lambda sol, inst, rng: sol,
    )


H = [_dummy_heuristic(f"h{i}") for i in range(4)]
UNPROTECTED = _dummy_heuristic("unprotected", protected=False)


# ---------------------------------------------------------------------------
# RandomSelector
# ---------------------------------------------------------------------------


def test_random_selector_always_picks_from_pool() -> None:
    sel = RandomSelector()
    rng = random.Random(0)
    choices = {sel.select(H, rng) for _ in range(100)}
    assert choices.issubset(set(H))


def test_random_selector_update_is_a_noop() -> None:
    sel = RandomSelector()
    sel.update(H[0], 1.0)  # must not raise


def test_random_selector_accepts_state_kwarg() -> None:
    sel = RandomSelector()
    rng = random.Random(0)
    state: SolverState = (1, 2)
    sel.select(H, rng, state=state)
    sel.update(H[0], 0.5, state=state)


# ---------------------------------------------------------------------------
# EpsilonGreedySelector
# ---------------------------------------------------------------------------


def test_epsilon_greedy_exploits_best_heuristic() -> None:
    sel = EpsilonGreedySelector(epsilon=0.0)
    rng = random.Random(42)
    sel.update(H[0], 0.9)
    sel.update(H[1], 0.1)
    for _ in range(20):
        assert sel.select(H[:2], rng) is H[0]


def test_epsilon_greedy_explores_with_epsilon_one() -> None:
    sel = EpsilonGreedySelector(epsilon=1.0)
    rng = random.Random(42)
    sel.update(H[0], 0.9)
    sel.update(H[1], 0.1)
    choices = {sel.select(H[:2], rng) for _ in range(200)}
    assert len(choices) == 2


def test_epsilon_greedy_q_converges_towards_reward() -> None:
    sel = EpsilonGreedySelector(epsilon=0.0, alpha=0.5)
    for _ in range(30):
        sel.update(H[0], 1.0)
    assert sel._q_values[H[0].id] > 0.99


def test_epsilon_greedy_raises_on_empty_pool() -> None:
    with pytest.raises(ValueError):
        EpsilonGreedySelector().select([], random.Random(0))


# ---------------------------------------------------------------------------
# UCBSelector — forced exploration
# ---------------------------------------------------------------------------


def test_ucb_explores_unpulled_heuristics_first() -> None:
    sel = UCBSelector()
    rng = random.Random(0)
    pool = H[:3]

    # Pull h0 first
    sel.select(pool, rng)
    sel.update(H[0], 1.0)

    # h1 and h2 are still unpulled; all subsequent selects must choose from them
    for _ in range(20):
        chosen = sel.select(pool, rng)
        assert chosen in (H[1], H[2])


def test_ucb_explores_new_heuristic_before_established_ones() -> None:
    sel = UCBSelector()
    rng = random.Random(0)
    pool = H[:2]

    # Fully warm up h0 and h1 in state (1, 0)
    state: SolverState = (1, 0)
    for _ in range(5):
        sel.update(H[0], 0.5, state=state)
        sel.update(H[1], 0.5, state=state)
    # Mark them as pulled in that state so no forced exploration triggers
    sel._stats[(state, H[0].id)] = (0.5, 5.0)
    sel._stats[(state, H[1].id)] = (0.5, 5.0)
    sel._total_n[state] = 10.0

    # Add a brand-new heuristic to the pool
    h_new = _dummy_heuristic("new")
    extended = [*pool, h_new]
    # h_new has n=0 in state (1,0) so it must be picked first
    chosen = sel.select(extended, rng, state=state)
    assert chosen is h_new


# ---------------------------------------------------------------------------
# UCBSelector — UCB formula
# ---------------------------------------------------------------------------


def test_ucb_favours_less_explored_heuristic_when_q_is_equal() -> None:
    sel = UCBSelector(c=1.0)
    rng = random.Random(0)
    state: SolverState = (1, 0)
    pool = H[:2]

    # Manually set equal Q, different n
    sel._stats[(state, H[0].id)] = (0.5, 10.0)
    sel._stats[(state, H[1].id)] = (0.5, 2.0)
    sel._total_n[state] = 12.0

    # UCB bonus for H[1] is larger (smaller n), so it should be chosen
    chosen = sel.select(pool, rng, state=state)
    assert chosen is H[1]


def test_ucb_favours_higher_q_when_n_is_equal() -> None:
    sel = UCBSelector(c=0.0)  # disable exploration bonus
    rng = random.Random(0)
    state: SolverState = (0, 1)
    pool = H[:2]

    sel._stats[(state, H[0].id)] = (0.9, 5.0)
    sel._stats[(state, H[1].id)] = (0.1, 5.0)
    sel._total_n[state] = 10.0

    chosen = sel.select(pool, rng, state=state)
    assert chosen is H[0]


# ---------------------------------------------------------------------------
# UCBSelector — Q update
# ---------------------------------------------------------------------------


def test_ucb_update_applies_ema_correctly() -> None:
    sel = UCBSelector(alpha=0.1)
    state: SolverState = (1, 0)
    sel._stats[(state, H[0].id)] = (0.5, 1.0)
    sel._total_n[state] = 1.0

    sel.update(H[0], 1.0, state=state)
    q, n = sel._stats[(state, H[0].id)]
    expected_q = (1 - 0.1) * 0.5 + 0.1 * 1.0
    assert abs(q - expected_q) < 1e-9
    assert n == 2.0


def test_ucb_total_n_increments_per_state() -> None:
    sel = UCBSelector()
    state_a: SolverState = (0, 0)
    state_b: SolverState = (1, 2)

    sel.update(H[0], 0.5, state=state_a)
    sel.update(H[0], 0.5, state=state_a)
    sel.update(H[1], 0.5, state=state_b)

    assert sel._total_n[state_a] == 2.0
    assert sel._total_n[state_b] == 1.0


# ---------------------------------------------------------------------------
# UCBSelector — optimistic initialisation
# ---------------------------------------------------------------------------


def test_ucb_add_heuristic_initialises_all_six_states() -> None:
    sel = UCBSelector(initial_q=0.5)
    h = _dummy_heuristic("brand_new")
    sel.add_heuristic(h, initial_q=0.7)

    for s in [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]:
        q, n = sel._stats[(s, h.id)]
        assert q == 0.7
        assert n == 0.0


def test_ucb_add_heuristic_uses_instance_default_when_no_q_given() -> None:
    sel = UCBSelector(initial_q=0.42)
    h = _dummy_heuristic("new2")
    sel.add_heuristic(h)
    q, _ = sel._stats[((0, 0), h.id)]
    assert q == 0.42


def test_ucb_add_heuristic_does_not_overwrite_existing_stats() -> None:
    sel = UCBSelector()
    h = _dummy_heuristic("existing")
    sel._stats[((0, 0), h.id)] = (0.99, 7.0)
    sel.add_heuristic(h, initial_q=0.1)
    q, n = sel._stats[((0, 0), h.id)]
    assert q == 0.99  # unchanged
    assert n == 7.0


# ---------------------------------------------------------------------------
# UCBSelector — try_remove_weakest
# ---------------------------------------------------------------------------


def test_try_remove_weakest_returns_none_when_pool_within_limit() -> None:
    sel = UCBSelector(pool_limit=20)
    pool = [_dummy_heuristic(f"x{i}", protected=False) for i in range(5)]
    assert sel.try_remove_weakest(pool) is None


def test_try_remove_weakest_returns_none_when_no_eligible_heuristic() -> None:
    # Pool over limit but all are protected
    sel = UCBSelector(pool_limit=2)
    pool = [_dummy_heuristic(f"p{i}", protected=True) for i in range(5)]
    assert sel.try_remove_weakest(pool) is None


def test_try_remove_weakest_returns_none_when_protection_period_not_met() -> None:
    sel = UCBSelector(pool_limit=2, min_pulls_before_removal=100)
    h = _dummy_heuristic("u", protected=False)
    pool = [_dummy_heuristic("p", protected=True), h, _dummy_heuristic("p2")]
    # h only has 5 total pulls — below the 100 threshold
    for s in [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]:
        sel._stats[(s, h.id)] = (0.1, 5.0 / 6)
    assert sel.try_remove_weakest(pool) is None


def test_try_remove_weakest_picks_lowest_mean_q_unprotected_heuristic() -> None:
    sel = UCBSelector(pool_limit=2, min_pulls_before_removal=10)
    good = _dummy_heuristic("good", protected=False)
    bad = _dummy_heuristic("bad", protected=False)
    protected = _dummy_heuristic("prot", protected=True)
    pool = [good, bad, protected]

    # Both good and bad have enough pulls; bad has lower Q
    pulls_per_state = 20.0 / 6
    for s in [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]:
        sel._stats[(s, good.id)] = (0.8, pulls_per_state)
        sel._stats[(s, bad.id)] = (0.1, pulls_per_state)
        sel._stats[(s, protected.id)] = (0.0, pulls_per_state)

    to_remove = sel.try_remove_weakest(pool)
    assert to_remove is bad


def test_try_remove_weakest_never_removes_protected_heuristic() -> None:
    sel = UCBSelector(pool_limit=2, min_pulls_before_removal=10)
    protected = _dummy_heuristic("prot", protected=True)
    unprotected = _dummy_heuristic("u", protected=False)
    pool = [protected, unprotected, _dummy_heuristic("extra")]

    pulls_per_state = 20.0 / 6
    for s in [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]:
        sel._stats[(s, protected.id)] = (0.0, pulls_per_state)
        sel._stats[(s, unprotected.id)] = (0.5, pulls_per_state)
        sel._stats[(s, "extra")] = (0.9, pulls_per_state)

    to_remove = sel.try_remove_weakest(pool)
    assert to_remove is not protected
    assert to_remove is unprotected


# ---------------------------------------------------------------------------
# UCBSelector — state-independence
# ---------------------------------------------------------------------------


def test_ucb_uses_per_state_stats() -> None:
    """Same heuristics, same Q globally — but different pull counts per state
    should yield different UCB scores in different states."""
    sel = UCBSelector(c=1.0)
    state_a: SolverState = (0, 0)
    state_b: SolverState = (1, 2)
    pool = H[:2]

    # In state_a: H[0] highly explored, H[1] rarely
    sel._stats[(state_a, H[0].id)] = (0.5, 50.0)
    sel._stats[(state_a, H[1].id)] = (0.5, 2.0)
    sel._total_n[state_a] = 52.0

    # In state_b: H[1] highly explored, H[0] rarely
    sel._stats[(state_b, H[0].id)] = (0.5, 2.0)
    sel._stats[(state_b, H[1].id)] = (0.5, 50.0)
    sel._total_n[state_b] = 52.0

    rng = random.Random(0)
    chosen_a = sel.select(pool, rng, state=state_a)
    chosen_b = sel.select(pool, rng, state=state_b)
    # In state_a, H[1] has more UCB bonus; in state_b, H[0] does.
    assert chosen_a is H[1]
    assert chosen_b is H[0]


# ---------------------------------------------------------------------------
# UCBSelector — raises on empty pool
# ---------------------------------------------------------------------------


def test_ucb_raises_on_empty_pool() -> None:
    with pytest.raises(ValueError):
        UCBSelector().select([], random.Random(0))


# ---------------------------------------------------------------------------
# UCBSelector — collapse_stagnation (2-state variant)
# ---------------------------------------------------------------------------


def test_ucb_2state_has_two_active_states() -> None:
    sel = UCBSelector(collapse_stagnation=True)
    assert len(sel._all_states) == 2
    assert set(sel._all_states) == {(0, 0), (1, 0)}


def test_ucb_2state_collapses_stagnation_in_select_and_update() -> None:
    sel = UCBSelector(collapse_stagnation=True)
    rng = random.Random(0)
    state_low: SolverState = (1, 0)
    state_high: SolverState = (1, 2)

    # An update in stagnation=2 should land in the same cell as stagnation=0
    sel.update(H[0], 1.0, state=state_high)
    q_high, n_high = sel._stats[((1, 0), H[0].id)]
    sel.update(H[0], 0.0, state=state_low)
    q_both, n_both = sel._stats[((1, 0), H[0].id)]

    # Both updates went to (1, 0); n should be 2
    assert n_high == 1.0
    assert n_both == 2.0


def test_ucb_2state_initialises_only_two_slots_per_heuristic() -> None:
    sel = UCBSelector(collapse_stagnation=True, initial_q=0.5)
    sel._init_heuristic("x")
    keys_for_x = [(s, h) for (s, h) in sel._stats if h == "x"]
    assert len(keys_for_x) == 2


def test_ucb_6state_still_works_after_change() -> None:
    sel = UCBSelector(collapse_stagnation=False)
    assert len(sel._all_states) == 6
    # Stagnation states must remain distinct
    sel.update(H[0], 1.0, state=(0, 0))
    sel.update(H[0], 0.0, state=(0, 2))
    assert sel._stats[((0, 0), H[0].id)][1] == 1.0
    assert sel._stats[((0, 2), H[0].id)][1] == 1.0
