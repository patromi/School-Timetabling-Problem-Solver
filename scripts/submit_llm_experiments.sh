#!/bin/bash
# Koordynator EXP-L1: startuje vLLM na hgx2, eksperyment na obl i strażnika,
# który wyłącza vLLM po zakończeniu eksperymentu.
#
# Eksperyment (slurm_exp_l1_llm.sh) puszcza oba warianty — UCB bez LLM i UCB
# z LLM — w jednym zadaniu na całym węźle obl, więc osobny baseline nie jest
# potrzebny.
#
# Użycie:
#   bash scripts/submit_llm_experiments.sh -A <KONTO> [opcje]
#
# Opcje:
#   -A <KONTO>          SLURM account (wymagany)
#   -m <MODEL>          Model vLLM (domyślnie Qwen/Qwen2.5-Coder-32B-Instruct)
#   -t <LIMIT>          Sekundy przeszukiwania na przebieg (domyślnie 300)
#   -n <SEEDS>          Liczba seedów 0..N-1 (domyślnie 10)
#   -i <INSTANCJE>      Instancje oddzielone spacją (domyślnie 10 instancji
#                       w zakresie pracy, zob. slurm_exp_l1_llm.sh)
#   -w <WORKERS>        Równoległe przebiegi na obl (domyślnie 96)
#   --obl2              Użyj obl2 zamiast obl1
#   --dry-run           Wypisz komendy bez uruchamiania
#
# Przykłady:
#   bash scripts/submit_llm_experiments.sh -A <KONTO> --dry-run
#   bash scripts/submit_llm_experiments.sh -A <KONTO> -n 3 -t 60 -i "BR-SA-00 GR-PA-08"   # próba
#   bash scripts/submit_llm_experiments.sh -A <KONTO>                                      # pełny

set -euo pipefail

# ── wartości domyślne ─────────────────────────────────────────────────────
ACCOUNT=""
MODEL="Qwen/Qwen2.5-Coder-32B-Instruct"
TIME_LIMIT=300
NUM_SEEDS=10
INSTANCES="BR-SA-00 BR-SN-00 FI-WP-06 FI-MP-06 GR-P3-10 GR-PA-08 GR-H1-97 ZA-LW-09 ZA-WD-09 IT-I4-96"
WORKERS=96
DRY_RUN=0
OBL_NODE="obl1"
VLLM_PORT=8000

# ── parsowanie argumentów ─────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -A)        ACCOUNT="$2"; shift 2 ;;
        -m)        MODEL="$2"; shift 2 ;;
        -t)        TIME_LIMIT="$2"; shift 2 ;;
        -n)        NUM_SEEDS="$2"; shift 2 ;;
        -i)        INSTANCES="$2"; shift 2 ;;
        -w)        WORKERS="$2"; shift 2 ;;
        --obl2)    OBL_NODE="obl2"; shift ;;
        --dry-run) DRY_RUN=1; shift ;;
        *)         echo "Nieznana opcja: $1" >&2; exit 1 ;;
    esac
done

if [[ -z "$ACCOUNT" ]]; then
    echo "BŁĄD: Podaj konto SLURM: -A <KONTO>" >&2
    exit 1
fi

SEEDS=$(seq -s' ' 0 $(( NUM_SEEDS - 1 )))

# katalog repo: bieżący (uruchamiaj z katalogu projektu); PROJ=... nadpisuje
PROJ="${PROJ:-$PWD}"
SCRIPTS="$PROJ/scripts"

# ── szacowany czas ────────────────────────────────────────────────────────
# przebiegi: instancje * seedy * 2 warianty (none, a), w falach po WORKERS;
# każda fala trwa limit przeszukiwania + czas LLM (zapas x2) + 10 min na start
INST_COUNT=$(echo "$INSTANCES" | wc -w)
RUNS=$(( INST_COUNT * NUM_SEEDS * 2 ))
WAVES=$(( (RUNS + WORKERS - 1) / WORKERS ))
EXP_MIN=$(( WAVES * TIME_LIMIT * 2 / 60 + 10 ))
EXP_H=$(( EXP_MIN / 60 ))
EXP_M=$(( EXP_MIN % 60 ))
VLLM_H=$(( EXP_H + 2 ))   # vLLM musi żyć dłużej: start modelu + kolejka

echo "=== submit_llm_experiments.sh (EXP-L1) ==="
echo "Konto       : $ACCOUNT"
echo "Model vLLM  : $MODEL"
echo "Node CPU    : $OBL_NODE (cały węzeł)"
echo "Instancje   : $INSTANCES"
echo "Seedy       : $SEEDS"
echo "Time-limit  : ${TIME_LIMIT}s przeszukiwania / przebieg"
echo "Przebiegi   : $RUNS (2 warianty), $WORKERS workerów, $WAVES fal"
echo "Limit EXP   : ${EXP_H}h ${EXP_M}min"
echo "Limit vLLM  : ${VLLM_H}h"
echo ""

sbatch_cmd() {
    if [[ $DRY_RUN -eq 1 ]]; then
        echo "  [DRY-RUN] sbatch $*" >&2
        echo "0"  # fikcyjny job ID
    else
        sbatch "$@" | awk '{print $4}'
    fi
}

# ── 1. vLLM na hgx2 ───────────────────────────────────────────────────────
echo ">> Submituję: vLLM server na hgx2 ..."
VLLM_JOB=$(sbatch_cmd \
    -A "$ACCOUNT" \
    --time="${VLLM_H}:00:00" \
    --export="ALL,VLLM_MODEL=${MODEL},VLLM_PORT=${VLLM_PORT}" \
    "$SCRIPTS/slurm_vllm_hgx2.sh")
echo "   Job ID vLLM: $VLLM_JOB"
echo ""

# ── 2. EXP-L1 na obl (startuje razem z vLLM, czeka na /health) ───────────
echo ">> Submituję: EXP-L1 (na $OBL_NODE, po starcie vLLM job $VLLM_JOB) ..."
EXP_ENV="ALL,PROJ=${PROJ},VLLM_HOST=hgx2,VLLM_PORT=${VLLM_PORT},VLLM_MODEL=${MODEL}"
EXP_ENV="${EXP_ENV},EXP_INSTANCES=${INSTANCES},EXP_SEEDS=${SEEDS}"
EXP_ENV="${EXP_ENV},EXP_TIME_LIMIT=${TIME_LIMIT},EXP_WORKERS=${WORKERS}"

EXP_JOB=$(sbatch_cmd \
    -A "$ACCOUNT" \
    -w "$OBL_NODE" \
    --time="${EXP_H}:${EXP_M}:00" \
    --dependency="after:${VLLM_JOB}" \
    --export="${EXP_ENV}" \
    "$SCRIPTS/slurm_exp_l1_llm.sh")
echo "   Job ID EXP-L1: $EXP_JOB"
echo ""

# ── 3. strażnik: wyłącza vLLM po zakończeniu eksperymentu ─────────────────
echo ">> Submituję strażnika: anuluje vLLM po zakończeniu EXP-L1 ..."
CANCEL_JOB=$(sbatch_cmd \
    -A "$ACCOUNT" \
    -J vllm_cancel \
    -p obl \
    --cpus-per-task 1 \
    --mem 512M \
    --time="00:05:00" \
    --dependency="afterany:${EXP_JOB}" \
    --export="ALL,VLLM_JOB=${VLLM_JOB}" \
    --wrap='echo "Anulowanie vLLM job $VLLM_JOB ..."; scancel "$VLLM_JOB" && echo "OK." || true')
echo "   Job ID strażnik: $CANCEL_JOB"

# ── podsumowanie ──────────────────────────────────────────────────────────
echo ""
echo "==========================================="
echo " vLLM server : job $VLLM_JOB  (hgx2, limit ${VLLM_H}h)"
echo " EXP-L1      : job $EXP_JOB  ($OBL_NODE, limit ${EXP_H}h ${EXP_M}min)"
echo " strażnik    : job $CANCEL_JOB"
echo ""
echo " Monitoruj:  squeue -u \$USER"
echo "             tail -f slurm_exp_l1_${EXP_JOB}.out"
echo "             tail -f slurm_vllm_${VLLM_JOB}.out"
echo " Anuluj:     scancel $VLLM_JOB $EXP_JOB $CANCEL_JOB"
echo "==========================================="
echo "Wyniki: $PROJ/data/results/exp_l1.csv"
