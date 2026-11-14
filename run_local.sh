#!/usr/bin/env bash
# run_local.sh — uruchamia eksperymenty LLM lokalnie.
#
# Zakłada że baseline (random/epsilon-greedy/ucb bez LLM, 10 seedów, 120s)
# jest już gotowy w data/logs_v2/.
#
# Użycie:
#   bash run_local.sh              # wszystkie eksperymenty, domyślne parametry
#   bash run_local.sh --quick      # 3 seedy, 60s — szybki test
#   bash run_local.sh --exp l1     # tylko EXP-L1
#   bash run_local.sh --exp l2     # tylko EXP-L2
#   TIME_LIMIT=300 bash run_local.sh   # dłuższy budżet
#
# Wymagania:
#   - uv (https://docs.astral.sh/uv/)
#   - Ollama uruchomione lokalnie: ollama serve
#   - Model: ollama pull qwen2.5-coder:32b (lub ustaw OLLAMA_MODEL)

set -euo pipefail

# ── konfiguracja (nadpisywalna przez env) ──────────────────────────────────
TIME_LIMIT=${TIME_LIMIT:-120}
SEEDS_FULL="0 1 2 3 4 5 6 7 8 9"
SEEDS_QUICK="0 1 2"
INSTANCES="BR-SA-00 ZA-WD-09 AU-SA-96"
LLM_TRIGGER=${LLM_TRIGGER:-3}
LOG_DIR_LLM="data/logs_llm"
RESULTS_DIR="data/results"

# ── parsowanie argumentów ──────────────────────────────────────────────────
QUICK=0
EXP="all"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --quick)   QUICK=1; shift ;;
        --exp)     EXP="$2"; shift 2 ;;
        *)         echo "Nieznany argument: $1" >&2; exit 1 ;;
    esac
done

if [[ $QUICK -eq 1 ]]; then
    SEEDS="$SEEDS_QUICK"
    TIME_LIMIT=$(( TIME_LIMIT < 60 ? TIME_LIMIT : 60 ))
    echo "Tryb QUICK: seeds=[${SEEDS}], time-limit=${TIME_LIMIT}s"
else
    SEEDS="$SEEDS_FULL"
fi

# ── sprawdzenie środowiska ─────────────────────────────────────────────────
echo "=== Sprawdzanie środowiska ==="

if ! command -v uv &>/dev/null; then
    echo "BŁĄD: uv nie znalezione. Zainstaluj: https://docs.astral.sh/uv/" >&2
    exit 1
fi

# Sprawdź czy Ollama odpowiada (opcjonalne — eksperymenty bez LLM nie potrzebują)
OLLAMA_OK=1
if ! curl -sf "${OLLAMA_HOST:-http://localhost:11434}/api/tags" &>/dev/null; then
    echo "UWAGA: Ollama niedostępna pod ${OLLAMA_HOST:-http://localhost:11434}."
    echo "       Eksperymenty LLM będą zawodziły. Uruchom: ollama serve"
    OLLAMA_OK=0
fi

echo "  time-limit : ${TIME_LIMIT}s"
echo "  seeds      : ${SEEDS}"
echo "  instancje  : ${INSTANCES}"
echo "  llm-trigger: ${LLM_TRIGGER}x"
echo "  ollama     : $([ $OLLAMA_OK -eq 1 ] && echo OK || echo NIEDOSTEPNA)"
echo ""

mkdir -p "$LOG_DIR_LLM" "$RESULTS_DIR"

# ── pomocnicze ────────────────────────────────────────────────────────────
run_exp() {
    local label="$1"; shift
    echo ""
    echo "──────────────────────────────────────────────────────"
    echo "  $label"
    echo "──────────────────────────────────────────────────────"
    uv run python experiments.py "$@"
}

# ── EXP-L1: UCB static vs UCB + LLM pool ─────────────────────────────────
# Pytanie: czy LLM generujący heurystyki pomaga UCB?
# Baseline (ucb/none) już gotowy w data/logs_v2/; tu tylko warianty LLM.
exp_l1() {
    if [[ $OLLAMA_OK -eq 0 ]]; then
        echo "POMIJAM EXP-L1 — Ollama niedostępna." >&2
        return
    fi
    run_exp "EXP-L1: UCB + LLM pool (wariant A) vs statyczna pula" \
        --selectors ucb \
        --llm-variants a \
        --instances $INSTANCES \
        --seeds $SEEDS \
        --time-limit "$TIME_LIMIT" \
        --workers 1 \
        --llm-trigger "$LLM_TRIGGER" \
        --log-dir "$LOG_DIR_LLM" \
        --output "$RESULTS_DIR/exp_l1_a.csv"
}

# ── EXP-L2: porównanie wariantów promptu ──────────────────────────────────
# Pytanie: A vs B vs RAG — który daje lepsze heurystyki?
exp_l2() {
    if [[ $OLLAMA_OK -eq 0 ]]; then
        echo "POMIJAM EXP-L2 — Ollama niedostępna." >&2
        return
    fi
    run_exp "EXP-L2: warianty promptu A vs B vs RAG" \
        --selectors ucb \
        --llm-variants a b rag \
        --instances $INSTANCES \
        --seeds $SEEDS \
        --time-limit "$TIME_LIMIT" \
        --workers 1 \
        --llm-trigger "$LLM_TRIGGER" \
        --log-dir "$LOG_DIR_LLM" \
        --output "$RESULTS_DIR/exp_l2_variants.csv"
}

# ── wybór eksperymentów ────────────────────────────────────────────────────
case "$EXP" in
    l1)   exp_l1 ;;
    l2)   exp_l2 ;;
    all)
        exp_l1
        exp_l2
        ;;
    *)
        echo "Nieznany eksperyment: $EXP (dostępne: l1 l2 all)" >&2
        exit 1
        ;;
esac

echo ""
echo "=== Zakończono. Wyniki w: $RESULTS_DIR/ ==="
echo "    Logi JSONL: $LOG_DIR_LLM/"
echo ""
echo "Następny krok — analiza i wykresy:"
echo "    uv run python analyse_results.py"
echo "    uv run python plot_results.py"
