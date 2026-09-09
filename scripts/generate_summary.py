#!/usr/bin/env python
"""Script to evaluate solution files and generate CSV/Markdown summaries."""

import argparse
import sys
from pathlib import Path

# Add project root to sys.path
sys.path.insert(0, str(Path(__file__).parent.parent.resolve()))

import csv
from typing import Any

from src.evaluator_ref import evaluate_cost_components
from src.model import Instance
from src.parser import parse_archive, parse_solution_groups


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate school timetabling run summaries."
    )
    parser.add_argument(
        "--instances", nargs="+", required=True, help="List of instance XML files."
    )
    parser.add_argument(
        "--solutions", nargs="+", required=True, help="List of solution XML files."
    )
    parser.add_argument(
        "--csv-output", type=Path, required=True, help="Output path for CSV summary."
    )
    parser.add_argument(
        "--md-output",
        type=Path,
        required=True,
        help="Output path for Markdown summary.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    # Load all instances
    instances_by_id: dict[str, Instance] = {}
    for inst_path in args.instances:
        p = Path(inst_path)
        if not p.exists():
            continue
        try:
            parsed = parse_archive(p.read_text(encoding="utf-8-sig"))
            for inst in parsed:
                instances_by_id[inst.id] = inst
        except Exception as e:
            print(f"Error parsing instance file {inst_path}: {e}")

    # Process all solutions
    results: list[dict[str, Any]] = []
    for sol_path in args.solutions:
        p = Path(sol_path)
        if not p.exists():
            print(f"Solution file does not exist: {sol_path}")
            continue
        try:
            xml_text = p.read_text(encoding="utf-8")
            groups = parse_solution_groups(xml_text)
            if not groups:
                print(f"No solution groups found in {sol_path}")
                continue

            for group in groups:
                for solution in group.solutions:
                    inst_id = solution.instance_ref
                    if inst_id not in instances_by_id:
                        print(
                            f"Warning: Instance {inst_id} matching solution in {sol_path} not found in instances."
                        )
                        continue

                    instance = instances_by_id[inst_id]
                    infeasibility, objective = evaluate_cost_components(instance, solution)
                    feasible = infeasibility == 0

                    results.append(
                        {
                            "Instance": inst_id,
                            "Name": instance.name,
                            "Status": "FEASIBLE" if feasible else "INFEASIBLE",
                            "Infeasibility": infeasibility,
                            "Objective": objective,
                            "SolutionGroup": group.id,
                            "File": p.name,
                        }
                    )
        except Exception as e:
            print(f"Error processing solution file {sol_path}: {e}")

    # Sort results by Instance ID
    results.sort(key=lambda x: x["Instance"])

    # Write CSV
    args.csv_output.parent.mkdir(parents=True, exist_ok=True)
    with open(args.csv_output, mode="w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "Instance",
                "Name",
                "Status",
                "Infeasibility",
                "Objective",
                "SolutionGroup",
                "File",
            ],
        )
        writer.writeheader()
        writer.writerows(results)
    print(f"Saved CSV summary to {args.csv_output}")

    # Write Markdown
    args.md_output.parent.mkdir(parents=True, exist_ok=True)
    with open(args.md_output, mode="w", encoding="utf-8") as f:
        f.write("# Timetabling Run Summary\n\n")
        f.write(
            "| Instance | Name | Status | Infeasibility | Objective | Solution Group | File |\n"
        )
        f.write("| --- | --- | --- | --- | --- | --- | --- |\n")
        for r in results:
            f.write(
                f"| {r['Instance']} | {r['Name']} | **{r['Status']}** | {r['Infeasibility']:,} | {r['Objective']:,} | {r['SolutionGroup']} | `{r['File']}` |\n"
            )
    print(f"Saved Markdown summary to {args.md_output}")


if __name__ == "__main__":
    main()
