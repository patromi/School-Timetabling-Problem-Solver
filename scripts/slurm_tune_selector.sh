#!/bin/bash
# Strojenie SA i UCB oraz test względem random (scripts/tune_selector.py) na
# węźle obl. Etapy idą po kolei; każdy wynik trafia do CSV od razu, więc po
# przekroczeniu limitu czasu wystarczy zlecić skrypt ponownie z tego samego
# katalogu, a policzą się tylko brakujące przebiegi.
#
# Użycie (z katalogu repo):
#   sbatch -A inz_school_tt scripts/slurm_tune_selector.sh
#   TUNE_STAGES="test300 report" sbatch -A inz_school_tt --export=ALL \
#       scripts/slurm_tune_selector.sh                     # wybrane etapy
#   TUNE_V2=1 sbatch -A inz_school_tt --export=ALL \
#       scripts/slurm_tune_selector.sh                     # druga runda (tuning_v2)
#   TUNE_V3=1 sbatch -A inz_school_tt --export=ALL \
#       scripts/slurm_tune_selector.sh                     # trzecia runda: nagroda (tuning_v3)
#   TUNE_V2=1 TUNE_SHORT=1 TUNE_OUT=data/results/tuning_v2_proba \
#       sbatch -A inz_school_tt --export=ALL -t 00:30:00 \
#       scripts/slurm_tune_selector.sh                     # próba drugiej rundy
#   TUNE_OUT=data/results/tuning_proba TUNE_SHORT=1 \
#       sbatch -A inz_school_tt --export=ALL -t 00:30:00 \
#       scripts/slurm_tune_selector.sh                     # próba

#SBATCH -J tune_selector
#SBATCH -p obl
#SBATCH --exclusive
#SBATCH --mem 0
#SBATCH -t 23:00:00
#SBATCH -o slurm_tune_%j.out
#SBATCH -e slurm_tune_%j.err

set -euo pipefail

PROJ="${PROJ:-$SLURM_SUBMIT_DIR}"
if [[ "${TUNE_V3:-0}" == "1" ]]; then
    STAGES="${TUNE_STAGES:-reward100 reward300 report3}"
    OUT="${TUNE_OUT:-data/results/tuning_v3}"
elif [[ "${TUNE_V2:-0}" == "1" ]]; then
    STAGES="${TUNE_STAGES:-sa2 sa2final test2_100 test2_300 report2}"
    OUT="${TUNE_OUT:-data/results/tuning_v2}"
else
    STAGES="${TUNE_STAGES:-sa ucb ucb2 test100 test300 report}"
    OUT="${TUNE_OUT:-data/results/tuning}"
fi

# jeden przebieg na wątek; przy dużym przydziale 6 wątków zostaje dla systemu
THREADS=$(nproc)
if (( THREADS > 12 )); then
    WORKERS="${TUNE_WORKERS:-$(( THREADS - 6 ))}"
else
    WORKERS="${TUNE_WORKERS:-$THREADS}"
fi

SHORT_FLAG=()
if [[ "${TUNE_SHORT:-0}" == "1" ]]; then
    SHORT_FLAG=(--short)
fi

cd "$PROJ"
echo "=== Strojenie selektora (PUT cluster) ==="
echo "Node    : $(hostname)  ($THREADS wątków)"
echo "Date    : $(date)"
echo "Job ID  : $SLURM_JOB_ID"
echo "Commit  : $(git rev-parse --short HEAD)"
echo "Etapy   : $STAGES"
echo "Workers : $WORKERS"
echo "Wyniki  : $OUT/"

git lfs pull
uv sync
if head -c 200 data/xhstt2014/XHSTT-2014.xml | grep -q "git-lfs"; then
    echo "BŁĄD: XHSTT-2014.xml to wskaźnik LFS (git lfs pull nie zadziałał)." >&2
    exit 1
fi

# shellcheck disable=SC2086
uv run python scripts/tune_selector.py \
    --stages $STAGES \
    --out "$OUT" \
    --workers "$WORKERS" \
    "${SHORT_FLAG[@]}"

echo "=== Zakończono $(date) ==="
