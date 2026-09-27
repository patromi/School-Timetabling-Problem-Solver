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
        "data/results/summary.md",
        expand("data/results/{instance}_validation.txt", instance=INSTANCES)


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

rule download_khe:
    output:
        "downloads/khe.tar.gz"
    shell:
        "uv run python -c \"import urllib.request; urllib.request.urlretrieve('http://jeffreykingston.id.au/khe/khe-2025_12_04.tar.gz', '{output}')\""

rule extract_khe:
    input:
        "downloads/khe.tar.gz"
    output:
        directory("downloads/khe")
    shell:
        "uv run python -c \"import tarfile; tar=tarfile.open('{input}'); tar.extractall('{output}'); tar.close()\""

rule compile_khe:
    input:
        "downloads/khe"
    output:
        "downloads/khe/khe-2025_12_04/src_hseval/hseval.cgi"
    shell:
        "wsl make -C downloads/khe/khe-2025_12_04 all FINAL_DIR=."

rule validate_solution:
    input:
        xml = "data/results/{instance}_solution.xml",
        khe = "downloads/khe/khe-2025_12_04/src_hseval/hseval.cgi"
    output:
        report = "data/results/{instance}_validation.txt"
    shell:
        "uv run python -m src.validator {input.xml} {output.report} --khe-path {input.khe}"
