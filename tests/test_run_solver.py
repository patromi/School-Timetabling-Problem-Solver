import json
from pathlib import Path

import run_solver

FIXTURES = Path(__file__).parent / "fixtures"


def _run(tmp_path: Path, *args: str) -> list[dict[str, object]]:
    output = tmp_path / "solution.xml"
    run_solver.main(
        [
            "--archive",
            str(FIXTURES / "ArtificialSudoku4x4.xml"),
            "--output",
            str(output),
            "--seed",
            "1",
            *args,
        ]
    )
    log = output.with_suffix(".jsonl").read_text(encoding="utf-8")
    return [json.loads(line) for line in log.splitlines()]


def test_the_stagnation_limit_does_not_grow_with_the_iteration_budget(
    tmp_path: Path,
) -> None:
    # A time-limited run is given a huge --iterations so the clock decides
    # when it ends; the perturbation threshold must stay at the default
    # instead of becoming max_iterations // 20 (5 million steps here).
    records = _run(tmp_path, "--iterations", "100000000", "--time-limit", "0.5")
    config = next(r for r in records if r["event"] == "run_config")
    assert config["idle_limit"] == run_solver.DEFAULT_IDLE_LIMIT


def test_an_explicit_idle_limit_makes_the_solver_perturb(tmp_path: Path) -> None:
    records = _run(tmp_path, "--iterations", "2000", "--idle-limit", "50")
    config = next(r for r in records if r["event"] == "run_config")
    assert config["idle_limit"] == 50
    assert any(r["event"] == "perturbation" for r in records)
