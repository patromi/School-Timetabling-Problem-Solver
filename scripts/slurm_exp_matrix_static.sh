#!/bin/bash
# Eksperymenty bez LLM na całą dobę na węźle obl: pięć faz po kolei, każda
# z własnym CSV. Domyślnie 13 instancji w zakresie pracy x 50 seedów, po
# jednym przebiegu na wątek (nproc - 6 równolegle, czyli 250 na obl).
#
#   faza        konfiguracje                                   przebiegi  czas
#   selektory   random / epsilon-greedy / ucb, SA, 10 min          1950  ~1,3 h
#   akceptacja  te same selektory z LAHC, 10 min                   1950  ~1,3 h
#               (SA do porównania z fazy "selektory")
#   preset      te same selektory, --preset bazowy, 10 min         1950  ~1,3 h
#               (konfiguracja sprzed issue #82)
#   ablacja     UCB: c 0,005 / 0,1 / 0,5, alfa 0,005 / 0,08,       3900  ~2,7 h
#               2 stany zamiast 6; 10 min (UCB domyślny z
#               fazy "selektory")
#   budzet      3 selektory przy 1, 5 i 30 min (10 min z fazy     5850  ~4,8 h
#               "selektory")
#   razem                                                         15600 ~11,5 h
#
# Przy jednym przebiegu na wątek dwa przebiegi dzielą fizyczny rdzeń (SMT),
# więc każdy robi mniej iteracji na sekundę niż sam na rdzeniu. Wszystkie
# konfiguracje mają te same warunki, więc porównania między nimi są uczciwe.
#
# Wynik każdego przebiegu trafia do CSV od razu po jego zakończeniu, a każda
# faza startuje z --resume: ponowne zlecenie tego samego skryptu (po
# przekroczeniu limitu czasu albo awarii węzła) dokończy tylko brakujące
# przebiegi i pominie skończone fazy.
#
# Użycie:
#   cd <katalog projektu>          # wyniki i logi trafią do tego katalogu
#   sbatch -A <KONTO> scripts/slurm_exp_matrix_static.sh
#   EXP_PHASES="selektory ablacja" sbatch -A <KONTO> --export=ALL \
#       scripts/slurm_exp_matrix_static.sh                             # wybrane fazy
#   EXP_PREFIX=proba EXP_SEEDS="0 1" EXP_SHORT=1 \
#       sbatch -A <KONTO> --export=ALL scripts/slurm_exp_matrix_static.sh   # próba ~10 min

#SBATCH -J exp_static_24h
#SBATCH -p hgx
#SBATCH -w obl1
#SBATCH --exclusive
#SBATCH --mem 0
#SBATCH -t 24:00:00
#SBATCH -o slurm_exp_static_%j.out
#SBATCH -e slurm_exp_static_%j.err

set -euo pipefail

# ── konfiguracja ──────────────────────────────────────────────────────────
# katalog repo: ten, z którego zlecono sbatch (SLURM_SUBMIT_DIR), więc
# zlecaj z katalogu projektu; PROJ=... nadpisuje
PROJ="${PROJ:-$SLURM_SUBMIT_DIR}"
PREFIX="${EXP_PREFIX:-static}"
PHASES="${EXP_PHASES:-selektory akceptacja preset ablacja budzet}"

# 13 instancji w zakresie pracy (wszystkie zasoby przypisane z góry)
INSTANCES="${EXP_INSTANCES:-BR-SA-00 BR-SM-00 BR-SN-00 FI-WP-06 FI-MP-06 FI-PB-98 GR-P3-10 GR-PA-08 GR-H1-97 ZA-LW-09 ZA-WD-09 IT-I4-96 KS-PR-11}"
SELECTORS="random epsilon-greedy ucb"
SEEDS="${EXP_SEEDS:-$(seq -s' ' 0 49)}"
# jeden przebieg na wątek; przy dużym przydziale 6 wątków zostaje dla
# systemu i procesu głównego, przy małym (<= 12) wszystkie idą na przebiegi
THREADS=$(nproc)
if (( THREADS > 12 )); then
    WORKERS="${EXP_WORKERS:-$(( THREADS - 6 ))}"
else
    WORKERS="${EXP_WORKERS:-$THREADS}"
fi
if (( THREADS < 32 )); then
    echo "UWAGA: zadanie dostało tylko $THREADS wątków -- pełna macierz potrwa" \
         "wielokrotnie dłużej niż doba. Zleć je z -c <liczba CPU>." >&2
fi

# budżety w sekundach; EXP_SHORT=1 skraca wszystko do próby
MAIN_T=600
BUDGETS="60 300 1800"
if [[ "${EXP_SHORT:-0}" == "1" ]]; then
    MAIN_T=30
    BUDGETS="10 20"
fi

# wyniki i logi w katalogu projektu (CSV dopisywane po każdym przebiegu,
# więc --resume działa po ponownym zleceniu z tego samego katalogu)
RESULTS_DIR="$PROJ/data/results/${PREFIX}"
LOGS_DIR="$PROJ/data/logs_${PREFIX}"

echo "=== Eksperymenty bez LLM (PUT cluster) ==="
echo "Node      : $(hostname)  ($(nproc) wątków)"
echo "Date      : $(date)"
echo "Job ID    : $SLURM_JOB_ID"
echo "Project   : $PROJ  ($(git -C "$PROJ" rev-parse --short HEAD))"
echo "Fazy      : $PHASES"
echo "Instancje : $INSTANCES"
echo "Seedy     : $(echo "$SEEDS" | wc -w)"
echo "Workers   : $WORKERS"
echo "Wyniki    : $RESULTS_DIR/"
echo ""

mkdir -p "$RESULTS_DIR" "$LOGS_DIR"
cd "$PROJ"

# przygotowanie: pliki LFS (archiwum instancji leży w LFS) i środowisko uv;
# oba kroki są idempotentne, przy kolejnym zleceniu nic nie robią
git lfs pull
uv sync
if head -c 200 data/xhstt2014/XHSTT-2014.xml | grep -q "git-lfs"; then
    echo "BŁĄD: XHSTT-2014.xml to wskaźnik LFS, nie instancje (git lfs pull nie zadziałał)." >&2
    exit 1
fi

# run NAZWA LIMIT_S [argumenty experiments.py...]
run() {
    local name="$1" limit="$2"
    shift 2
    echo ""
    echo "────── $name (${limit}s / przebieg) — start $(date +%H:%M) ──────"
    mkdir -p "$LOGS_DIR/$name"
    # shellcheck disable=SC2086
    uv run python experiments.py \
        --instances $INSTANCES \
        --seeds $SEEDS \
        --time-limit "$limit" \
        --workers "$WORKERS" \
        --log-dir "$LOGS_DIR/$name" \
        --output "$RESULTS_DIR/${name}.csv" \
        --resume \
        "$@"
    echo "────── $name — koniec $(date +%H:%M) ──────"
}

for phase in $PHASES; do
    case "$phase" in
        selektory)
            # shellcheck disable=SC2086
            run selektory "$MAIN_T" --selectors $SELECTORS ;;
        akceptacja)
            # shellcheck disable=SC2086
            run akceptacja_lahc "$MAIN_T" --selectors $SELECTORS --acceptance lahc ;;
        preset)
            # shellcheck disable=SC2086
            run preset_bazowy "$MAIN_T" --selectors $SELECTORS --preset bazowy ;;
        ablacja)
            run ablacja_ucb "$MAIN_T" --selectors \
                ucb-c=0.005 ucb-c=0.1 ucb-c=0.5 ucb-a=0.005 ucb-a=0.08 ucb-2state ;;
        budzet)
            for t in $BUDGETS; do
                # shellcheck disable=SC2086
                run "budzet_${t}s" "$t" --selectors $SELECTORS
            done ;;
        *)
            echo "Nieznana faza: $phase" >&2; exit 1 ;;
    esac
done

echo ""
echo "=== Zakończono $(date) ==="
echo "CSV        : $RESULTS_DIR/"
echo "Logi JSONL : $LOGS_DIR/"
