import os

# Automatically detect XHSTT instances in data/raw
ALL_INSTANCES, = glob_wildcards("data/raw/{instance}.xml")
# Filter out empty files or placeholders
INSTANCES = [
    inst for inst in ALL_INSTANCES
    if inst != "instance" and os.path.getsize(f"data/raw/{inst}.xml") > 0
]

# Default run parameters
ITERATIONS = os.environ.get("SOLVER_ITERATIONS", "30000")
SEED = os.environ.get("SOLVER_SEED", "0")

rule all:
    input:
        "data/results/summary.csv",
        "data/results/summary.md"

rule run_solver:
    input:
        xml = "data/raw/{instance}.xml"
    output:
        solution = "data/results/{instance}_solution.xml",
        html = "data/results/{instance}_timetable.html"
    params:
        iterations = ITERATIONS,
        seed = SEED
    log:
        "data/results/{instance}_solver.log"
    shell:
        "uv run python run_solver.py --archive {input.xml} --output {output.solution} --iterations {params.iterations} --seed {params.seed} > {log} 2>&1"

rule generate_summary:
    input:
        instances = expand("data/raw/{instance}.xml", instance=INSTANCES),
        solutions = expand("data/results/{instance}_solution.xml", instance=INSTANCES)
    output:
        csv = "data/results/summary.csv",
        md = "data/results/summary.md"
    shell:
        "uv run python scripts/generate_summary.py --instances {input.instances} --solutions {input.solutions} --csv-output {output.csv} --md-output {output.md}"
