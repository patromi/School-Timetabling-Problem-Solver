#!/bin/bash
# Uruchamia serwer vLLM na hgx2 (1x A100-80GB).
# Serwer żyje przez cały czas trwania eksperymentu i jest zabijany przez SLURM.
#
# Użycie:
#   sbatch -A <KONTO> scripts/slurm_vllm_hgx2.sh
#   # lub przez submit_llm_experiments.sh

#SBATCH -J vllm_server
#SBATCH -p hgx
#SBATCH -w hgx2
#SBATCH --gres=gpu:1
#SBATCH --cpus-per-task 16
#SBATCH --mem 80G
#SBATCH -t 12:00:00
#SBATCH -o slurm_vllm_%j.out
#SBATCH -e slurm_vllm_%j.err

set -euo pipefail

# ── konfiguracja ──────────────────────────────────────────────────────────
# Nazwa modelu dokładnie taka jak w huggingface lub lokalnej ścieżce.
# Musi pasować do OPENAI_MODEL w slurm_exp_l1_llm.sh.
MODEL="${VLLM_MODEL:-Qwen/Qwen2.5-Coder-32B-Instruct}"
PORT="${VLLM_PORT:-8000}"
# Plik sygnalizujący że serwer jest gotowy — odczytywany przez job eksperymentu.
READY_FILE="/home/$USER/.vllm_ready_${SLURM_JOB_ID}"

# ── środowisko ────────────────────────────────────────────────────────────
# Aktywuj środowisko z zainstalowanym vllm.
# Zakładamy conda env "vllm"; zmień jeśli inaczej.
source ~/miniconda3/bin/activate vllm 2>/dev/null \
    || source ~/miniconda3/etc/profile.d/conda.sh && conda activate vllm

echo "=== vLLM server ==="
echo "Node    : $(hostname)"
echo "Date    : $(date)"
echo "Model   : $MODEL"
echo "Port    : $PORT"
echo "Job ID  : $SLURM_JOB_ID"
echo "GPU     : $(nvidia-smi --query-gpu=name,memory.total --format=csv,noheader)"
echo ""

# ── czyszczenie po zakończeniu ────────────────────────────────────────────
cleanup() {
    echo "vLLM: otrzymano sygnał zakończenia, sprzątam..."
    rm -f "$READY_FILE"
}
trap cleanup EXIT TERM INT

# ── start serwera ─────────────────────────────────────────────────────────
python -m vllm.entrypoints.openai.api_server \
    --model "$MODEL" \
    --port "$PORT" \
    --host 0.0.0.0 \
    --dtype auto \
    --max-model-len 4096 \
    --gpu-memory-utilization 0.90 \
    --max-num-seqs 128 \
    --trust-remote-code \
    &

VLLM_PID=$!
echo "vLLM PID: $VLLM_PID"

# ── czekaj aż serwer odpowie na health check ──────────────────────────────
echo "Czekam na gotowość serwera (max 10 min)..."
WAITED=0
until curl -sf "http://localhost:${PORT}/health" &>/dev/null; do
    sleep 10
    WAITED=$(( WAITED + 10 ))
    if (( WAITED >= 600 )); then
        echo "BŁĄD: serwer nie odpowiedział po 10 minutach." >&2
        kill "$VLLM_PID" 2>/dev/null || true
        exit 1
    fi
done

echo "Serwer gotowy po ${WAITED}s. Zapisuję sygnał: $READY_FILE"
echo "$(hostname):${PORT}" > "$READY_FILE"

# ── czekaj aż SLURM zakończy job ─────────────────────────────────────────
wait "$VLLM_PID"
