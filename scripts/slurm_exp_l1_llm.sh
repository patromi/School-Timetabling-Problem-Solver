#!/bin/bash
# EXP-L1: UCB z pulą statyczną (llm=none) vs UCB z pulą rozszerzaną przez LLM
# (llm=a) — oba warianty w jednym zadaniu, na całym węźle obl, równolegle.
#
# Oba warianty biegną w tym samym procesie experiments.py, przemieszane w
# jednej puli workerów, więc dzielą identyczne warunki sprzętowe. Limit
# czasu dotyczy samego przeszukiwania: czas generatora LLM (zapytania +
# walidacja kodu) nie wlicza się do niego i ląduje osobno w kolumnie llm_s.
#
# Łączy się z serwerem vLLM uruchomionym przez slurm_vllm_hgx2.sh na hgx2.
#
# Użycie przez koordynator (zalecane):
#   bash scripts/submit_llm_experiments.sh -A <KONTO>
# Bezpośrednio (vLLM musi już działać):
#   sbatch -A <KONTO> scripts/slurm_exp_l1_llm.sh

#SBATCH -J exp_l1_llm
# węzły obl1/obl2 (CPU) są w partycji obl -- nie hgx, jak podaje instrukcja
#SBATCH -p obl
#SBATCH -w obl1
#SBATCH --exclusive
#SBATCH --cpus-per-task 128
#SBATCH --mem 256G
#SBATCH -t 03:00:00
#SBATCH -o slurm_exp_l1_%j.out
#SBATCH -e slurm_exp_l1_%j.err

set -euo pipefail

# ── konfiguracja ──────────────────────────────────────────────────────────
# katalog repo: ten, z którego zlecono sbatch (SLURM_SUBMIT_DIR), więc
# zlecaj z katalogu projektu; PROJ=... nadpisuje
PROJ="${PROJ:-$SLURM_SUBMIT_DIR}"
LOG_DIR="$PROJ/data/logs_exp_l1/${SLURM_JOB_ID}"
RESULTS_CSV="$PROJ/data/results/exp_l1.csv"

VLLM_HOST="${VLLM_HOST:-hgx2}"
VLLM_PORT="${VLLM_PORT:-8000}"
VLLM_MODEL="${VLLM_MODEL:-Qwen/Qwen2.5-Coder-32B-Instruct}"

# 10 instancji w zakresie pracy (zasoby przypisane z góry), od małych do
# dużych; zob. analiza instancji w opisie issue #46.
INSTANCES="${EXP_INSTANCES:-BR-SA-00 BR-SN-00 FI-WP-06 FI-MP-06 GR-P3-10 GR-PA-08 GR-H1-97 ZA-LW-09 ZA-WD-09 IT-I4-96}"
SEEDS="${EXP_SEEDS:-0 1 2 3 4 5 6 7 8 9}"
TIME_LIMIT="${EXP_TIME_LIMIT:-300}"   # sekundy przeszukiwania na przebieg
LLM_TRIGGER="${LLM_TRIGGER:-3}"
# 96 ze 128 rdzeni: reszta zostaje dla podprocesów sandboxa i walidacji
WORKERS="${EXP_WORKERS:-96}"

# ── nagłówek ──────────────────────────────────────────────────────────────
echo "=== EXP-L1: UCB bez LLM vs UCB + LLM (PUT cluster) ==="
echo "Node      : $(hostname)  ($(nproc) CPU)"
echo "Date      : $(date)"
echo "Job ID    : $SLURM_JOB_ID"
echo "Project   : $PROJ  ($(git -C "$PROJ" rev-parse --short HEAD))"
echo "LogDir    : $LOG_DIR"
echo "vLLM      : http://${VLLM_HOST}:${VLLM_PORT}/v1 (model=$VLLM_MODEL)"
echo "instances : $INSTANCES"
echo "seeds     : $SEEDS"
echo "time-limit: ${TIME_LIMIT}s przeszukiwania / przebieg"
echo "workers   : $WORKERS"
echo "llm-trigger: ${LLM_TRIGGER}x"
echo ""

mkdir -p "$LOG_DIR" "$PROJ/data/results"

# ── czekaj na gotowość vLLM ───────────────────────────────────────────────
echo "Sprawdzam dostępność vLLM na http://${VLLM_HOST}:${VLLM_PORT}/health ..."
WAITED=0
until curl -sf "http://${VLLM_HOST}:${VLLM_PORT}/health" &>/dev/null; do
    sleep 15
    WAITED=$(( WAITED + 15 ))
    if (( WAITED >= 1800 )); then
        echo "BŁĄD: vLLM nie odpowiedział po 30 minutach." >&2
        exit 1
    fi
    echo "  ... czekam (${WAITED}s)"
done
echo "vLLM gotowy po ${WAITED}s."
echo ""

# ── backend LLM ───────────────────────────────────────────────────────────
export LLM_BACKEND="openai"
export OPENAI_BASE_URL="http://${VLLM_HOST}:${VLLM_PORT}/v1"
export OPENAI_MODEL="$VLLM_MODEL"
export OPENAI_API_KEY="EMPTY"

cd "$PROJ"

# shellcheck disable=SC2086
uv run python experiments.py \
    --selectors ucb \
    --llm-variants none a \
    --instances $INSTANCES \
    --seeds $SEEDS \
    --time-limit "$TIME_LIMIT" \
    --workers "$WORKERS" \
    --llm-trigger "$LLM_TRIGGER" \
    --log-dir "$LOG_DIR" \
    --output "$RESULTS_CSV"

echo ""
echo "=== Zakończono EXP-L1 ==="

echo "Logi JSONL : $LOG_DIR/"
echo "CSV        : $RESULTS_CSV"
