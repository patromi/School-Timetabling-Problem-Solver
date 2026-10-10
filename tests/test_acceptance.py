"""Tests for acceptance criteria (LAHC and SA)."""

import math
import random

import pytest

from src.acceptance import LAHCAcceptance, SimulatedAnnealingAcceptance


class TestLAHCAcceptance:
    def test_always_accepts_improvement(self) -> None:
        c = LAHCAcceptance(history_length=5)
        c.reset(100)
        rng = random.Random(0)
        assert c.accepts(100, 50, rng) is True

    def test_always_accepts_equal_cost(self) -> None:
        c = LAHCAcceptance(history_length=5)
        c.reset(100)
        rng = random.Random(0)
        assert c.accepts(100, 100, rng) is True

    def test_accepts_worsening_within_history(self) -> None:
        c = LAHCAcceptance(history_length=3)
        c.reset(200)
        rng = random.Random(0)
        # history is [200, 200, 200]; candidate 150 <= history[0]=200 → accept
        assert c.accepts(210, 150, rng) is True

    def test_rejects_worsening_above_history(self) -> None:
        c = LAHCAcceptance(history_length=3)
        c.reset(100)
        rng = random.Random(0)
        # history is [100, 100, 100]; current=100; candidate=300 > both → reject
        assert c.accepts(100, 300, rng) is False

    def test_history_updated_after_step(self) -> None:
        # L=2: after two improving steps, history[0] still holds the old high value.
        # A candidate worse than current but better than history[0] should be accepted.
        c = LAHCAcceptance(history_length=2)
        c.reset(300)
        rng = random.Random(0)
        # step 0: v=0, history[0]=300; accept improvement to 50
        assert c.accepts(300, 50, rng) is True
        c.after_step(50)   # history[0]=50, step=1
        # step 1: v=1, history[1]=300; accept improvement to 40
        assert c.accepts(50, 40, rng) is True
        c.after_step(40)   # history[1]=40, step=2
        # step 2: v=0, history[0]=50; current=40; candidate=45
        # 45 <= 40? No. 45 <= history[0]=50? Yes → accept
        assert c.accepts(40, 45, rng) is True

    def test_reset_reinitialises_history(self) -> None:
        c = LAHCAcceptance(history_length=3)
        c.reset(100)
        rng = random.Random(0)
        for _ in range(6):
            c.after_step(50)
        # after reset to 1000, history is all 1000s
        c.reset(1000)
        # candidate 999 <= history[0]=1000 → accept even though current=500
        assert c.accepts(500, 999, rng) is True

    def test_name(self) -> None:
        assert LAHCAcceptance(history_length=42).name == "lahc(L=42)"


class TestSimulatedAnnealingAcceptance:
    def test_always_accepts_improvement(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1.0, t_end=0.01, cooling_rate=0.99)
        rng = random.Random(0)
        assert c.accepts(100, 50, rng) is True

    def test_always_accepts_equal(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1.0, t_end=0.01, cooling_rate=0.99)
        rng = random.Random(0)
        assert c.accepts(100, 100, rng) is True

    def test_zero_temperature_rejects_worsening(self) -> None:
        # T tiny → exp(-delta/T) ≈ 0 for any delta > 0
        c = SimulatedAnnealingAcceptance(t_start=1e-300, t_end=1e-300, cooling_rate=0.5)
        rng = random.Random(0)
        results = [c.accepts(100, 101, rng) for _ in range(100)]
        assert not any(results), "At near-zero T, no worsening move should be accepted"

    def test_high_temperature_accepts_most_worsenings(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1e12, t_end=1.0, cooling_rate=0.9999)
        rng = random.Random(0)
        accepted = sum(c.accepts(100, 101, rng) for _ in range(1000))
        assert accepted > 990, "At very high T, nearly all worsenings should be accepted"

    def test_cooling_decreases_temperature(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1000.0, t_end=1.0, cooling_rate=0.5)
        c.reset(0)
        t_before = c.temperature
        c.after_step(0)
        assert c.temperature < t_before

    def test_temperature_never_drops_below_t_end(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=10.0, t_end=5.0, cooling_rate=0.1)
        for _ in range(100):
            c.after_step(0)
        assert c.temperature >= 5.0

    def test_reset_restores_t_start(self) -> None:
        c = SimulatedAnnealingAcceptance(t_start=1000.0, t_end=1.0, cooling_rate=0.9)
        for _ in range(50):
            c.after_step(0)
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
