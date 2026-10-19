#!/usr/bin/env python
"""Report comparing configurations taken from several experiments.

Each --group names one configuration and points at the runs that belong to
it: an experiments.py CSV, its JSONL log directory and the acceptance
criterion to keep from them (one experiment may hold several). The runs are
copied into --out-dir with the group's name written in place of the
selector, and the usual report (scripts/report_experiment.py) is built from
the copy -- so every figure and table compares the groups side by side.
Runs are paired by (instance, seed), which is valid when the groups share
seeds, instances and time limit.

Usage:
    uv run python scripts/compare_experiments.py --out-dir data/results/cmp \
        --group "1. bazowy=data/results/a.csv:data/logs/a:sa" \
        --group "2. T0 1e-2=data/results/b.csv:data/logs/b:sa-t0=1e-2"
"""

import argparse
import csv
import json
import subprocess
import sys
from pathlib import Path


def _parse_group(spec: str) -> tuple[str, Path, Path, str]:
    name, _, rest = spec.partition("=")
    csv_path, log_dir, acceptance = rest.rsplit(":", 2)
    # a Windows drive letter ("C:") would otherwise be split off as a field
    return name, Path(csv_path), Path(log_dir), acceptance


def _copy_group(
    name: str,
    csv_path: Path,
    log_dir: Path,
    acceptance: str,
    rows_out: list[dict[str, str]],
    logs_out: Path,
) -> int:
    kept = 0
    with csv_path.open(encoding="utf-8") as f:
        for row in csv.DictReader(f):
            if row["acceptance"] != acceptance:
                continue
            log = log_dir / (
                f"{row['instance_id']}_{row['selector']}_{acceptance}"
                f"_seed{int(row['seed']):02d}.jsonl"
            )
            rows_out.append({**row, "selector": name, "acceptance": "sa"})
            target = (
                logs_out
                / f"{row['instance_id']}_{name}_seed{int(row['seed']):02d}.jsonl"
            )
            with (
                log.open(encoding="utf-8") as src,
                target.open("w", encoding="utf-8") as dst,
            ):
                for line in src:
                    if '"run_summary"' in line:
                        record = json.loads(line)
                        record.update(selector=name, acceptance="sa")
                        line = json.dumps(record, separators=(",", ":")) + "\n"
                    dst.write(line)
            kept += 1
    return kept


def main() -> None:
    parser = argparse.ArgumentParser(description="Compare configurations.")
    parser.add_argument("--group", action="append", required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--time-limit", type=float, default=120.0)
    args = parser.parse_args()

    logs_out = args.out_dir / "runs"
    logs_out.mkdir(parents=True, exist_ok=True)
    for old in logs_out.glob("*.jsonl"):
        old.unlink()
    rows: list[dict[str, str]] = []
    for spec in args.group:
        name, csv_path, log_dir, acceptance = _parse_group(spec)
        kept = _copy_group(name, csv_path, log_dir, acceptance, rows, logs_out)
        print(f"  {name}: {kept} przebiegów")

    merged = args.out_dir / "runs.csv"
    with merged.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).parent / "report_experiment.py"),
            "--csv",
            str(merged),
            "--log-dir",
            str(logs_out),
            "--out-dir",
            str(args.out_dir),
            "--time-limit",
            str(args.time_limit),
        ],
        check=True,
    )


if __name__ == "__main__":
    main()
