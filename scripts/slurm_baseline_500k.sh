#!/bin/bash
# ==============================================================================
# Eksperyment bazowy (Baseline) bez preprocessingu:
# 17 instancji XHSTT × 10 seedów = 170 zadań po 500 000 iteracji
# Śledzenie momentu osiągnięcia hard_violations = 0 (krok i czas)
#
# Użycie na klastrze SLURM:
#   sbatch scripts/slurm_baseline_500k.sh
#   sbatch -A <KONTO> scripts/slurm_baseline_500k.sh
#
# Podgląd statusu:
#   squeue -u $USER
#   tail -f slurm_baseline_*.out
#
# Anulowanie:
#   scancel <JOB_ID>
#   scancel -u $USER
# ==============================================================================

#SBATCH -J baseline_500k
#SBATCH -p obl
#SBATCH -w obl2
#SBATCH --cpus-per-task 120
#SBATCH --mem 80G
#SBATCH -t 05:00:00
#SBATCH -o slurm_baseline_%j.out
#SBATCH -e slurm_baseline_%j.err

set -euo pipefail

# W SLURM skrypt wsadowy jest kopiowany do /var/spool/slurmd/...
# Aby ustalic wlasciwy katalog repozytorium, uzywamy SLURM_SUBMIT_DIR
if [ -n "${SLURM_SUBMIT_DIR:-}" ]; then
    if [ -f "${SLURM_SUBMIT_DIR}/baseline_experiments.py" ]; then
        PROJECT_DIR="$SLURM_SUBMIT_DIR"
    elif [ -f "${SLURM_SUBMIT_DIR}/../baseline_experiments.py" ]; then
        PROJECT_DIR="$(cd "${SLURM_SUBMIT_DIR}/.." && pwd)"
    else
        PROJECT_DIR="$SLURM_SUBMIT_DIR"
    fi
else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
fi

cd "$PROJECT_DIR"

if [ ! -f "baseline_experiments.py" ]; then
    echo "BLAD: Nie znaleziono baseline_experiments.py w $(pwd)" >&2
    exit 1
fi

# Zapewnienie natychmiastowego buforowania wyjscia Pythona
export PYTHONUNBUFFERED=1

# Szybki katalog na logi w /raid (jeśli dostępny), w przeciwnym razie katalog lokalny
if [ -d "/raid" ] && mkdir -p "/raid/$USER" 2>/dev/null; then
    LOG_DIR="/raid/$USER/baseline_500k_logs"
else
    LOG_DIR="${PROJECT_DIR}/data/logs/baseline_experiments"
fi

mkdir -p "$LOG_DIR"
mkdir -p data/results

WORKERS="${SLURM_CPUS_PER_TASK:-120}"

echo "=============================================================================="
echo "Eksperyment bazowy: 17 instancji x 10 seedow x 500 000 iteracji"
echo "Wezel: $(hostname) | Data: $(date)"
echo "Katalog projektu: $PROJECT_DIR"
echo "Liczba procesow (workers): $WORKERS"
echo "Katalog logow: $LOG_DIR"
echo "=============================================================================="

uv run python baseline_experiments.py \
  --iterations 500000 \
  --workers "$WORKERS" \
  --output-csv data/results/baseline_experiments_500k.csv \
  --log-dir "$LOG_DIR"

# Kopia logow do repozytorium po zakonczeniu (jesli logowano do /raid)
if [ "$LOG_DIR" != "${PROJECT_DIR}/data/logs/baseline_experiments" ]; then
    echo "Kopiowanie logow z $LOG_DIR do data/logs_baseline_500k..."
    mkdir -p data/logs_baseline_500k
    cp -r "$LOG_DIR"/* data/logs_baseline_500k/ 2>/dev/null || true
fi

echo "=============================================================================="
echo "Gotowe: $(date)"
echo "Wyniki zapisano w: data/results/baseline_experiments_500k.csv"
echo "=============================================================================="
