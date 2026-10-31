#!/bin/bash
# ==============================================================================
# Eksperyment Adaptive (z preprocessingiem):
# 17 instancji XHSTT × 10 seedów = 170 zadań po 500 000 iteracji
# Uruchomienie na 170 wątkach równolegle (węzeł obl2 ma 256 rdzeni)
#
# Użycie na klastrze SLURM:
#   sbatch scripts/slurm_adaptive_500k.sh
#   sbatch -A <KONTO> scripts/slurm_adaptive_500k.sh
#
# Podgląd statusu:
#   squeue -u $USER
#   tail -f slurm_adaptive_*.out
#
# Anulowanie:
#   scancel <JOB_ID>
#   scancel -u $USER
# ==============================================================================

#SBATCH -J adaptive_500k
#SBATCH -p obl
#SBATCH -w obl2
#SBATCH --cpus-per-task 170
#SBATCH --mem 120G
#SBATCH -t 05:00:00
#SBATCH -o slurm_adaptive_%j.out
#SBATCH -e slurm_adaptive_%j.err

set -euo pipefail

# Wczytaj srodowisko uzytkownika i dodaj typowe sciezki w PATH
export PATH="$HOME/.local/bin:$HOME/.cargo/bin:/home/inz_school_tt/.local/bin:/home/inz_school_tt/.cargo/bin:$PATH"
[ -f "$HOME/.local/bin/env" ] && source "$HOME/.local/bin/env" || true
if [ -f "$HOME/.bashrc" ]; then
    set +u
    source "$HOME/.bashrc"
    set -u
fi

# W SLURM skrypt wsadowy jest kopiowany do /var/spool/slurmd/...
# Aby ustalic wlasciwy katalog repozytorium, uzywamy SLURM_SUBMIT_DIR
if [ -n "${SLURM_SUBMIT_DIR:-}" ]; then
    if [ -f "${SLURM_SUBMIT_DIR}/preprocessing_experiments.py" ]; then
        PROJECT_DIR="$SLURM_SUBMIT_DIR"
    elif [ -f "${SLURM_SUBMIT_DIR}/../preprocessing_experiments.py" ]; then
        PROJECT_DIR="$(cd "${SLURM_SUBMIT_DIR}/.." && pwd)"
    else
        PROJECT_DIR="$SLURM_SUBMIT_DIR"
    fi
else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
fi

cd "$PROJECT_DIR"

if [ ! -f "preprocessing_experiments.py" ]; then
    echo "BLAD: Nie znaleziono preprocessing_experiments.py w $(pwd)" >&2
    exit 1
fi

# Wykryj dostepny interpreter Pythona (uv, repo .venv, lub python3)
if command -v uv &>/dev/null; then
    PY_CMD="uv run python"
elif [ -x "${PROJECT_DIR}/.venv/bin/python" ]; then
    PY_CMD="${PROJECT_DIR}/.venv/bin/python"
elif command -v python3 &>/dev/null; then
    PY_CMD="python3"
else
    echo "BLAD: Nie znaleziono ani 'uv', ani '.venv/bin/python', ani 'python3'!" >&2
    exit 1
fi

# Zapewnienie natychmiastowego buforowania wyjscia Pythona
export PYTHONUNBUFFERED=1

# Szybki katalog na logi w /raid (jeśli dostępny), w przeciwnym razie katalog lokalny
if [ -d "/raid" ] && mkdir -p "/raid/$USER" 2>/dev/null; then
    LOG_DIR="/raid/$USER/preprocessing_500k_logs"
else
    LOG_DIR="${PROJECT_DIR}/data/logs/preprocessing_experiments"
fi

mkdir -p "$LOG_DIR"
mkdir -p data/results

WORKERS="${SLURM_CPUS_PER_TASK:-170}"

echo "=============================================================================="
echo "Eksperyment Adaptive (Preprocessing): 17 instancji x 10 seedow x 500k iteracji"
echo "Wezel: $(hostname) | Data: $(date)"
echo "Katalog projektu: $PROJECT_DIR"
echo "Liczba procesow (workers): $WORKERS"
echo "Interpreter: $PY_CMD"
echo "Katalog logow: $LOG_DIR"
echo "=============================================================================="

$PY_CMD preprocessing_experiments.py \
  --modes adaptive \
  --iterations 500000 \
  --workers "$WORKERS" \
  --output-csv data/results/preprocessing_experiments_500k.csv \
  --log-dir "$LOG_DIR"

# Kopia logow do repozytorium po zakonczeniu (jesli logowano do /raid)
if [ "$LOG_DIR" != "${PROJECT_DIR}/data/logs/preprocessing_experiments" ]; then
    echo "Kopiowanie logow z $LOG_DIR do data/logs/preprocessing_experiments..."
    mkdir -p data/logs/preprocessing_experiments
    cp -r "$LOG_DIR"/* data/logs/preprocessing_experiments/ 2>/dev/null || true
fi

echo "=============================================================================="
echo "Gotowe: $(date)"
echo "Wyniki zapisano w: data/results/preprocessing_experiments_500k.csv"
echo "Logi trajektorii: data/logs/preprocessing_experiments/"
echo "=============================================================================="

