# Strojenie SA i UCB: wnioski (24.11.2026)

Strojenie przeprowadził `scripts/tune_selector.py` na klastrze PUT w dwóch rundach. Cel: znaleźć
parametry symulowanego wyżarzania (SA) i selektora UCB, przy których UCB jest lepszy od losowej
selekcji heurystyk. Pula statyczna, bez LLM, start zachłanny `build_greedy`, budżet liczony w
iteracjach (przebiegi są odtwarzalne z seeda).

- Runda 1 (`data/results/tuning/`): `sa` → `ucb` → `ucb2` → `test100` → `test300`.
  Strojenie na 6 instancjach, seedy 100–129; test na 13 instancjach, seedy 200–249 (100k it.)
  i 200–229 (300k it.).
- Runda 2 (`data/results/tuning_v2/`): `sa2` → `sa2final` → `test2_100` → `test2_300`.
  Siatka t0 {0,01; 0,02; 0,05} × idle {6000; 12000; 25000; bez perturbacji}; test na 13
  instancjach, seedy 300–349 (100k it., 650 bloków) i 300–329 (300k it., 390 bloków).

Wszystkie przebiegi używają SA chłodzonego przez pozostały budżet (`reheat="remaining"`). W
eksperymencie statycznym 1163195 (`data/results/static_v2/`) SA po każdej perturbacji startowało
od nowa (`reheat="full"`), więc temperatura nigdy nie spadała i mediana końcowej temperatury przy
100k i 300k iteracji wynosiła ok. 1e6.

## Najlepsze parametry

| Parametr | random / epsilon-greedy | UCB / UCB z preprocessingiem | Przed strojeniem |
|---|---|---|---|
| SA: T0 (ułamek kosztu startowego) | 0,01 | 0,02 | 0,01 |
| SA: chłodzenie po perturbacji | `remaining` | `remaining` | `full` |
| Perturbacje przy stagnacji | **wyłączone** | **wyłączone** | po 3000 it. bez poprawy |
| `idle_limit` (progi stanów UCB = 1/3 i 2/3) | 6000 | 6000 | 3000 |
| UCB: c | – | 0,02 | 0,02 |
| UCB: alpha | – | 0,02 | 0,02 |
| UCB: premia za nowe najlepsze | – | 0,1 | 0,1 |
| UCB: liczba stanów | – | 6 | 6 |

Uwagi:
- T0 w przedziale 0,01–0,05 ma znikomy wpływ: w dogrywce (`sa2final`) średnie rangi trzech
  najlepszych wariantów różnią się o 0,01–0,15. Rozdział 0,01 / 0,02 wynika z rankingu, nie z
  istotnej różnicy.
- Bez perturbacji `idle_limit` wyznacza już tylko progi stagnacji w stanie UCB (2000 i 4000
  iteracji) oraz moment wyzwolenia generatora LLM (3 × `idle_limit`).
- Parametry UCB (c, alpha, premia, 6 stanów) potwierdziła runda 1; wygrały wartości domyślne.
- Epsilon-greedy nie był strojony osobno: dostał konfigurację SA wygraną przez random.

### Zastosowanie w kodzie

`experiments.py` używa tych ustawień domyślnie (`TUNED_SA_T0`, `STAGNATION_LIMIT = 6000`,
`--reheat remaining`, perturbacje wyłączone). Pozostałe flagi:

```bash
uv run python experiments.py ...                             # konfiguracja ze strojenia
uv run python experiments.py --perturbations ...             # z perturbacjami po --idle-limit it.
uv run python experiments.py --untuned-sa ...                # T0 = 0,01 dla wszystkich selektorów
uv run python experiments.py --preset przed_strojeniem ...   # jak eksperyment statyczny 1163195
uv run python experiments.py --preset bazowy ...             # jak przed issue #82
```

Kolumna `acceptance_detail` w CSV zapisuje T0 i tryb chłodzenia, a `run_config` w logu JSONL
zapisuje `idle_limit` i progi stanów.

## Wnioski

### 1. Strojenie SA dało więcej niż wybór selektora

Odsetek planów wykonalnych na 13 instancjach (rundy liczone na różnych seedach, test Fishera):

| Selektor | 100k: runda 1 → 2 | p | 300k: runda 1 → 2 | p |
|---|---|---|---|---|
| random | 51,2% → 59,8% | 0,002 | 57,7% → 74,1% | < 0,001 |
| epsilon-greedy | 52,8% → 50,6% | 0,47 | 58,5% → 59,5% | 0,83 |
| UCB | 54,9% → 60,2% | 0,06 | 57,9% → 74,6% | < 0,001 |
| UCB z preprocessingiem | 54,5% → 60,8% | 0,03 | 60,3% → 73,3% | < 0,001 |

Największe zmiany przy 300k: BR-SN-00 z 7% do 93% (random) i z 0% do 93% (UCB), FI-MP-06 z 50%
do 100% (random), ZA-LW-09 z 50% do 87% (random) i z 47% do 93% (UCB), IT-I4-96 z 77% do 100%
(random). Różnice między selektorami w obrębie jednej rundy to najwyżej ok. 1 punkt.

Główna zmiana rundy 2 to wyłączenie perturbacji. W etapie `sa2` (6 instancji) mediana kosztu
miękkiego planów wykonalnych spadła po ich wyłączeniu ze 151,5 do 116 (random) i ze 140 do 110
(UCB). Wykonalność random się nie zmieniła (70,4% → 70,6%), a UCB wzrosła z 72,6% do 78,3% (bez
testu istotności).

### 2. UCB względem random po strojeniu

Test w blokach (instancja, seed), korekta Holma dla 6 par.

- **100k:** UCB jest lepszy na koszcie (ranga 2,35 wobec 2,61, p = 0,0002), a UCB z
  preprocessingiem także (p = 0,01). Przewaga pochodzi z kosztu miękkiego: gdy oba plany są
  wykonalne, UCB wygrywa w 198 parach, przegrywa w 101, a mediana kosztu miękkiego spada o 10,5%.
  Wykonalność jest równa (58 do 60 par, w których plan wykonalny ma tylko jeden selektor).
- **300k:** brak istotnych różnic ani w koszcie, ani w wykonalności; mediana zmiany kosztu
  miękkiego wynosi 0%.
- **Instancje:** UCB jest lepszy na KS-PR-11 (50 wygranych : 0 porażek przy 100k i 30 : 0 przy
  300k), GR-P3-10 i IT-I4-96 (100k); gorszy na BR-SA-00 (12 : 37 i 2 : 27) i FI-MP-06 (300k).
  Lepszą średnią rangę ma na 7 z 13 instancji przy 100k i na 6 z 13 przy 300k.
- **Koszt twardy przy obu niewykonalnych:** remis (82 : 81 przy 100k, 32 : 37 przy 300k). W
  rundzie 1 UCB był tu wyraźnie gorszy (81 porażek na 118 par przy 100k). Możliwe, że szkodziły
  perturbacje; tego nie sprawdzono bezpośrednio.
- **UCB z preprocessingiem** jest najszybszy (989 s wobec 1183 s dla random przy 100k), ale gdy
  oba plany są niewykonalne, ma wyższy koszt twardy (p < 0,001), głównie przez ZA-WD-09 (mediana
  90 naruszeń wobec 68,5 u random).

### 3. Epsilon-greedy: najlepszy koszt miękki, najgorsza wykonalność

- Gdy oba plany są wykonalne, koszt miękki jest niższy niż u random (mediana −25% przy 100k i −20%
  przy 300k, 218 : 31 par przy 100k).
- Wykonalność jest istotnie niższa (p < 0,001): BR-SA-00 44% wobec 100% u random (100k), BR-SN-00
  3% wobec 93% i ZA-LW-09 23% wobec 87% (300k).
- To jedyny selektor, który nie zyskał na rundzie 2. Hipoteza: epsilon-greedy w 90% wyborów jest
  zachłanny i bez perturbacji nie wychodzi z utknięcia. Wynik trzeba opisywać z zastrzeżeniem, że
  epsilon-greedy dostał konfigurację SA wygraną przez random.

### 4. Instancje nadal nierozwiązane

BR-SM-00 i ZA-WD-09 mają 0% planów wykonalnych dla wszystkich selektorów i budżetów; FI-PB-98
osiąga tylko 10% (random, 300k).

### 5. Które heurystyki UCB wybiera w stanie niewykonalnym

Analiza `scripts/analyze_ucb_states.py` na logach z rundy 1 (`test100`, `test300`), 4 trudne
instancje (BR-SM-00, BR-SN-00, FI-PB-98, ZA-WD-09), kroki od 20% budżetu, gdy żaden plan nie był
jeszcze wykonalny. Tabele dla każdej instancji: `data/results/tuning/ucb_states_test100.csv` i
`ucb_states_test300.csv`.

| Heurystyka | Udział wywołań UCB (100k / 300k) | Udział redukcji kosztu twardego przy random (100k / 300k) |
|---|---|---|
| `ruin_and_recreate` | 31,7% / 49,6% | 20,7% / 16,1% |
| `repair_hard_violation` | 20,9% / 17,9% | 6,0% / 5,6% |
| `move_best` | 19,2% / 15,3% | 6,6% / 4,6% |
| `repair_clash` | **4,9% / 1,5%** | **60,8% / 69,8%** |

- UCB prawie nie wybiera `repair_clash`, choć ta heurystyka daje większość redukcji kosztu
  twardego. Na jedno wywołanie redukuje go ok. 7 razy (100k) i 19 razy (300k) skuteczniej niż
  `ruin_and_recreate`.
- Prawdopodobne przyczyny (z kodu, bez pomiaru wkładu każdej):
  1. nagroda jest binarna i nie odróżnia poprawy twardej od miękkiej (`src/lahc.py`),
  2. `repair_clash` dostaje `r = 0`, gdy w planie nie ma konfliktu zasobów do naprawienia,
  3. premia eksploracyjna przy c = 0,02 jest rzędu 0,002, więc nie odrabia różnicy Q rzędu 0,06,
     a Q heurystyki zmienia się tylko po jej wywołaniu,
  4. hipoteza: przy random inne ruchy częściej tworzą konflikty, które `repair_clash` naprawia
     (124 poprawy na 1000 wywołań przy random wobec 19 przy UCB, 100k).
- Analiza dotyczy konfiguracji z rundy 1 (z perturbacjami). Dla konfiguracji z rundy 2 nie była
  powtórzona.

## Wniosek do pracy

Dostrojenie SA, przede wszystkim wyłączenie perturbacji, podniosło odsetek planów wykonalnych o
ok. 16 punktów przy 300k iteracji. Wybór selektora zmienia go najwyżej o ok. 1 punkt. UCB poprawia
koszt miękki przy krótszym budżecie (100k) i na części instancji, ale nie pomaga osiągnąć
wykonalności. W fazie niewykonalnej zaniedbuje heurystykę, która najskuteczniej usuwa naruszenia
twarde, co wskazuje na ograniczenie nagrody binarnej.

## Otwarte kwestie

- Nagroda leksykograficzna (decyzja 24.11.2026: najpierw koszt twardy, potem miękki). Dodana
  jako opcja `reward_mode="lex"` w `run_lahc` (`src/lahc.py`, `selector_reward`); domyślnie
  nadal nagroda binarna. W stanie niewykonalnym: 1 za mniej naruszeń twardych, 0,1 za samą
  poprawę miękką; w wykonalnym bez zmian. Test: trzecia runda `tune_selector.py`
  (`reward100`, `reward300`, `report3`; random, `ucb`, `ucb-lex` na konfiguracjach z rundy 2,
  seedy 400–449 i 400–429), uruchamiana przez
  `TUNE_V3=1 sbatch -A inz_school_tt --export=ALL scripts/slurm_tune_selector.sh`.
  W `experiments.py` ten wariant to selektor `ucb-lex`. Domyślną zmienimy, jeśli test to
  potwierdzi.
- Strojenie SA osobno dla epsilon-greedy albo test epsilon-greedy z perturbacjami.
- Powtórzenie analizy wyboru heurystyk na logach rundy 2 (`tuning_v2/logs/test2_*`, na klastrze).

## Odtworzenie

```bash
sbatch -A inz_school_tt scripts/slurm_tune_selector.sh                         # runda 1
TUNE_V2=1 sbatch -A inz_school_tt --export=ALL scripts/slurm_tune_selector.sh  # runda 2
uv run python scripts/analyze_ucb_states.py --log-dir data/results/tuning/logs/test100 \
    --out data/results/tuning/ucb_states_test100.csv
```
