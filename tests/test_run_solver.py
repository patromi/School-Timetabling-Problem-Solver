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


def test_the_summary_reports_what_the_run_did_not_its_budget(tmp_path: Path) -> None:
    records = _run(tmp_path, "--iterations", "2000", "--idle-limit", "50")
    summary = next(r for r in records if r["event"] == "run_summary")
    last_state = [r for r in records if r["event"] == "selector_state"][-1]
    perturbations = [r for r in records if r["event"] == "perturbation"]

    assert summary["iterations"] == 2000  # the budget, unchanged
    assert summary["steps_done"] == last_state["step"] == 2000
    assert summary["small_perturbations"] + summary["large_perturbations"] == len(
        perturbations
    )
    # a first feasible step exactly when the run ended feasible
    first = summary["first_feasible_step"]
    assert (first is None) == (summary["infeasibility_after"] != 0)
    assert summary["iterations_per_second"] > 0


def test_the_cost_breakdown_adds_up_to_the_reported_cost(tmp_path: Path) -> None:
    records = _run(tmp_path, "--iterations", "300")
    s = next(r for r in records if r["event"] == "run_summary")
    for when in ("before", "after"):
        by_type = s[f"cost_by_type_{when}"]
        assert isinstance(by_type, dict)
        assert sum(by_type["hard"].values()) == s[f"infeasibility_{when}"]
        assert sum(by_type["soft"].values()) == s[f"objective_{when}"]


def test_each_perturbation_gets_an_outcome_record(tmp_path: Path) -> None:
    records = _run(tmp_path, "--iterations", "2000", "--idle-limit", "50")
    perturbations = [r for r in records if r["event"] == "perturbation"]
    outcomes = [r for r in records if r["event"] == "perturbation_outcome"]

    assert perturbations and all(p["temperature"] is not None for p in perturbations)
    assert [o["step"] for o in outcomes] == [p["step"] for p in perturbations]
    for o in outcomes:
        assert o["recovered"] == (o["steps_to_recover"] is not None)
        if o["new_best"]:
            assert o["recovered"] and o["best_after"] < o["best_before"]


def test_the_environment_is_logged_for_reproducing_the_run(tmp_path: Path) -> None:
    records = _run(tmp_path, "--iterations", "100")
    env = next(r for r in records if r["event"] == "run_environment")
    assert env["argv"][-2:] == ["--iterations", "100"]
    assert env["reproducible_from_seed"] is True  # cooled by the step number
    assert env["python"] and env["cpu_count"]
    # run from a git checkout, so the commit is known
    assert isinstance(env["git_commit"], str) and len(env["git_commit"]) == 40


def test_the_automatic_idle_limit_grows_with_the_instance() -> None:
    from src.parser import parse_archive

    for name in ("ArtificialSudoku4x4.xml", "BrazilInstance1.xml"):
        instance = parse_archive((FIXTURES / name).read_text(encoding="utf-8"))[0]
        expected = max(
            run_solver.DEFAULT_IDLE_LIMIT,
            run_solver.IDLE_LIMIT_PER_EVENT * len(instance.events),
        )
        assert run_solver.resolve_idle_limit("auto", instance) == expected
        assert run_solver.resolve_idle_limit("123", instance) == 123


def test_run_solver_reheats_over_the_remaining_budget_by_default(
    tmp_path: Path,
) -> None:
    records = _run(tmp_path, "--iterations", "100")
    config = next(r for r in records if r["event"] == "run_config")
    assert "reheat=remaining" in str(config["acceptance"])


def test_perturbations_can_be_switched_off(tmp_path: Path) -> None:
    # the same run that perturbs with --idle-limit 50 must not perturb at all
    records = _run(
        tmp_path, "--iterations", "2000", "--idle-limit", "50", "--no-perturbations"
    )
    config = next(r for r in records if r["event"] == "run_config")
    summary = next(r for r in records if r["event"] == "run_summary")
    assert config["fired_on_stagnation"] == []
    assert not any(r["event"] == "perturbation" for r in records)
    assert summary["small_perturbations"] == summary["large_perturbations"] == 0
