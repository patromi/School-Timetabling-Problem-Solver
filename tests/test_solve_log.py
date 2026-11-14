import io
import json
import random
from pathlib import Path

from src.construct import build_initial
from src.lahc import run_lahc
from src.parser import parse_archive
from src.solve_log import SolveLogger

FIXTURES = Path(__file__).parent / "fixtures"


def _windows(out: io.StringIO) -> list[dict[str, object]]:
    records = [json.loads(line) for line in out.getvalue().splitlines()]
    return [r for r in records if r["event"] == "selector_window"]


def test_a_window_splits_accepted_candidates_by_cost_change():
    out = io.StringIO()
    logger = SolveLogger(out, window_size=5)
    logger.record_step(
        step=0, heuristic_id="h", reward=1.0, accepted=True, cost_change=-3
    )
    logger.record_step(
        step=1, heuristic_id="h", reward=0.0, accepted=True, cost_change=0
    )
    logger.record_step(
        step=2, heuristic_id="h", reward=0.0, accepted=True, cost_change=2
    )
    logger.record_step(
        step=3, heuristic_id="h", reward=0.0, accepted=False, cost_change=5
    )
    logger.record_step(
        step=4, heuristic_id="h", reward=0.0, accepted=True, cost_change=0
    )

    (window,) = _windows(out)
    stats = window["heuristics"]["h"]  # type: ignore[index]
    assert stats["calls"] == 5
    assert stats["accepted"] == 4
    assert stats["accepted_better"] == 1
    assert stats["accepted_equal"] == 2
    assert stats["accepted_worse"] == 1


def test_solver_logs_accepted_candidates_split_by_cost_change():
    # Every accepted candidate falls into exactly one of better / equal /
    # worse, so the three counters always add up to the accepted total.
    text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    instance = parse_archive(text)[0]
    initial = build_initial(instance, random.Random(0))
    out = io.StringIO()
    logger = SolveLogger(out, window_size=50)

    run_lahc(instance, initial, random.Random(2), max_iterations=400, logger=logger)
    logger.flush()

    totals = {"accepted": 0, "better": 0, "equal": 0, "worse": 0}
    for window in _windows(out):
        for stats in window["heuristics"].values():  # type: ignore[attr-defined]
            totals["accepted"] += stats["accepted"]
            totals["better"] += stats["accepted_better"]
            totals["equal"] += stats["accepted_equal"]
            totals["worse"] += stats["accepted_worse"]
    assert totals["accepted"] > 0
    assert totals["better"] + totals["equal"] + totals["worse"] == totals["accepted"]
    assert totals["better"] > 0


def _events(out: io.StringIO, name: str) -> list[dict[str, object]]:
    records = [json.loads(line) for line in out.getvalue().splitlines()]
    return [r for r in records if r["event"] == name]


def test_ucb_snapshot_holds_q_and_n_per_state_and_heuristic():
    from src.heuristics import MANUAL_HEURISTICS
    from src.selectors import UCBSelector

    selector = UCBSelector(c=0.05, alpha=0.1)
    h = MANUAL_HEURISTICS[0]
    selector.select([h], random.Random(0), state=(0, 0))
    selector.update(h, 1.0, state=(0, 0))

    snap = selector.snapshot()
    assert snap["type"] == "ucb"
    assert snap["params"]["c"] == 0.05 and snap["params"]["alpha"] == 0.1
    cell = snap["states"]["0,0"]
    assert cell["N"] == 1
    assert cell["heuristics"][h.id] == {"Q": 0.5 * 0.9 + 0.1, "n": 1}


def test_solver_logs_its_configuration_and_selector_state():
    text = (FIXTURES / "ArtificialSudoku4x4.xml").read_text(encoding="utf-8")
    instance = parse_archive(text)[0]
    initial = build_initial(instance, random.Random(0))
    out = io.StringIO()

    run_lahc(
        instance,
        initial,
        random.Random(2),
        max_iterations=400,
        logger=SolveLogger(out, window_size=50),
        snapshot_every=100,
    )

    (config,) = _events(out, "run_config")
    assert config["selector"]["type"] == "ucb"
    assert "acceptance" in config and "pool" in config and "idle_limit" in config
    states = _events(out, "selector_state")
    assert len(states) >= 4  # every 100 steps plus the final one
    last = states[-1]
    assert last["final"] is True
    assert {"step", "state", "stagnant_iterations", "current_cost", "best_cost"} <= set(
        last
    )
    assert last["selector"]["states"]
