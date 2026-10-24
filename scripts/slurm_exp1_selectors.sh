#!/bin/bash
# Exp1: porównanie selektorów (DoD Etapu 6)
# 3 selektory × 1 acceptance × 5 instancji × 20 seedów = 300 runów
# obl1: 128 rdzeni, --workers 120 → ~12 min wall time przy 300s/run
#
# Użycie:
#   sbatch -A <KONTO> scripts/slurm_exp1_selectors.sh
#   # lub z kontem wpisanym na stałe:
#   sbatch scripts/slurm_exp1_selectors.sh

#SBATCH -J exp1_selectors
#SBATCH -p hgx
#SBATCH -w obl1
#SBATCH --cpus-per-task 120
#SBATCH --mem 80G
#SBATCH -t 00:30:00
#SBATCH -o slurm_exp1_%j.out
#SBATCH -e slurm_exp1_%j.err

set -euo pipefail

PROJECT_DIR="$HOME/PycharmProjects/LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem"
cd "$PROJECT_DIR"

LOG_DIR="/raid/$USER/exp1_logs"
mkdir -p "$LOG_DIR"
mkdir -p data/results

echo "=== Exp1: selectors comparison ==="
echo "Node: $(hostname)  Date: $(date)"
echo "CPUs allocated: $SLURM_CPUS_PER_TASK"

uv run python experiments.py \
  --instances BR-SA-00 ZA-WD-09 AU-SA-96 GR-PA-08 AU-BG-98 \
  --selectors random epsilon-greedy ucb \
  --acceptance lahc \
  --seeds $(seq 0 19) \
  --time-limit 300 \
  --workers 120 \
  --output data/results/exp1_selectors.csv \
  --log-dir "$LOG_DIR"

echo "Kopiowanie logów..."
cp -r "$LOG_DIR" data/logs_exp1
echo "=== Gotowe: $(date) ==="
