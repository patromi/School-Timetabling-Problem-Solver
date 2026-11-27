#!/bin/bash
# Eksperymenty bez LLM (CPU-only): baseline + UCB, 3 instancje, 10 seedów.
#
# Użycie:
#   bash scripts/run_local_experiments.sh
#   SEEDS=5 WORKERS=4 bash scripts/run_local_experiments.sh
#   INSTANCES="BR-SA-00" TIME=120 bash scripts/run_local_experiments.sh

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

INSTANCES="${INSTANCES:-BR-SA-00 ZA-WD-09 AU-SA-96}"
SEEDS="${SEEDS:-10}"
TIME="${TIME:-300}"        # sekund na jeden przebieg
WORKERS="${WORKERS:-4}"    # równoległych jobów (dobierz do liczby rdzeni)
OUTPUT="${OUTPUT:-data/logs}"

echo "=== Eksperymenty lokalne (bez LLM) ==="
echo "Instancje : $INSTANCES"
echo "Seedów    : $SEEDS (0..$(( SEEDS - 1 )))"
echo "Czas/run  : ${TIME}s"
echo "Równolegle: $WORKERS"
echo "Wyniki    : $OUTPUT"
echo ""

# shellcheck disable=SC2086
uv run python experiments.py \
    --selectors random ucb \
    --llm-variants none \
    --instances $INSTANCES \
    --seeds $(seq 0 $(( SEEDS - 1 ))) \
    --time-limit "$TIME" \
    --workers "$WORKERS" \
    --output "$OUTPUT"

echo ""
echo "Gotowe. Logi zapisane w: $OUTPUT"
echo "Aby wygenerować wykresy:"
echo "  uv run python plot_results.py --log-dir $OUTPUT"
