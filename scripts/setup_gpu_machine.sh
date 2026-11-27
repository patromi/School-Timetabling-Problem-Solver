#!/bin/bash
# Konfiguracja maszyny GPU do uruchamiania serwera vLLM.
#
# Wymagania sprzętowe: NVIDIA GPU (min. 24 GB VRAM dla Qwen2.5-Coder-7B,
#                      40 GB+ dla 32B), sterowniki NVIDIA + CUDA 12+.
#
# Uruchom raz na maszynie GPU:
#   bash scripts/setup_gpu_machine.sh
#
# Potem uruchamiaj serwer przez:
#   bash scripts/start_vllm_server.sh

set -euo pipefail

MODEL="${VLLM_MODEL:-Qwen/Qwen2.5-Coder-7B-Instruct}"   # mniejszy model dla GPU < 40 GB
VENV_DIR="$HOME/.venvs/vllm"

echo "=== Konfiguracja środowiska vLLM ==="
echo "Maszyna : $(hostname)"
echo "Data    : $(date)"
echo "Model   : $MODEL"
echo ""

# ── sprawdź CUDA ─────────────────────────────────────────────────────────
if ! command -v nvidia-smi &>/dev/null; then
    echo "BŁĄD: nvidia-smi nie znaleziony. Zainstaluj sterowniki NVIDIA." >&2
    exit 1
fi
echo "GPU:"
nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader
echo ""

# ── sprawdź / zainstaluj uv ──────────────────────────────────────────────
if ! command -v uv &>/dev/null; then
    echo "Instaluję uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.cargo/bin:$PATH"
fi
echo "uv: $(uv --version)"

# ── wirtualne środowisko dla vLLM ────────────────────────────────────────
# vLLM ma zależności kolidujące z solverem — trzymamy je oddzielnie.
if [[ ! -d "$VENV_DIR" ]]; then
    echo "Tworzę venv dla vLLM: $VENV_DIR"
    uv venv "$VENV_DIR" --python 3.12
fi

echo "Instaluję vLLM (może potrwać kilka minut)..."
"$VENV_DIR/bin/pip" install --upgrade pip
"$VENV_DIR/bin/pip" install vllm

echo ""
echo "Sprawdzam instalację vLLM..."
"$VENV_DIR/bin/python" -c "import vllm; print(f'vLLM {vllm.__version__} OK')"

# ── pobierz model z Hugging Face ─────────────────────────────────────────
echo ""
echo "Pobieram model $MODEL z Hugging Face..."
echo "(Wymaga zalogowania: huggingface-cli login  jeśli model jest prywatny)"
"$VENV_DIR/bin/python" -c "
from huggingface_hub import snapshot_download
snapshot_download('$MODEL', repo_type='model', local_dir_use_symlinks=True)
print('Model pobrany / już istnieje w cache HF.')
" || echo "OSTRZEŻENIE: pobranie modelu nie powiodło się — zostanie pobrane przy pierwszym starcie."

# ── sprawdź tmux ─────────────────────────────────────────────────────────
if ! command -v tmux &>/dev/null; then
    echo ""
    echo "OSTRZEŻENIE: tmux nie zainstalowany."
    echo "Zainstaluj: sudo apt install tmux  (Ubuntu/Debian)  lub  sudo dnf install tmux"
fi

echo ""
echo "=== Setup GPU zakończony ==="
echo ""
echo "Aby uruchomić serwer vLLM:"
echo "  VLLM_MODEL=$MODEL bash scripts/start_vllm_server.sh"
echo ""
echo "Domyślny model (7B, ~16 GB VRAM):"
echo "  Qwen/Qwen2.5-Coder-7B-Instruct"
echo "Większy model (32B, ~70 GB VRAM):"
echo "  VLLM_MODEL=Qwen/Qwen2.5-Coder-32B-Instruct bash scripts/start_vllm_server.sh"
