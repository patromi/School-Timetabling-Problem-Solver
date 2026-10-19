"""Tests for acceptance criteria (LAHC and SA)."""

import math
import random

import pytest

from src.acceptance import (
    AutoSAAcceptance,
    LAHCAcceptance,
    SimulatedAnnealingAcceptance,
)


class TestLAHCAcceptance:
    def test_always_accepts_improvement(self) -> None:
        c = LAHCAcceptance(history_length=5)
        c.reset(100)
        rng = random.Random(0)
        assert c.accepts(100, 50, rng, 0) is True

    def test_always_accepts_equal_cost(self) -> None:
        c = LAHCAcceptance(history_length=5)
        c.reset(100)
        rng = random.Random(0)
        assert c.accepts(100, 100, rng, 0) is True

    def test_accepts_worsening_within_history(self) -> None:
        c = LAHCAcceptance(history_length=3)
        c.reset(200)
        rng = random.Random(0)
        # history is [200, 200, 200]; candidate 150 <= history[0]=200 → accept
        assert c.accepts(210, 150, rng, 0) is True

    def test_rejects_worsening_above_history(self) -> None:
        c = LAHCAcceptance(history_length=3)
        c.reset(100)
        rng = random.Random(0)
        # history is [100, 100, 100]; current=100; candidate=300 > both → reject
        assert c.accepts(100, 300, rng, 0) is False

    def test_history_updated_after_step(self) -> None:
        # L=2: after two improving steps, history[0] still holds the old high value.
        # A candidate worse than current but better than history[0] should be accepted.
        c = LAHCAcceptance(history_length=2)
        c.reset(300)
        rng = random.Random(0)
        # step 0: v=0, history[0]=300; accept improvement to 50
        assert c.accepts(300, 50, rng, 0) is True
        c.after_step(50, 0)  # history[0]=50
        # step 1: v=1, history[1]=300; accept improvement to 40
        assert c.accepts(50, 40, rng, 1) is True
        c.after_step(40, 1)  # history[1]=40
        # step 2: v=0, history[0]=50; current=40; candidate=45
        # 45 <= 40? No. 45 <= history[0]=50? Yes → accept
        assert c.accepts(40, 45, rng, 2) is True

    def test_reset_reinitialises_history(self) -> None:
        c = LAHCAcceptance(history_length=3)
        c.reset(100)
        rng = random.Random(0)
        for step in range(6):
            c.after_step(50, step)
        # after reset to 1000, history is all 1000s
        c.reset(1000)
        # candidate 999 <= history[0]=1000 → accept even though current=500
        assert c.accepts(500, 999, rng, 6) is True

    def test_slot_follows_loop_step_across_skipped_steps_and_reset(self) -> None:
        # The solver loop also advances on steps that never reach the criterion
        # (failed moves, perturbation restarts), and reset() must not rewind the
        # slot index -- otherwise the default LAHC trajectory changes.
        c = LAHCAcceptance(history_length=3)
        c.reset(100)
        rng = random.Random(0)
        c.after_step(10, 4)  # step 4 -> slot 1, although steps 0..3 were skipped
        assert c.accepts(5, 10, rng, 7) is True  # step 7 -> slot 1 = 10
        assert c.accepts(5, 101, rng, 7) is False
        c.reset(50)
        c.after_step(20, 8)  # step 8 -> slot 2
        assert c.accepts(5, 30, rng, 11) is False  # slot 2 = 20
        assert c.accepts(5, 30, rng, 9) is True  # slot 0 = 50

    def test_name(self) -> None:
        assert LAHCAcceptance(history_length=42).name == "lahc(L=42)"


class TestSimulatedAnnealingAcceptance:
    def test_always_accepts_improvement(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1.0, t_end=0.01, cooling_rate=0.99)
        rng = random.Random(0)
        assert c.accepts(100, 50, rng, 0) is True

    def test_always_accepts_equal(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1.0, t_end=0.01, cooling_rate=0.99)
        rng = random.Random(0)
        assert c.accepts(100, 100, rng, 0) is True

    def test_zero_temperature_rejects_worsening(self) -> None:
        # T tiny → exp(-delta/T) ≈ 0 for any delta > 0
        c = SimulatedAnnealingAcceptance(t_start=1e-300, t_end=1e-300, cooling_rate=0.5)
        rng = random.Random(0)
        results = [c.accepts(100, 101, rng, step) for step in range(100)]
        assert not any(results), "At near-zero T, no worsening move should be accepted"

    def test_high_temperature_accepts_most_worsenings(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1e12, t_end=1.0, cooling_rate=0.9999)
        rng = random.Random(0)
        accepted = sum(c.accepts(100, 101, rng, step) for step in range(1000))
        assert accepted > 990, (
            "At very high T, nearly all worsenings should be accepted"
        )

    def test_cooling_decreases_temperature(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1000.0, t_end=1.0, cooling_rate=0.5)
        c.reset(0)
        t_before = c.temperature
        c.after_step(0, 0)
        assert c.temperature < t_before

    def test_temperature_never_drops_below_t_end(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=10.0, t_end=5.0, cooling_rate=0.1)
        for step in range(100):
            c.after_step(0, step)
        assert c.temperature >= 5.0

    def test_reset_restores_t_start(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1000.0, t_end=1.0, cooling_rate=0.9)
        for step in range(50):
            c.after_step(0, step)
        assert c.temperature < 1000.0
        c.reset(0)
        assert math.isclose(c.temperature, 1000.0)

    def test_invalid_cooling_rate_raises(self) -> None:
        with pytest.raises(ValueError):
            SimulatedAnnealingAcceptance(cooling_rate=1.5)
        with pytest.raises(ValueError):
            SimulatedAnnealingAcceptance(cooling_rate=0.0)

    def test_invalid_temperatures_raise(self) -> None:
        with pytest.raises(ValueError):
            SimulatedAnnealingAcceptance(t_start=-1.0)
        with pytest.raises(ValueError):
            SimulatedAnnealingAcceptance(t_end=0.0)

    def test_name(self) -> None:
        name = SimulatedAnnealingAcceptance(
            t_start=10000.0, t_end=1.0, cooling_rate=0.9999
        ).name
        assert "sa(" in name
        assert "T0=10000" in name


class TestAutoSAIterationBudget:
    """The iteration-budget mode cools by step number instead of the clock,
    so a run is reproducible from its seed."""

    def test_temperature_runs_from_t0_to_t_end_over_the_budget(self) -> None:
        from src.acceptance import AutoSAAcceptance

        c = AutoSAAcceptance(t0_factor=0.01, t_end=1.0, iteration_budget=1000)
        c.reset(1_000_000)
        assert c.temperature_at(0) == pytest.approx(10_000.0)
        assert c.temperature_at(500) == pytest.approx(100.0)
        assert c.temperature_at(1000) == pytest.approx(1.0)
        assert c.temperature_at(5000) == pytest.approx(1.0)

    def test_reset_reheats_from_the_current_step(self) -> None:
        from src.acceptance import AutoSAAcceptance

        c = AutoSAAcceptance(t0_factor=0.01, t_end=1.0, iteration_budget=1000)
        c.reset(1_000_000)
        for step in range(400):
            c.after_step(1_000_000, step)
        c.reset(1_000_000)
        assert c.temperature_at(400) == pytest.approx(10_000.0)

    def test_decisions_are_reproducible_from_the_seed(self) -> None:
        from src.acceptance import AutoSAAcceptance

        def decisions() -> list[bool]:
            c = AutoSAAcceptance(iteration_budget=200)
            c.reset(50_000)
            rng = random.Random(7)
            out = []
            for step in range(200):
                out.append(c.accepts(50_000, 50_300, rng, step))
                c.after_step(50_000, step)
            return out

        assert decisions() == decisions()


class TestAutoSAReheat:
    """After a perturbation's reset(), "full" cools over the whole budget
    again and "remaining" over what is left of it."""

    @staticmethod
    def _reheated(reheat: str) -> AutoSAAcceptance:
        c = AutoSAAcceptance(
            t0_factor=0.01, t_end=1.0, iteration_budget=1000, reheat=reheat
        )
        c.reset(1_000_000)
        for step in range(600):
            c.after_step(1_000_000, step)
        c.reset(1_000_000)  # perturbation at step 600: 400 steps left
        return c

    def test_full_cools_over_the_whole_budget_again(self) -> None:
        c = self._reheated("full")
        assert c.temperature_at(600) == pytest.approx(10_000.0)
        assert c.temperature_at(1100) == pytest.approx(100.0)
        assert c.temperature_at(1600) == pytest.approx(1.0)

    def test_remaining_still_ends_at_t_end_on_budget(self) -> None:
        c = self._reheated("remaining")
        assert c.temperature_at(600) == pytest.approx(10_000.0)
        assert c.temperature_at(800) == pytest.approx(100.0)
        assert c.temperature_at(1000) == pytest.approx(1.0)

    def test_remaining_in_clock_mode_shortens_the_schedule(self) -> None:
        from src.acceptance import AutoSAAcceptance

        c = AutoSAAcceptance(time_budget=100.0, reheat="remaining")
        c.reset(1_000_000)
        c._run_start = c._start - 60.0  # 60 s of the budget already used
        c.reset(1_000_000)
        assert c._schedule_time == pytest.approx(40.0, abs=0.5)

    def test_an_unknown_mode_is_rejected(self) -> None:
        from src.acceptance import AutoSAAcceptance

        with pytest.raises(ValueError):
            AutoSAAcceptance(reheat="sometimes")

    def test_the_default_keeps_the_original_name(self) -> None:
        from src.acceptance import AutoSAAcceptance

        assert "reheat" not in AutoSAAcceptance().name
        assert "reheat=remaining" in AutoSAAcceptance(reheat="remaining").name
