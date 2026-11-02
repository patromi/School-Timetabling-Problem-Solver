import json
import random
from dataclasses import asdict, replace
from multiprocessing.process import BaseProcess
from pathlib import Path

import experiments
import pytest
import research_experiments as campaign
from src.construct import build_initial
from src.model import Instance, Solution
from src.parser import parse_archive


def result(iterations_done: int = 37) -> experiments.RunResult:
    return experiments.RunResult(
        instance_id="BR-SA-00",
        selector="ucb",
        acceptance="sa",
        seed=0,
        infeasibility_before=5,
        objective_before=30,
        infeasibility_after=0,
        objective_after=10,
        elapsed_s=1.0,
        iterations_done=iterations_done,
    )


def job() -> campaign.Job:
    return campaign.Job("BR-SA-00", campaign.REFERENCE, 0, 37)


def test_job_order_is_repeatable_and_keys_are_unique() -> None:
    args = (["BR-SA-00"], ["reference", "no-goal"], [0, 1], [37, 100])
    first = campaign.make_jobs(*args, order_seed=7)
    assert first == campaign.make_jobs(*args, order_seed=7)
    assert len({j.key for j in first}) == 8
    assert set(first) == set(campaign.make_jobs(*args, order_seed=8))


def test_jobs_are_split_by_track() -> None:
    tracks = {"BR-SA-00": "time", "ES-SS-08": "resource"}
    jobs = campaign.make_jobs(
        ["BR-SA-00", "ES-SS-08"],
        ["reference", "no-resource-ops"],
        [0],
        {"time": [100], "resource": [10, 20]},
        order_seed=1,
        tracks=tracks,
    )
    assert {(j.instance_id, j.variant.name, j.iterations, j.track) for j in jobs} == {
        ("BR-SA-00", "reference", 100, "time"),
        ("ES-SS-08", "reference", 10, "resource"),
        ("ES-SS-08", "reference", 20, "resource"),
        ("ES-SS-08", "no-resource-ops", 10, "resource"),
        ("ES-SS-08", "no-resource-ops", 20, "resource"),
    }


def test_default_variants_use_ucb_only() -> None:
    for track, names in campaign.DEFAULT_VARIANTS.items():
        assert "reference" in names
        assert all(campaign.VARIANTS[n].selector.startswith("ucb") for n in names)
        assert all(track in campaign.VARIANTS[n].tracks for n in names)
    assert "no-resource-moves" in campaign.DEFAULT_VARIANTS["resource"]
    assert "no-resource-moves" not in campaign.DEFAULT_VARIANTS["time"]


def test_resource_ablations_only_remove_resource_operators() -> None:
    no_ops = campaign.VARIANTS["no-resource-ops"]
    no_moves = campaign.VARIANTS["no-resource-moves"]
    assert set(no_ops.exclude) == {"resource_swap", "repair_resource_clash"}
    assert set(no_moves.exclude) == {*no_ops.exclude, "resource_reassign"}
    assert no_ops.tracks == no_moves.tracks == ("resource",)
    assert replace(no_ops, name="reference", exclude=(), tracks=campaign.TRACKS) == (
        campaign.REFERENCE
    )


def test_group_selects_default_instances(tmp_path: Path) -> None:
    def instances(*options: str) -> list[str]:
        args = campaign.parse_args(["--output", str(tmp_path), *options])
        return args.instances

    assert instances("--group", "time") == list(campaign.TRACK_INSTANCES["time"])
    assert instances("--group", "resource") == list(
        campaign.TRACK_INSTANCES["resource"]
    )
    assert instances() == [
        *campaign.TRACK_INSTANCES["time"],
        *campaign.TRACK_INSTANCES["resource"],
    ]
    assert instances("--group", "resource", "--instances", "BR-SA-00") == ["BR-SA-00"]


def test_campaign_tracks_instances_by_resource_choices(tmp_path: Path) -> None:
    def spec(*options: str) -> dict:
        args = campaign.parse_args(["--output", str(tmp_path), *options])
        return campaign.specification(args)[0]

    profiles = spec(
        "--instances", "BR-SA-00", "ES-SS-08", "--seeds", "0", "--budgets", "10"
    )["profiles"]
    assert profiles["BR-SA-00"]["track"] == "time"
    assert profiles["ES-SS-08"]["track"] == "resource"
    with pytest.raises(ValueError, match="LinkEvents"):
        spec("--instances", "AU-TE-99")


def test_ablation_changes_only_target_parameters() -> None:
    assert campaign.VARIANTS["no-goal"] == replace(
        campaign.REFERENCE,
        name="no-goal",
        exclude=("event_block_swap", "reassign_resource_times"),
    )
    assert campaign.VARIANTS["rrt-selector"].escape_every is None
    assert campaign.VARIANTS["fixed-idle"].reheat == "remaining"
    assert campaign.VARIANTS["legacy-perturbations"].idle_limit == 3000
    assert campaign.VARIANTS["legacy-perturbations"].reheat == "full"
    assert not campaign.VARIANTS["no-perturbations"].perturbations


@pytest.mark.parametrize(
    ("wins", "losses", "expected"),
    [(0, 0, 1.0), (5, 0, 0.0625), (0, 5, 0.0625), (3, 3, 1.0)],
)
def test_exact_sign_test(wins: int, losses: int, expected: float) -> None:
    assert campaign.sign_test(wins, losses) == expected


def test_sign_test_handles_many_seeds_without_enumeration() -> None:
    assert campaign.sign_test(100, 0) == pytest.approx(2**-99)
    assert campaign.sign_test(1000, 1000) == 1.0
    with pytest.raises(ValueError):
        campaign.sign_test(-1, 0)


def test_holm_corrects_the_entire_family_in_original_order() -> None:
    assert campaign.holm_adjust([0.04, 0.001, 0.03]) == pytest.approx(
        [0.06, 0.003, 0.06]
    )
    assert campaign.holm_adjust([]) == []


def test_comparisons_are_paired_lexicographically_not_scalar() -> None:
    rows = [
        {
            "track": "time",
            "instance_id": "test",
            "budget": 37,
            "variant": variant,
            "seed": seed,
            "infeasibility_after": hard,
            "objective_after": soft,
        }
        for variant, seed, hard, soft in [
            ("reference", 0, 1, 0),
            ("reference", 1, 0, 3),
            ("no-goal", 0, 0, 100_000_000),
            ("no-goal", 1, 0, 3),
            ("no-goal", 2, 0, 0),
        ]
    ]
    comparison = campaign.comparisons(rows)[0]
    assert comparison["pairs"] == 2
    assert comparison["unpaired"] == 1
    assert (comparison["wins"], comparison["ties"], comparison["losses"]) == (1, 1, 0)
    assert comparison["p_sign"] == 1.0


def test_no_pairs_is_not_reported_as_a_significant_result() -> None:
    rows = [
        {
            "track": "time",
            "instance_id": "test",
            "budget": 37,
            "variant": "no-goal",
            "seed": 0,
            "infeasibility_after": 0,
            "objective_after": 0,
        }
    ]
    assert campaign.comparisons(rows)[0]["p_sign"] is None


def test_manifest_refuses_changed_configuration(tmp_path: Path) -> None:
    specification = {"sources": {"x": "abc"}, "jobs": [asdict(job())]}
    campaign.prepare_manifest(tmp_path, specification)
    before = (tmp_path / "manifest.json").read_bytes()
    campaign.prepare_manifest(tmp_path, specification)
    assert (tmp_path / "manifest.json").read_bytes() == before
    with pytest.raises(ValueError, match="Zmiana kodu"):
        campaign.prepare_manifest(tmp_path, {"sources": {"x": "changed"}})


def test_lock_releases_and_refuses_concurrent_campaigns(tmp_path: Path) -> None:
    with campaign.campaign_lock(tmp_path):
        with pytest.raises(RuntimeError, match="inna kampanie"):
            with campaign.campaign_lock(tmp_path):
                pass
    with campaign.campaign_lock(tmp_path):
        pass


def test_resume_skips_only_complete_runs(tmp_path: Path) -> None:
    current = job()
    directory = tmp_path / "runs" / current.key
    directory.mkdir(parents=True)
    assert campaign.pending_jobs(tmp_path, [current], False) == [current]
    campaign.atomic_json(directory / "error.json", {"traceback": "failure"})
    assert campaign.pending_jobs(tmp_path, [current], False) == []
    assert campaign.pending_jobs(tmp_path, [current], True) == [current]
    campaign.atomic_json(directory / "result.json", asdict(result()))
    with pytest.raises(ValueError, match="Brak artefaktu"):
        campaign.pending_jobs(tmp_path, [current], False)
    (directory / "solver.jsonl").write_text("", encoding="utf-8")
    (directory / "solution.xml").write_text("<solution/>", encoding="utf-8")
    assert campaign.pending_jobs(tmp_path, [current], True) == []
    campaign.atomic_json(directory / "result.json", asdict(result(iterations_done=36)))
    with pytest.raises(ValueError, match="Niepelny wynik"):
        campaign.pending_jobs(tmp_path, [current], False)


def test_worker_persists_failure_without_success_result(tmp_path: Path) -> None:
    work = campaign.Work(job(), tmp_path, {"not": "current"}, 1000, False)
    _, success = campaign.execute(work)
    assert not success
    directory = tmp_path / "runs" / job().key
    assert not (directory / "result.json").exists()
    assert "zmienily" in (directory / "error.json").read_text()


def test_crashed_worker_is_recorded_as_failed(tmp_path: Path) -> None:
    process = BaseProcess()
    work = campaign.Work(job(), tmp_path, {}, 1000, False)
    campaign.finish_worker(work, process)
    assert (tmp_path / "runs" / job().key / "error.json").exists()
    assert not (tmp_path / "runs" / job().key / "result.json").exists()


@pytest.mark.parametrize(
    "options",
    [
        ["--workers", "0"],
        ["--budgets", "-1"],
        ["--seeds", "0", "0"],
        ["--variants", "no-goal"],
        ["--variants", "unknown"],
    ],
)
def test_invalid_campaign_options_fail(options: list[str], tmp_path: Path) -> None:
    with pytest.raises(SystemExit):
        campaign.parse_args(["--output", str(tmp_path), *options])


def test_diagnostics_have_exact_checkpoint_costs(tmp_path: Path) -> None:
    records = [
        {
            "event": "new_best",
            "step": 9,
            "infeasibility": 1,
            "objective": 99,
            "heuristic_id": "swap",
        },
        {
            "event": "new_best",
            "step": 99,
            "infeasibility": 0,
            "objective": 10,
            "heuristic_id": "swap",
        },
        {
            "event": "selector_window",
            "heuristics": {
                "swap": {
                    "calls": 100,
                    "accepted_better": 2,
                    "accepted_equal": 20,
                    "accepted_worse": 3,
                }
            },
        },
    ]
    (tmp_path / "solver.jsonl").write_text(
        "\n".join(json.dumps(r) for r in records), encoding="utf-8"
    )
    diagnostics = campaign.read_diagnostics(
        tmp_path, replace(job(), iterations=100), result()
    )
    assert diagnostics["checkpoints"][0]["infeasibility"] == 5
    assert diagnostics["checkpoints"][1]["infeasibility"] == 1
    assert diagnostics["checkpoints"][-1]["infeasibility"] == 0
    assert diagnostics["usage"]["swap"]["calls"] == 100
    assert diagnostics["usage"]["swap"]["new_best"] == 2


def test_report_marks_incomplete_runs_and_creates_figures(tmp_path: Path) -> None:
    current = job()
    directory = tmp_path / "runs" / current.key
    directory.mkdir(parents=True)
    campaign.atomic_json(directory / "result.json", asdict(result()))
    (directory / "solver.jsonl").write_text(
        json.dumps(
            {
                "event": "new_best",
                "step": 36,
                "infeasibility": 0,
                "objective": 10,
                "heuristic_id": "swap",
            }
        )
        + "\n",
        encoding="utf-8",
    )
    missing = replace(current, seed=1)
    assert campaign.build_report(tmp_path, [current, missing]) == 1
    report = tmp_path / "report"
    assert (report / "BR-SA-00_it37.png").is_file()
    assert "pending" in (report / "incomplete.csv").read_text()
    assert "Ukonczono 1/2" in (report / "README.txt").read_text()


def test_iteration_run_is_repeatable_and_exports_xml(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    archive = Path(__file__).parent / "fixtures" / "ArtificialSudoku4x4.xml"
    monkeypatch.setattr(experiments, "DEFAULT_ARCHIVE", archive)
    instance = parse_archive(archive.read_text())[0]
    results = []
    solutions = []
    for index in range(2):
        xml = tmp_path / f"solution{index}.xml"
        outcome = experiments._run_one(
            instance.id,
            "ucb",
            "sa",
            7,
            0.001,
            tmp_path / f"run{index}.jsonl",
            iteration_limit=37,
            solution_path=xml,
            verify_every=1,
        )
        assert outcome.iterations_done == 37
        assert "budget=37it" in outcome.acceptance_detail
        results.append((outcome.infeasibility_after, outcome.objective_after))
        solutions.append(xml.read_text())
    assert results[0] == results[1]
    assert solutions[0] == solutions[1]
    assert "<SolutionGroups>" in solutions[0]


def test_final_cost_mismatch_fails_before_export(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    archive = Path(__file__).parent / "fixtures" / "ArtificialSudoku4x4.xml"
    monkeypatch.setattr(experiments, "DEFAULT_ARCHIVE", archive)
    instance = parse_archive(archive.read_text())[0]

    def wrong_cost(
        instance: Instance,
        initial: Solution,
        rng: random.Random,
        **kwargs: object,
    ) -> tuple[Solution, int]:
        return build_initial(instance, rng), -1

    monkeypatch.setattr("src.lahc.run_lahc", wrong_cost)
    with pytest.raises(AssertionError, match="full evaluation"):
        experiments._run_one(
            instance.id,
            "ucb",
            "sa",
            7,
            120,
            None,
            iteration_limit=37,
            solution_path=tmp_path / "bad.xml",
        )
    assert not (tmp_path / "bad.xml").exists()


def test_iteration_acceptance_uses_remaining_budget() -> None:
    acceptance = experiments._make_acceptance(
        "sa", 120, iteration_limit=37, reheat="remaining"
    )
    assert "budget=37it,reheat=remaining" in acceptance.name
