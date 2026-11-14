#!/bin/bash
# Koordynator: startuje vLLM na hgx2 i EXP-L1 na obl.
#
# Użycie:
#   bash scripts/submit_llm_experiments.sh -A <KONTO> [opcje]
#
# Opcje:
#   -A <KONTO>          SLURM account (wymagany)
#   -m <MODEL>          Nazwa modelu vLLM (domyślnie Qwen/Qwen2.5-Coder-32B-Instruct)
#   -t <LIMIT>          Czas na jeden przebieg w sekundach (domyślnie 300)
#   -n <SEEDS>          Liczba seedów 0..N-1 (domyślnie 10)
#   -i <INSTANCJE>      Instancje oddzielone spacją (domyślnie "BR-SA-00 ZA-WD-09 AU-SA-96")
#   --dry-run           Wypisz komendy bez uruchamiania
#   --no-baseline       Pomiń fazę baseline (ucb/none) — używaj gdy masz je z EXP-1
#   --obl2              Użyj obl2 zamiast obl1 do eksperymentów
#
# Przykład:
#   bash scripts/submit_llm_experiments.sh -A grants-put -t 300 -n 10

set -euo pipefail

# ── wartości domyślne ─────────────────────────────────────────────────────
ACCOUNT=""
MODEL="Qwen/Qwen2.5-Coder-32B-Instruct"
TIME_LIMIT=300
NUM_SEEDS=10
INSTANCES="BR-SA-00 ZA-WD-09 AU-SA-96"
DRY_RUN=0
SKIP_BASELINE=0
OBL_NODE="obl1"
VLLM_PORT=8000

# ── parsowanie argumentów ─────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -A)           ACCOUNT="$2"; shift 2 ;;
        -m)           MODEL="$2"; shift 2 ;;
        -t)           TIME_LIMIT="$2"; shift 2 ;;
        -n)           NUM_SEEDS="$2"; shift 2 ;;
        -i)           INSTANCES="$2"; shift 2 ;;
        --dry-run)    DRY_RUN=1; shift ;;
        --no-baseline) SKIP_BASELINE=1; shift ;;
        --obl2)       OBL_NODE="obl2"; shift ;;
        *)            echo "Nieznana opcja: $1" >&2; exit 1 ;;
    esac
done

if [[ -z "$ACCOUNT" ]]; then
    echo "BŁĄD: Podaj konto SLURM: -A <KONTO>" >&2
    exit 1
fi

SEEDS=$(seq -s' ' 0 $(( NUM_SEEDS - 1 )))

PROJ="$HOME/PycharmProjects/LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem"
SCRIPTS="$PROJ/scripts"

# ── szacowany czas (z marginesem) ─────────────────────────────────────────
# EXP-L1 LLM: 1 wariant * 3 instancje * NUM_SEEDS * TIME_LIMIT + 20% narzutu
INST_COUNT=$(echo "$INSTANCES" | wc -w)
LLM_RUNS=$(( INST_COUNT * NUM_SEEDS ))
LLM_EST_H=$(( (LLM_RUNS * TIME_LIMIT * 12 / 10) / 3600 + 1 ))  # +20%, min 1h
VLLM_LIMIT=$(( LLM_EST_H + 2 ))  # vLLM musi żyć dłużej

echo "=== submit_llm_experiments.sh ==="
echo "Konto       : $ACCOUNT"
echo "Model vLLM  : $MODEL"
echo "Node CPU    : $OBL_NODE"
echo "Instancje   : $INSTANCES"
echo "Seedy       : $SEEDS"
echo "Time-limit  : ${TIME_LIMIT}s / przebieg"
echo "EXP-L1 est  : ~${LLM_EST_H}h (workers=1)"
echo "vLLM limit  : ${VLLM_LIMIT}h"
echo ""

sbatch_cmd() {
    if [[ $DRY_RUN -eq 1 ]]; then
        echo "  [DRY-RUN] sbatch $*"
        echo "0"  # fikcyjny job ID
    else
        sbatch "$@" | awk '{print $4}'
    fi
}

# ── 1. Uruchom vLLM na hgx2 ───────────────────────────────────────────────
echo ">> Submituję: vLLM server na hgx2 ..."
VLLM_JOB=$(sbatch_cmd \
    -A "$ACCOUNT" \
    --time="${VLLM_LIMIT}:00:00" \
    --export="VLLM_MODEL=${MODEL},VLLM_PORT=${VLLM_PORT}" \
    "$SCRIPTS/slurm_vllm_hgx2.sh")

echo "   Job ID vLLM: $VLLM_JOB"
echo ""

# ── 2. EXP-L1: UCB + LLM wariant A (na obl, po starcie vLLM) ─────────────
echo ">> Submituję: EXP-L1 LLM (na $OBL_NODE, zależny od vLLM job $VLLM_JOB) ..."

EXP_ENV="VLLM_HOST=hgx2"
EXP_ENV="${EXP_ENV},VLLM_PORT=${VLLM_PORT}"
EXP_ENV="${EXP_ENV},VLLM_MODEL=${MODEL}"
EXP_ENV="${EXP_ENV},EXP_INSTANCES=${INSTANCES}"
EXP_ENV="${EXP_ENV},EXP_SEEDS=${SEEDS}"
EXP_ENV="${EXP_ENV},EXP_TIME_LIMIT=${TIME_LIMIT}"

LLM_JOB=$(sbatch_cmd \
    -A "$ACCOUNT" \
    -w "$OBL_NODE" \
    --time="${LLM_EST_H}:30:00" \
    --dependency="after:${VLLM_JOB}" \
    --export="${EXP_ENV}" \
    "$SCRIPTS/slurm_exp_l1_llm.sh")

echo "   Job ID EXP-L1 LLM: $LLM_JOB"
echo ""

# ── 3. Opcjonalnie: baseline ucb/none z więcej workerami ─────────────────
if [[ $SKIP_BASELINE -eq 0 ]]; then
    echo ">> Submituję: EXP-L1 baseline (ucb/none, workers=16) równolegle z vLLM ..."

    # Baseline nie potrzebuje vLLM — może startować od razu.
    # Nadpisuje istniejący plik z data/logs_v2/ — użyj --no-baseline jeśli już masz.
    BASELINE_ENV="EXP_INSTANCES=${INSTANCES}"
    BASELINE_ENV="${BASELINE_ENV},EXP_SEEDS=${SEEDS}"
    BASELINE_ENV="${BASELINE_ENV},EXP_TIME_LIMIT=${TIME_LIMIT}"
    BASELINE_ENV="${BASELINE_ENV},LLM_VARIANT=none"

    # Inline skrypt baseline — prosty, nie wymaga osobnego pliku
    BASELINE_JOB=$(sbatch_cmd \
        -A "$ACCOUNT" \
        -p hgx \
        -w "$OBL_NODE" \
        -J exp_l1_baseline \
        --cpus-per-task 32 \
        --mem 48G \
        --time="02:00:00" \
        --export="${BASELINE_ENV}" \
        --wrap="
            set -euo pipefail
            PROJ=\"$PROJ\"
            LOG_DIR=\"/raid/\$USER/exp_l1_baseline_\${SLURM_JOB_ID}\"
            mkdir -p \"\$LOG_DIR\"
            cd \"\$PROJ\"
            uv run python experiments.py \
                --selectors ucb \
                --llm-variants none \
                --instances \$EXP_INSTANCES \
                --seeds \$EXP_SEEDS \
                --time-limit \$EXP_TIME_LIMIT \
                --workers 16 \
                --log-dir \"\$LOG_DIR\" \
                --output \"\$LOG_DIR/exp_l1_baseline.csv\"
            mkdir -p \"\$PROJ/data/logs_llm\"
            cp -r \"\$LOG_DIR\"/. \"\$PROJ/data/logs_llm/\"
            cp \"\$LOG_DIR/exp_l1_baseline.csv\" \"\$PROJ/data/results/exp_l1_baseline.csv\"
        ")

    echo "   Job ID EXP-L1 baseline: $BASELINE_JOB"
    echo ""
fi

# ── podsumowanie ──────────────────────────────────────────────────────────
echo "==========================================="
echo " Zadania SLURM uruchomione:"
echo "   vLLM server  : job $VLLM_JOB  (hgx2, limit ${VLLM_LIMIT}h)"
echo "   EXP-L1 LLM   : job $LLM_JOB  ($OBL_NODE, starts after vLLM)"
[[ $SKIP_BASELINE -eq 0 ]] && echo "   EXP-L1 base  : job $BASELINE_JOB  ($OBL_NODE, natychmiast)"
echo ""
echo " Monitoruj:"
echo "   squeue -u \$USER"
echo "   tail -f slurm_exp_l1_${LLM_JOB}.out"
echo "   tail -f slurm_vllm_${VLLM_JOB}.out"
echo ""
echo " Anuluj wszystko:"
echo "   scancel $VLLM_JOB $LLM_JOB"
echo "==========================================="

# ── zamknij vLLM gdy experiment się skończy ───────────────────────────────
# Uruchamiamy zadanie-strażnika: czeka na zakończenie LLM-joba, potem anuluje vLLM.
if [[ $DRY_RUN -eq 0 ]]; then
    echo ""
    echo ">> Submituję strażnika: anuluje vLLM po zakończeniu eksperymentu ..."
    CANCEL_JOB=$(sbatch \
        -A "$ACCOUNT" \
        -J vllm_cancel \
        -p hgx \
        --cpus-per-task 1 \
        --mem 512M \
        --time="00:05:00" \
        --dependency="afterany:${LLM_JOB}" \
        --export="VLLM_JOB=${VLLM_JOB}" \
        --wrap='echo "Anulowanie vLLM job $VLLM_JOB ..."; scancel "$VLLM_JOB" && echo "OK." || true' \
        | awk '{print $4}')
    echo "   Job ID strażnik: $CANCEL_JOB"
fi

echo ""
echo "Gotowe. Wyniki pojawią się w: $PROJ/data/results/"
