#!/bin/bash
# Exp3: porównanie kryteriów akceptacji (LAHC warianty vs SA warianty)
# 2 selektory × 6 acceptance × 3 instancje × 15 seedów = 540 runów
# obl1: --workers 120 → ~22 min wall time przy 300s/run
#
# Uwaga: Exp3 uruchamiamy PO Exp1 (jeśli obl1 wolne) lub razem z Exp2 na obl2.
#
# Użycie:
#   sbatch -A <KONTO> scripts/slurm_exp3_acceptance.sh

#SBATCH -J exp3_acceptance
#SBATCH -p hgx
#SBATCH -w obl1
#SBATCH --cpus-per-task 120
#SBATCH --mem 80G
#SBATCH -t 01:00:00
#SBATCH -o slurm_exp3_%j.out
#SBATCH -e slurm_exp3_%j.err

set -euo pipefail

PROJECT_DIR="$HOME/PycharmProjects/LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem"
cd "$PROJECT_DIR"

LOG_DIR="/raid/$USER/exp3_logs"
mkdir -p "$LOG_DIR"
mkdir -p data/results

echo "=== Exp3: acceptance criterion comparison ==="
echo "Node: $(hostname)  Date: $(date)"

uv run python experiments.py \
  --instances BR-SA-00 ZA-WD-09 AU-SA-96 \
  --selectors random ucb \
  --acceptance lahc lahc-L10 lahc-L100 lahc-L300 sa sa-t005 \
  --seeds $(seq 0 14) \
  --time-limit 300 \
  --workers 120 \
  --output data/results/exp3_acceptance.csv \
  --log-dir "$LOG_DIR"

cp -r "$LOG_DIR" data/logs_exp3
echo "=== Gotowe: $(date) ==="
