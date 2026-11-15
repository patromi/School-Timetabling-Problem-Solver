#!/bin/bash
# Submituje wszystkie 4 eksperymenty do kolejki SLURM.
# Exp1 i Exp2 mogą biec równolegle (różne węzły: obl1, obl2).
# Exp3 startuje po zakończeniu Exp1 (--dependency).
# Exp4 biegnie niezależnie na pmem-1.
#
# Użycie:
#   bash scripts/submit_experiments.sh <KONTO>
#   np.: bash scripts/submit_experiments.sh ai

set -euo pipefail

ACCOUNT="${1:?Podaj nazwę konta rozliczeniowego jako argument, np.: bash submit_experiments.sh ai}"

echo "Konto: $ACCOUNT"

JOB1=$(sbatch -A "$ACCOUNT" scripts/slurm_exp1_selectors.sh | awk '{print $4}')
echo "Exp1 (selectors)   -> job $JOB1  [obl1, ~30 min]"

JOB2=$(sbatch -A "$ACCOUNT" scripts/slurm_exp2_ucb_tuning.sh | awk '{print $4}')
echo "Exp2 (UCB tuning)  -> job $JOB2  [obl2, ~20 min]"

# Exp3 czeka na zakończenie Exp1 (obl1 musi być wolny)
JOB3=$(sbatch -A "$ACCOUNT" --dependency=afterok:"$JOB1" scripts/slurm_exp3_acceptance.sh | awk '{print $4}')
echo "Exp3 (acceptance)  -> job $JOB3  [obl1, ~60 min, po Exp1]"

JOB4=$(sbatch -A "$ACCOUNT" scripts/slurm_exp4_budget.sh | awk '{print $4}')
echo "Exp4 (budget)      -> job $JOB4  [pmem-1, ~120 min]"

echo ""
echo "Monitorowanie: squeue -u \$USER"
echo "Logi SLURM:    tail -f slurm_exp1_${JOB1}.out"
