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
#   -V <WARIANTY>       Warianty LLM oddzielone spacją (domyślnie "none a")
#   --exp-time <H:M:S>  Limit zadania eksperymentu (domyślnie szacowany)
#   --r1                DeepSeek-R1 na całym hgx2 (slurm_vllm_r1.sh, model
#                       deepseek-r1), warianty "a b rag" bez puli statycznej,
#                       max_tokens 16384, timeout HTTP 30 min, czekanie na
#                       start serwera do 2 h, limit eksperymentu 23 h,
#                       wyniki w data/results/exp_l1_r1.csv
#   --obl2              Użyj obl2 zamiast obl1
#   --dry-run           Wypisz komendy bez uruchamiania
#
# Przykłady:
#   bash scripts/submit_llm_experiments.sh -A <KONTO> --dry-run
#   bash scripts/submit_llm_experiments.sh -A <KONTO> -n 3 -t 60 -i "BR-SA-00 GR-PA-08"   # próba
#   bash scripts/submit_llm_experiments.sh -A <KONTO>                                      # pełny
#   bash scripts/submit_llm_experiments.sh -A <KONTO> --r1                                 # R1, 3 warianty
#   bash scripts/submit_llm_experiments.sh -A <KONTO> --r1 -n 2 -t 60 -i "BR-SA-00"        # próba R1

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
VARIANTS="none a"
EXP_TIME=""
R1=0

# ── parsowanie argumentów ─────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -A)        ACCOUNT="$2"; shift 2 ;;
        -m)        MODEL="$2"; shift 2 ;;
        -t)        TIME_LIMIT="$2"; shift 2 ;;
        -n)        NUM_SEEDS="$2"; shift 2 ;;
        -i)        INSTANCES="$2"; shift 2 ;;
        -w)        WORKERS="$2"; shift 2 ;;
        -V)        VARIANTS="$2"; VARIANTS_SET=1; shift 2 ;;
        --exp-time) EXP_TIME="$2"; shift 2 ;;
        --r1)      R1=1; shift ;;
        --obl2)    OBL_NODE="obl2"; shift ;;
        --dry-run) DRY_RUN=1; shift ;;
        *)         echo "Nieznana opcja: $1" >&2; exit 1 ;;
    esac
done

if [[ -z "$ACCOUNT" ]]; then
    echo "BŁĄD: Podaj konto SLURM: -A <KONTO>" >&2
    exit 1
fi

# tryb R1: inny skrypt serwera (cały hgx2, własny limit 72 h) i ustawienia
# pod model z rozumowaniem, które zużywa tysiące tokenów przed odpowiedzią
VLLM_SCRIPT_NAME="slurm_vllm_hgx2.sh"
OUTPUT_NAME="exp_l1"
LLM_MAX_TOKENS="${LLM_MAX_TOKENS:-1024}"
LLM_HTTP_TIMEOUT="${LLM_HTTP_TIMEOUT:-300}"
VLLM_WAIT_S="${VLLM_WAIT_S:-1800}"
if [[ $R1 -eq 1 ]]; then
    VLLM_SCRIPT_NAME="slurm_vllm_r1.sh"
    MODEL="deepseek-r1"
    [[ -z "${VARIANTS_SET:-}" ]] && VARIANTS="a b rag"
    OUTPUT_NAME="exp_l1_r1"
    LLM_MAX_TOKENS=16384
    LLM_HTTP_TIMEOUT=1800
    VLLM_WAIT_S=7200
    [[ -z "$EXP_TIME" ]] && EXP_TIME="23:00:00"
fi

SEEDS=$(seq -s' ' 0 $(( NUM_SEEDS - 1 )))

# katalog repo: bieżący (uruchamiaj z katalogu projektu); PROJ=... nadpisuje
PROJ="${PROJ:-$PWD}"
SCRIPTS="$PROJ/scripts"

# ── szacowany czas ────────────────────────────────────────────────────────
# przebiegi: instancje * seedy * warianty, w falach po WORKERS;
# każda fala trwa limit przeszukiwania + czas LLM (zapas x2) + 10 min na start
INST_COUNT=$(echo "$INSTANCES" | wc -w)
VAR_COUNT=$(echo "$VARIANTS" | wc -w)
RUNS=$(( INST_COUNT * NUM_SEEDS * VAR_COUNT ))
WAVES=$(( (RUNS + WORKERS - 1) / WORKERS ))
EXP_MIN=$(( WAVES * TIME_LIMIT * 2 / 60 + 10 ))
EXP_H=$(( EXP_MIN / 60 ))
EXP_M=$(( EXP_MIN % 60 ))
VLLM_H=$(( EXP_H + 2 ))   # vLLM musi żyć dłużej: start modelu + kolejka
if [[ -z "$EXP_TIME" ]]; then
    EXP_TIME="${EXP_H}:${EXP_M}:00"
fi

echo "=== submit_llm_experiments.sh (EXP-L1) ==="
echo "Konto       : $ACCOUNT"
echo "Model vLLM  : $MODEL  (serwer: $VLLM_SCRIPT_NAME)"
echo "Warianty LLM: $VARIANTS"
echo "max_tokens  : $LLM_MAX_TOKENS  timeout HTTP: ${LLM_HTTP_TIMEOUT}s"
echo "Node CPU    : $OBL_NODE (cały węzeł)"
echo "Instancje   : $INSTANCES"
echo "Seedy       : $SEEDS"
echo "Time-limit  : ${TIME_LIMIT}s przeszukiwania / przebieg"
echo "Przebiegi   : $RUNS ($VAR_COUNT warianty), $WORKERS workerów, $WAVES fal"
echo "Limit EXP   : $EXP_TIME"
if [[ $R1 -eq 1 ]]; then
    echo "Limit vLLM  : z $VLLM_SCRIPT_NAME (strażnik wyłączy go po eksperymencie)"
else
    echo "Limit vLLM  : ${VLLM_H}h"
fi
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
echo ">> Submituję: vLLM server na hgx2 ($VLLM_SCRIPT_NAME) ..."
if [[ $R1 -eq 1 ]]; then
    # model i limit czasu są w slurm_vllm_r1.sh
    VLLM_JOB=$(sbatch_cmd \
        -A "$ACCOUNT" \
        --export="ALL,VLLM_PORT=${VLLM_PORT}" \
        "$SCRIPTS/$VLLM_SCRIPT_NAME")
else
    VLLM_JOB=$(sbatch_cmd \
        -A "$ACCOUNT" \
        --time="${VLLM_H}:00:00" \
        --export="ALL,VLLM_MODEL=${MODEL},VLLM_PORT=${VLLM_PORT}" \
        "$SCRIPTS/$VLLM_SCRIPT_NAME")
fi
echo "   Job ID vLLM: $VLLM_JOB"
echo ""

# ── 2. EXP-L1 na obl (startuje razem z vLLM, czeka na /health) ───────────
echo ">> Submituję: EXP-L1 (na $OBL_NODE, po starcie vLLM job $VLLM_JOB) ..."
EXP_ENV="ALL,PROJ=${PROJ},VLLM_HOST=hgx2,VLLM_PORT=${VLLM_PORT},VLLM_MODEL=${MODEL}"
EXP_ENV="${EXP_ENV},EXP_INSTANCES=${INSTANCES},EXP_SEEDS=${SEEDS}"
EXP_ENV="${EXP_ENV},EXP_TIME_LIMIT=${TIME_LIMIT},EXP_WORKERS=${WORKERS}"
EXP_ENV="${EXP_ENV},EXP_LLM_VARIANTS=${VARIANTS},EXP_OUTPUT_NAME=${OUTPUT_NAME}"
EXP_ENV="${EXP_ENV},LLM_MAX_TOKENS=${LLM_MAX_TOKENS},LLM_HTTP_TIMEOUT=${LLM_HTTP_TIMEOUT}"
EXP_ENV="${EXP_ENV},VLLM_WAIT_S=${VLLM_WAIT_S}"

EXP_JOB=$(sbatch_cmd \
    -A "$ACCOUNT" \
    -w "$OBL_NODE" \
    --time="${EXP_TIME}" \
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
if [[ $R1 -eq 1 ]]; then
    echo " vLLM server : job $VLLM_JOB  (hgx2, $VLLM_SCRIPT_NAME)"
else
    echo " vLLM server : job $VLLM_JOB  (hgx2, limit ${VLLM_H}h)"
fi
echo " EXP-L1      : job $EXP_JOB  ($OBL_NODE, limit $EXP_TIME)"
echo " strażnik    : job $CANCEL_JOB"
echo ""
echo " Monitoruj:  squeue -u \$USER"
echo "             tail -f slurm_exp_l1_${EXP_JOB}.out"
echo "             tail -f slurm_vllm_*${VLLM_JOB}.out"
echo " Anuluj:     scancel $VLLM_JOB $EXP_JOB $CANCEL_JOB"
echo "==========================================="
echo "Wyniki: $PROJ/data/results/${OUTPUT_NAME}.csv"
