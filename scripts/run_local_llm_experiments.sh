#!/bin/bash
# Eksperymenty z LLM — łączy się do serwera vLLM na osobnej maszynie GPU.
#
# Wymagania:
#   - serwer vLLM uruchomiony na maszynie GPU (bash scripts/start_vllm_server.sh)
#   - zmienna LLM_HOST ustawiona na hostname lub IP maszyny GPU
#
# Użycie:
#   LLM_HOST=192.168.1.42 bash scripts/run_local_llm_experiments.sh
#   LLM_HOST=gpu-machine  bash scripts/run_local_llm_experiments.sh
#   LLM_HOST=gpu-machine  LLM_PORT=8001 bash scripts/run_local_llm_experiments.sh
#
# Opcje przez zmienne środowiskowe:
#   LLM_HOST      - hostname/IP maszyny GPU (wymagany)
#   LLM_PORT      - port serwera vLLM (domyślnie 8000)
#   LLM_MODEL     - nazwa modelu (musi pasować do serwera)
#   INSTANCES     - instancje XHSTT oddzielone spacją
#   SEEDS         - liczba seedów
#   TIME          - sekund na jeden przebieg
#   WORKERS       - równoległych jobów (ostrożnie — każdy może wywoływać LLM)
#   OUTPUT_DIR    - katalog wyjściowy

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

LLM_HOST="${LLM_HOST:-}"
LLM_PORT="${LLM_PORT:-8000}"
LLM_MODEL="${LLM_MODEL:-Qwen/Qwen2.5-Coder-7B-Instruct}"
INSTANCES="${INSTANCES:-BR-SA-00 ZA-WD-09 AU-SA-96}"
SEEDS="${SEEDS:-10}"
TIME="${TIME:-300}"
WORKERS="${WORKERS:-2}"   # mniej workerów — każdy woła LLM
OUTPUT="${OUTPUT:-data/logs}"

# ── sprawdź LLM_HOST ────────────────────────────────────────────────────
if [[ -z "$LLM_HOST" ]]; then
    echo "BŁĄD: podaj adres maszyny GPU przez LLM_HOST." >&2
    echo "Przykład: LLM_HOST=192.168.1.42 bash $0" >&2
    exit 1
fi

LLM_URL="http://${LLM_HOST}:${LLM_PORT}/v1"

echo "=== Eksperymenty lokalne z LLM ==="
echo "Instancje : $INSTANCES"
echo "Seedów    : $SEEDS (0..$(( SEEDS - 1 )))"
echo "Czas/run  : ${TIME}s"
echo "Równolegle: $WORKERS"
echo "LLM URL   : $LLM_URL"
echo "LLM model : $LLM_MODEL"
echo "Wyniki    : $OUTPUT"
echo ""

# ── sprawdź dostępność serwera ───────────────────────────────────────────
echo "Sprawdzam dostępność serwera vLLM..."
if ! curl -sf "${LLM_URL%/v1}/health" &>/dev/null; then
    echo "BŁĄD: Serwer vLLM nie odpowiada pod $LLM_URL" >&2
    echo ""
    echo "Na maszynie GPU uruchom:"
    echo "  bash scripts/start_vllm_server.sh"
    echo ""
    echo "Jeśli serwer dopiero startuje, poczekaj i spróbuj ponownie."
    exit 1
fi
echo "Serwer vLLM: OK"
echo ""

# ── konfiguracja LLM ────────────────────────────────────────────────────
export LLM_BACKEND="openai"
export OPENAI_BASE_URL="$LLM_URL"
export OPENAI_MODEL="$LLM_MODEL"
export OPENAI_API_KEY="EMPTY"

# ── uruchom eksperymenty ─────────────────────────────────────────────────
# shellcheck disable=SC2086
uv run python experiments.py \
    --selectors ucb \
    --llm-variants a \
    --instances $INSTANCES \
    --seeds $(seq 0 $(( SEEDS - 1 ))) \
    --time-limit "$TIME" \
    --workers "$WORKERS" \
    --output "$OUTPUT"

echo ""
echo "Gotowe. Logi zapisane w: $OUTPUT"
echo "Aby wygenerować wykresy:"
echo "  uv run python plot_results.py --log-dir $OUTPUT"
