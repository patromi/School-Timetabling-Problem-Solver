INSTANCES, = glob_wildcards("data/raw/{instance}.xml")

rule all:
    input:
        expand("data/results/{instance}_solution.xml", instance=INSTANCES)

rule parse_xml:
    input:
        "data/raw/{instance}.xml"
    output:
        "data/processed/{instance}_parsed.json"
    shell:
        "echo 'Parsowanie pliku {input} (instancja: {wildcards.instance})...' > {output}"

rule run_solver:
    input:
        "data/processed/{instance}_parsed.json"
    output:
        "data/results/{instance}_solution.xml"
    shell:
        "echo 'Generowanie rozwiązania dla {wildcards.instance} na bazie {input}...' > {output}"