#!/bin/bash
# EXP-L1: UCB static vs UCB + LLM pool — uruchamiane na obl.
# Łączy się z serwerem vLLM uruchomionym przez slurm_vllm_hgx2.sh na hgx2.
#
# Użycie (bezpośrednie):
#   VLLM_JOB_ID=<id> sbatch -A <KONTO> scripts/slurm_exp_l1_llm.sh
# Użycie przez koordynator:
#   bash scripts/submit_llm_experiments.sh -A <KONTO>

#SBATCH -J exp_l1_llm
#SBATCH -p hgx
#SBATCH -w obl1
#SBATCH --cpus-per-task 8
#SBATCH --mem 24G
#SBATCH -t 06:00:00
#SBATCH -o slurm_exp_l1_%j.out
#SBATCH -e slurm_exp_l1_%j.err

set -euo pipefail

# ── konfiguracja ──────────────────────────────────────────────────────────
PROJ="$HOME/PycharmProjects/LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem"
LOG_DIR="/raid/$USER/exp_l1_llm_logs_${SLURM_JOB_ID}"
RESULTS_CSV="${LOG_DIR}/exp_l1_llm.csv"

# vLLM — hostname hgx2, port 8000.
# Job ID vLLM-a przekazywany przez submit_llm_experiments.sh jako VLLM_JOB_ID;
# używamy go tylko do znalezienia pliku sygnałowego.
VLLM_HOST="${VLLM_HOST:-hgx2}"
VLLM_PORT="${VLLM_PORT:-8000}"
VLLM_MODEL="${VLLM_MODEL:-Qwen/Qwen2.5-Coder-32B-Instruct}"

# Parametry eksperymentu
INSTANCES="${EXP_INSTANCES:-BR-SA-00 ZA-WD-09 AU-SA-96}"
SEEDS="${EXP_SEEDS:-0 1 2 3 4 5 6 7 8 9}"
TIME_LIMIT="${EXP_TIME_LIMIT:-300}"   # 5 min na przebieg, zmień wg potrzeb
LLM_TRIGGER="${LLM_TRIGGER:-3}"

# ── nagłówek ──────────────────────────────────────────────────────────────
echo "=== EXP-L1 LLM (PUT cluster) ==="
echo "Node    : $(hostname)"
echo "Date    : $(date)"
echo "Job ID  : $SLURM_JOB_ID"
echo "Project : $PROJ"
echo "LogDir  : $LOG_DIR"
echo "vLLM    : http://${VLLM_HOST}:${VLLM_PORT}/v1 (model=$VLLM_MODEL)"
echo ""

mkdir -p "$LOG_DIR"

# ── czekaj na gotowość vLLM ───────────────────────────────────────────────
echo "Sprawdzam dostępność vLLM na http://${VLLM_HOST}:${VLLM_PORT}/health ..."
WAITED=0
until curl -sf "http://${VLLM_HOST}:${VLLM_PORT}/health" &>/dev/null; do
    sleep 15
    WAITED=$(( WAITED + 15 ))
    if (( WAITED >= 900 )); then
        echo "BŁĄD: vLLM nie odpowiedział po 15 minutach." >&2
        exit 1
    fi
    echo "  ... czekam (${WAITED}s)"
done
echo "vLLM gotowy po ${WAITED}s."
echo ""

# ── ustaw zmienne środowiskowe dla backendu LLM ───────────────────────────
export LLM_BACKEND="openai"
export OPENAI_BASE_URL="http://${VLLM_HOST}:${VLLM_PORT}/v1"
export OPENAI_MODEL="$VLLM_MODEL"
export OPENAI_API_KEY="EMPTY"

# ── przejdź do projektu i aktywuj środowisko uv ───────────────────────────
cd "$PROJ"
# uv nie wymaga aktywacji — używa wirtualnego env automatycznie

echo "=== Faza 1: EXP-L1 — UCB + LLM wariant A ==="
echo "instances : $INSTANCES"
echo "seeds     : $SEEDS"
echo "time-limit: ${TIME_LIMIT}s"
echo "llm-trigger: ${LLM_TRIGGER}x"
echo ""

# shellcheck disable=SC2086
uv run python experiments.py \
    --selectors ucb \
    --llm-variants a \
    --instances $INSTANCES \
    --seeds $SEEDS \
    --time-limit "$TIME_LIMIT" \
    --workers 1 \
    --llm-trigger "$LLM_TRIGGER" \
    --log-dir "$LOG_DIR" \
    --output "$RESULTS_CSV"

echo ""
echo "=== Zakończono EXP-L1 LLM ==="
echo "Wyniki CSV : $RESULTS_CSV"
echo "Logi JSONL : $LOG_DIR/"
echo ""

# ── skopiuj wyniki z /raid do /home (współdzielony NFS) ───────────────────
DEST="$PROJ/data/logs_llm"
mkdir -p "$DEST"
cp -r "$LOG_DIR"/. "$DEST/"
mkdir -p "$PROJ/data/results"
cp "$RESULTS_CSV" "$PROJ/data/results/exp_l1_llm.csv"

echo "Wyniki skopiowane do: $DEST"
echo "CSV skopiowane do   : $PROJ/data/results/exp_l1_llm.csv"
