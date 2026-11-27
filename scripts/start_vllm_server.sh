#!/bin/bash
# Uruchamia serwer vLLM w sesji tmux "vllm".
#
# Użycie (na maszynie GPU):
#   bash scripts/start_vllm_server.sh                        # Qwen2.5-Coder-7B
#   VLLM_MODEL=Qwen/Qwen2.5-Coder-32B-Instruct bash scripts/start_vllm_server.sh
#   VLLM_PORT=8001 bash scripts/start_vllm_server.sh
#
# Zatrzymaj serwer:
#   tmux kill-session -t vllm
#
# Logi na żywo:
#   tmux attach -t vllm   (odepnij: Ctrl+B D)

set -euo pipefail

MODEL="${VLLM_MODEL:-Qwen/Qwen2.5-Coder-7B-Instruct}"
PORT="${VLLM_PORT:-8000}"
VENV_DIR="$HOME/.venvs/vllm"
READY_FILE="$HOME/.vllm_ready"
SESSION="vllm"

# ── sprawdź środowisko ───────────────────────────────────────────────────
if [[ ! -f "$VENV_DIR/bin/python" ]]; then
    echo "BŁĄD: venv $VENV_DIR nie istnieje. Uruchom najpierw: bash scripts/setup_gpu_machine.sh" >&2
    exit 1
fi

if ! command -v tmux &>/dev/null; then
    echo "BŁĄD: tmux nie zainstalowany. Zainstaluj: sudo apt install tmux" >&2
    exit 1
fi

# Zabij starą sesję jeśli istnieje
tmux kill-session -t "$SESSION" 2>/dev/null || true
rm -f "$READY_FILE"

echo "=== Uruchamianie vLLM ==="
echo "Model   : $MODEL"
echo "Port    : $PORT"
echo "VENV    : $VENV_DIR"
echo "Sesja   : tmux:$SESSION"
echo ""

# ── skrypt uruchamiany wewnątrz tmux ────────────────────────────────────
INNER=$(cat <<TMUX_SCRIPT
set -euo pipefail
READY_FILE="$READY_FILE"
cleanup() { rm -f "\$READY_FILE"; }
trap cleanup EXIT TERM INT

echo "GPU: \$(nvidia-smi --query-gpu=name,memory.total --format=csv,noheader 2>/dev/null || echo 'brak nvidia-smi')"
echo "Startuję vLLM..."

"$VENV_DIR/bin/python" -m vllm.entrypoints.openai.api_server \\
    --model "$MODEL" \\
    --port "$PORT" \\
    --host 0.0.0.0 \\
    --dtype auto \\
    --max-model-len 4096 \\
    --gpu-memory-utilization 0.90 \\
    --trust-remote-code &

VLLM_PID=\$!
echo "PID vLLM: \$VLLM_PID"

# Czekaj na gotowość (maks 10 min)
for i in \$(seq 1 120); do
    if curl -sf "http://localhost:$PORT/health" &>/dev/null; then
        echo "=== vLLM gotowy (próba \$i) ==="
        touch "\$READY_FILE"
        echo "\$VLLM_PID" >> "\$READY_FILE"
        break
    fi
    sleep 5
done

if [[ ! -f "\$READY_FILE" ]]; then
    echo "BŁĄD: vLLM nie odpowiedział po 10 minutach." >&2
    kill "\$VLLM_PID" 2>/dev/null || true
    exit 1
fi

# Trzymaj proces — tmux zakończy sesję gdy tu wyjdziemy
wait "\$VLLM_PID"
TMUX_SCRIPT
)

_inner_script=$(mktemp /tmp/vllm_inner_XXXXXX.sh)
printf '%s\n' "$INNER" > "$_inner_script"
tmux new-session -d -s "$SESSION" "bash $_inner_script; rm -f $_inner_script"

echo "Sesja tmux '$SESSION' uruchomiona."
echo "Czekam na gotowość serwera (maks 10 min)..."

for i in $(seq 1 120); do
    if [[ -f "$READY_FILE" ]]; then
        echo ""
        echo "=== Serwer vLLM gotowy ==="
        echo "URL : http://$(hostname -I | awk '{print $1}'):$PORT/v1"
        echo "   lub jeśli maszyna CPU może się dostać przez hostname:"
        echo "URL : http://$(hostname):$PORT/v1"
        echo ""
        echo "Na maszynie CPU ustaw:"
        echo "  export LLM_BACKEND=openai"
        echo "  export OPENAI_BASE_URL=http://$(hostname):$PORT/v1"
        echo "  export OPENAI_MODEL=$MODEL"
        echo "  export OPENAI_API_KEY=EMPTY"
        echo ""
        echo "Logi: tmux attach -t $SESSION  (odepnij: Ctrl+B D)"
        exit 0
    fi
    if ! tmux has-session -t "$SESSION" 2>/dev/null; then
        echo "BŁĄD: sesja tmux zakończyła się przedwcześnie." >&2
        exit 1
    fi
    printf "."
    sleep 5
done

echo ""
echo "BŁĄD: Timeout — serwer nie odpowiedział po 10 min." >&2
echo "Sprawdź logi: tmux attach -t $SESSION" >&2
exit 1
