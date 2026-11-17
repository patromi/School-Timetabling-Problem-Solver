#!/usr/bin/env python3
"""Recover preprocessing_experiments_500k.csv directly from slurm_adaptive_*.out.

Reads all finished task log lines printed to stdout and reconstructs the full CSV
matching the exact schema of preprocessing_experiments.py (23 columns).
Does NOT touch or depend on baseline_experiments_500k.csv.
"""

import csv
import glob
import re
import sys
from pathlib import Path

# Static metadata for the 17 benchmark instances from XHSTT-2014
INSTANCE_METADATA: dict[str, dict[str, str | int]] = {
    "AU-BG-98": {
        "events_count": 387,
        "times_count": 40,
        "resources_count": 131,
        "constraints_count": 172,
        "scale": "medium",
        "tightness": "loose",
    },
    "AU-SA-96": {
        "events_count": 296,
        "times_count": 60,
        "resources_count": 99,
        "constraints_count": 61,
        "scale": "medium",
        "tightness": "moderate",
    },
    "BR-SA-00": {
        "events_count": 63,
        "times_count": 25,
        "resources_count": 20,
        "constraints_count": 15,
        "scale": "small",
        "tightness": "tight",
    },
    "BR-SM-00": {
        "events_count": 127,
        "times_count": 25,
        "resources_count": 35,
        "constraints_count": 28,
        "scale": "small",
        "tightness": "tight",
    },
    "BR-SN-00": {
        "events_count": 140,
        "times_count": 25,
        "resources_count": 44,
        "constraints_count": 14,
        "scale": "small",
        "tightness": "tight",
    },
    "DK-FG-12": {
        "events_count": 1077,
        "times_count": 50,
        "resources_count": 438,
        "constraints_count": 30,
        "scale": "large",
        "tightness": "moderate",
    },
    "FI-MP-06": {
        "events_count": 280,
        "times_count": 35,
        "resources_count": 64,
        "constraints_count": 93,
        "scale": "medium",
        "tightness": "moderate",
    },
    "FI-PB-98": {
        "events_count": 387,
        "times_count": 40,
        "resources_count": 111,
        "constraints_count": 50,
        "scale": "medium",
        "tightness": "moderate",
    },
    "FI-WP-06": {
        "events_count": 172,
        "times_count": 35,
        "resources_count": 41,
        "constraints_count": 31,
        "scale": "small",
        "tightness": "moderate",
    },
    "GR-H1-97": {
        "events_count": 372,
        "times_count": 35,
        "resources_count": 95,
        "constraints_count": 9,
        "scale": "medium",
        "tightness": "loose",
    },
    "GR-P3-10": {
        "events_count": 178,
        "times_count": 35,
        "resources_count": 113,
        "constraints_count": 102,
        "scale": "small",
        "tightness": "loose",
    },
    "GR-PA-08": {
        "events_count": 262,
        "times_count": 35,
        "resources_count": 31,
        "constraints_count": 64,
        "scale": "medium",
        "tightness": "loose",
    },
    "IT-I4-96": {
        "events_count": 748,
        "times_count": 36,
        "resources_count": 99,
        "constraints_count": 73,
        "scale": "large",
        "tightness": "moderate",
    },
    "KS-PR-11": {
        "events_count": 809,
        "times_count": 62,
        "resources_count": 164,
        "constraints_count": 15,
        "scale": "large",
        "tightness": "loose",
    },
    "NL-KP-05": {
        "events_count": 1235,
        "times_count": 37,
        "resources_count": 644,
        "constraints_count": 84,
        "scale": "large",
        "tightness": "loose",
    },
    "ZA-LW-09": {
        "events_count": 185,
        "times_count": 148,
        "resources_count": 37,
        "constraints_count": 30,
        "scale": "small",
        "tightness": "loose",
    },
    "ZA-WD-09": {
        "events_count": 278,
        "times_count": 42,
        "resources_count": 70,
        "constraints_count": 27,
        "scale": "medium",
        "tightness": "tight",
    },
}


def recover_csv(
    out_file: Path | str,
    output_csv: Path | str = "data/results/preprocessing_experiments_500k.csv",
) -> int:
    out_path = Path(out_file)
    csv_path = Path(output_csv)

    # Line format from _print_run_log in preprocessing_experiments.py:
    # [  1/170] BR-SA-00 [adaptive] seed= 0: hard 20 -> 0, soft 1234 -> 567 |
    # 500,000 it w 12.3s (40,650 it/s) | WYKONALNY w kroku 12345 (0.31s)
    pattern = re.compile(
        r"\[\s*\d+/\d+\]\s+(\S+)\s+\[(\S+)\]\s+seed=\s*(\d+):\s+"
        r"hard\s+(\d+)\s+->\s+(\d+),\s+soft\s+(\d+)\s+->\s+(\d+)\s+\|\s+"
        r"([\d,]+)\s+it\s+w\s+([\d.]+)s\s+\(([\d,]+)\s+it/s\)\s+\|\s+(.*)"
    )

    rows: list[dict[str, str | int | float | bool | None]] = []
    seen: set[tuple[str, int]] = set()

    with open(out_path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = pattern.search(line)
            if not m:
                continue

            (
                inst,
                mode,
                seed_str,
                h_before,
                h_after,
                s_before,
                s_after,
                it_str,
                el_str,
                rate_str,
                status_str,
            ) = m.groups()

            seed = int(seed_str)
            if (inst, seed) in seen:
                continue
            seen.add((inst, seed))

            inf_before = int(h_before)
            inf_after = int(h_after)
            obj_before = int(s_before)
            obj_after = int(s_after)
            it_done = int(it_str.replace(",", ""))
            elapsed_s = float(el_str)
            it_per_sec = float(rate_str.replace(",", ""))
            is_feasible = inf_after == 0

            # Parse feasibility iteration and seconds if feasible
            first_feas_iter: int | str = ""
            first_feas_sec: float | str = ""
            feas_m = re.search(r"w kroku\s+(\d+)(?:\s+\(([\d.]+)s\))?", status_str)
            if feas_m:
                first_feas_iter = int(feas_m.group(1))
                if feas_m.group(2):
                    first_feas_sec = float(feas_m.group(2))

            meta = INSTANCE_METADATA.get(
                inst,
                {
                    "events_count": 0,
                    "times_count": 0,
                    "resources_count": 0,
                    "constraints_count": 0,
                    "scale": "medium",
                    "tightness": "moderate",
                },
            )

            record = {
                "instance_id": inst,
                "mode": mode,
                "seed": seed,
                "iterations_limit": 500000,
                "iterations_done": it_done,
                "infeasibility_before": inf_before,
                "objective_before": obj_before,
                "infeasibility_after": inf_after,
                "objective_after": obj_after,
                "first_feasible_iteration": first_feas_iter,
                "first_feasible_seconds": first_feas_sec,
                "is_feasible": is_feasible,
                "elapsed_s": elapsed_s,
                "iterations_per_sec": it_per_sec,
                "events_count": meta["events_count"],
                "times_count": meta["times_count"],
                "resources_count": meta["resources_count"],
                "constraints_count": meta["constraints_count"],
                "scale": meta["scale"],
                "tightness": meta["tightness"],
                "selector": "ucb",
                "acceptance": "sa",
                "acceptance_detail": "sa",
            }
            rows.append(record)

    # Sort deterministically: by instance then seed
    rows.sort(key=lambda r: (str(r["instance_id"]), int(str(r["seed"]))))

    # Exact 23 columns from preprocessing_experiments.py
    fieldnames = [
        "instance_id",
        "mode",
        "seed",
        "iterations_limit",
        "iterations_done",
        "infeasibility_before",
        "objective_before",
        "infeasibility_after",
        "objective_after",
        "first_feasible_iteration",
        "first_feasible_seconds",
        "is_feasible",
        "elapsed_s",
        "iterations_per_sec",
        "events_count",
        "times_count",
        "resources_count",
        "constraints_count",
        "scale",
        "tightness",
        "selector",
        "acceptance",
        "acceptance_detail",
    ]

    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Odzyskano pomyślnie {len(rows)} wierszy z {out_path} -> {csv_path}")
    return len(rows)


def main() -> None:
    out_files = sorted(glob.glob("slurm_adaptive_*.out"))
    if not out_files:
        print("Nie znaleziono plikow slurm_adaptive_*.out w biezacym katalogu.")
        sys.exit(1)

    latest_out = out_files[-1]
    out_arg = sys.argv[1] if len(sys.argv) > 1 else latest_out
    recover_csv(out_arg)


if __name__ == "__main__":
    main()
