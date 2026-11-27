#!/bin/bash
# Konfiguracja maszyny CPU (solver + eksperymenty).
#
# Uruchom raz po sklonowaniu repo:
#   bash scripts/setup_cpu_machine.sh
#
# Potem uruchamiaj eksperymenty przez:
#   bash scripts/run_local_experiments.sh   (bez LLM)
#   bash scripts/run_local_llm_experiments.sh  (z LLM na osobnej maszynie GPU)

set -euo pipefail

echo "=== Konfiguracja środowiska CPU (solver) ==="
echo "Maszyna : $(hostname)"
echo "Data    : $(date)"
echo ""

# ── sprawdź / zainstaluj uv ──────────────────────────────────────────────
if ! command -v uv &>/dev/null; then
    echo "Instaluję uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.cargo/bin:$PATH"
    # Dopisz do .bashrc żeby było po restarcie
    if ! grep -q 'cargo/bin' "$HOME/.bashrc" 2>/dev/null; then
        echo 'export PATH="$HOME/.cargo/bin:$PATH"' >> "$HOME/.bashrc"
    fi
fi
echo "uv: $(uv --version)"

# ── repozytorium ─────────────────────────────────────────────────────────
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "Repo    : $REPO_DIR"
cd "$REPO_DIR"

# ── Python 3.12 przez uv ─────────────────────────────────────────────────
echo "Instaluję Python 3.12 (jeśli brakuje)..."
uv python install 3.12

# ── zależności projektu ──────────────────────────────────────────────────
echo "Synchronizuję zależności projektu..."
uv sync

# ── sprawdź git lfs ──────────────────────────────────────────────────────
if ! command -v git-lfs &>/dev/null; then
    echo ""
    echo "OSTRZEŻENIE: git-lfs nie zainstalowany."
    echo "Zainstaluj:"
    echo "  Ubuntu/Debian: sudo apt install git-lfs"
    echo "  Arch:          sudo pacman -S git-lfs"
    echo "  Fedora:        sudo dnf install git-lfs"
    echo "Bez git-lfs heurystyki generowane przez LLM nie będą cachowane między sesjami."
else
    git lfs install --local 2>/dev/null || true
    echo "git-lfs: $(git lfs version)"
fi

# ── test podstawowy ──────────────────────────────────────────────────────
echo ""
echo "Uruchamiam szybki test parsera..."
uv run python run_solver.py BR-SA-00 --iterations 2000 --seed 42 --time-limit 5 2>&1 | tail -6

echo ""
echo "Uruchamiam testy jednostkowe (bez wolnych)..."
uv run python -m pytest tests/ -m "not slow" --ignore=tests/test_validator.py -q 2>&1 | tail -4

echo ""
echo "=== Setup CPU zakończony ==="
echo ""
echo "Eksperymenty bez LLM:"
echo "  bash scripts/run_local_experiments.sh"
echo ""
echo "Eksperymenty z LLM (wymaga działającego serwera vLLM na GPU maszynie):"
echo "  LLM_HOST=<ip-lub-hostname-gpu> bash scripts/run_local_llm_experiments.sh"
