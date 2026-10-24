#!/bin/bash
# Exp2: wrażliwość UCB na parametry (c, alpha, collapse_stagnation)
# 7 wariantów × 1 acceptance × 3 instancje × 10 seedów = 210 runów
# obl2: 128 rdzeni, --workers 120 → ~9 min wall time przy 300s/run
#
# Użycie:
#   sbatch -A <KONTO> scripts/slurm_exp2_ucb_tuning.sh

#SBATCH -J exp2_ucb_tuning
#SBATCH -p hgx
#SBATCH -w obl2
#SBATCH --cpus-per-task 120
#SBATCH --mem 80G
#SBATCH -t 00:20:00
#SBATCH -o slurm_exp2_%j.out
#SBATCH -e slurm_exp2_%j.err

set -euo pipefail

PROJECT_DIR="$HOME/PycharmProjects/LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem"
cd "$PROJECT_DIR"

LOG_DIR="/raid/$USER/exp2_logs"
mkdir -p "$LOG_DIR"
mkdir -p data/results

echo "=== Exp2: UCB hyperparameter tuning ==="
echo "Node: $(hostname)  Date: $(date)"

uv run python experiments.py \
  --instances BR-SA-00 ZA-WD-09 AU-SA-96 \
  --selectors ucb ucb-c03 ucb-c2 ucb-alpha005 ucb-alpha015 ucb-2state ucb-2state-c03 \
  --acceptance lahc \
  --seeds $(seq 0 9) \
  --time-limit 300 \
  --workers 120 \
  --output data/results/exp2_ucb_tuning.csv \
  --log-dir "$LOG_DIR"

cp -r "$LOG_DIR" data/logs_exp2
echo "=== Gotowe: $(date) ==="
