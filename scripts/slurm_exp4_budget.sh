#!/bin/bash
# Exp4: skalowanie budżetu czasu (czy UCB uczy się lepiej przy dłuższym czasie?)
# 4 time-limity uruchamiane sekwencyjnie; każdy: 2 selektory × 2 instancje × 15 seedów = 60 runów
# Łącznie: 240 runów; bottleneck to przebiegi 600s → ~75 min wall time.
#
# Zalecane: pmem-[1-6] (24 rdzenie, 192GB) albo obl1 z mniejszą liczbą workerów.
# Uruchamiamy na pmem-1 żeby nie blokować obl[1-2] dla ważniejszych eksperymentów.
#
# Użycie:
#   sbatch -A <KONTO> scripts/slurm_exp4_budget.sh

#SBATCH -J exp4_budget
#SBATCH -p pmem
#SBATCH -w pmem-1
#SBATCH --cpus-per-task 48
#SBATCH --mem 60G
#SBATCH -t 02:00:00
#SBATCH -o slurm_exp4_%j.out
#SBATCH -e slurm_exp4_%j.err

set -euo pipefail

PROJECT_DIR="$HOME/PycharmProjects/LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem"
cd "$PROJECT_DIR"

mkdir -p data/results

echo "=== Exp4: time budget scaling ==="
echo "Node: $(hostname)  Date: $(date)"

INSTANCES="ZA-WD-09 AU-SA-96"
SELECTORS="random ucb"
SEEDS="$(seq 0 14)"
WORKERS=44

for TLIMIT in 60 120 300 600; do
  echo "--- time-limit=${TLIMIT}s ---"
  uv run python experiments.py \
    --instances $INSTANCES \
    --selectors $SELECTORS \
    --acceptance lahc \
    --seeds $SEEDS \
    --time-limit $TLIMIT \
    --workers $WORKERS \
    --no-logs \
    --output "data/results/exp4_budget_t${TLIMIT}.csv"
done

echo "=== Gotowe: $(date) ==="
