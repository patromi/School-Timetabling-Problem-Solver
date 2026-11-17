#!/bin/bash
# Eksperymenty bez LLM na całą dobę na węźle obl: pięć faz po kolei, każda
# z własnym CSV. Domyślnie 13 instancji w zakresie pracy x 50 seedów, po
# jednym przebiegu na wątek (nproc - 6 równolegle, czyli 250 na obl).
# Budżet wyrażony w iteracjach (--iterations), nie w czasie, więc wyniki są
# powtarzalne niezależnie od szybkości węzła.
#
#   faza        konfiguracje                                   przebiegi
#   selektory   random / epsilon-greedy / ucb, SA, 100k it        1950
#   akceptacja  te same selektory z LAHC, 100k it                 1950
#               (SA do porównania z fazy "selektory")
#   preset      te same selektory, --preset bazowy, 100k it       1950
#               (konfiguracja sprzed issue #82)
#   ablacja     UCB: c 0,005 / 0,1 / 0,5, alfa 0,005 / 0,08,     3900
#               2 stany zamiast 6; 100k it (UCB domyślny z
#               fazy "selektory")
#   budzet      3 selektory przy 10k / 100k / 300k it             5850
#   razem                                                        15600
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
#   sbatch -A <KONTO> -w obl2 scripts/slurm_exp_matrix_static.sh   # konkretny węzeł
#   EXP_PHASES="selektory ablacja" sbatch -A <KONTO> --export=ALL \
#       scripts/slurm_exp_matrix_static.sh                             # wybrane fazy
#   EXP_PREFIX=proba EXP_SEEDS="0 1" EXP_SHORT=1 \
#       sbatch -A <KONTO> --export=ALL scripts/slurm_exp_matrix_static.sh   # próba ~10 min

#SBATCH -J exp_static_24h
# węzły obl1/obl2 (CPU) są w partycji obl -- nie hgx, jak podaje instrukcja
# klastra; bez -w SLURM bierze wolny węzeł obl
#SBATCH -p obl
#SBATCH --exclusive
#SBATCH --mem 0
#SBATCH -t 23:00:00
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

# budżety w iteracjach; EXP_SHORT=1 skraca wszystko do próby
MAIN_ITERS=100000
BUDGETS="10000 100000 300000"
if [[ "${EXP_SHORT:-0}" == "1" ]]; then
    MAIN_ITERS=1000
    BUDGETS="500 1000"
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

# run NAZWA LIMIT_ITERS [argumenty experiments.py...]
run() {
    local name="$1" iters="$2"
    shift 2
    echo ""
    echo "────── $name (${iters} it / przebieg) — start $(date +%H:%M) ──────"
    mkdir -p "$LOGS_DIR/$name"
    # shellcheck disable=SC2086
    uv run python experiments.py \
        --instances $INSTANCES \
        --seeds $SEEDS \
        --iterations "$iters" \
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
            run selektory "$MAIN_ITERS" --selectors $SELECTORS ;;
        akceptacja)
            # shellcheck disable=SC2086
            run akceptacja_lahc "$MAIN_ITERS" --selectors $SELECTORS --acceptance lahc ;;
        preset)
            # shellcheck disable=SC2086
            run preset_bazowy "$MAIN_ITERS" --selectors $SELECTORS --preset bazowy ;;
        ablacja)
            run ablacja_ucb "$MAIN_ITERS" --selectors \
                ucb-c=0.005 ucb-c=0.1 ucb-c=0.5 ucb-a=0.005 ucb-a=0.08 ucb-2state ;;
        budzet)
            for it in $BUDGETS; do
                # shellcheck disable=SC2086
                run "budzet_${it}it" "$it" --selectors $SELECTORS
            done ;;
        *)
            echo "Nieznana faza: $phase" >&2; exit 1 ;;
    esac
done

echo ""
echo "=== Zakończono $(date) ==="
echo "CSV        : $RESULTS_DIR/"
echo "Logi JSONL : $LOGS_DIR/"
