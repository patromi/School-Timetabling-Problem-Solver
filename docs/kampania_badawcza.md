# Odtwarzalna kampania badawcza solvera

## Co jest obecnie warte zbadania

Stan na 1 listopada 2026: parser, pełny i przyrostowy ewaluator, ręczne
heurystyki, SA i selektory są zaimplementowane. Generator LLM jeszcze nie jest
zintegrowany. W bieżącym drzewie roboczym są też nowe sąsiedztwa GOAL:
`event_block_swap` oraz `reassign_resource_times` (RRT), uruchamiany domyślnie
poza selektorem. Nie należy traktować starych wyników jako oceny tej nowej puli.

Dotychczasowe eksperymenty pozostawiają trzy istotne pytania:

1. Czy ciasne instancje potrzebują innych sąsiedztw, czy tylko dłuższego szukania?
2. Czy perturbacje pomagają po poprawieniu chłodzenia, czy niszczą postęp?
3. Czy operatory przypisywania zasobów (branch `resource-assignment`) wnoszą
   wartość na instancjach, w których solver wybiera także zasoby?

Kampania bada **wyłącznie selektor UCB**. Warianty `random` i `epsilon-greedy`
są nadal zdefiniowane i dostępne przez `--variants` (porównanie selektorów),
ale nie wchodzą do zestawów domyślnych.

Nowy [runner](../research_experiments.py) bada te pytania bez zmiany algorytmu
i bez uruchamiania LLM. Testy infrastruktury są w
[test_research_experiments.py](../tests/test_research_experiments.py).

## Macierz i hipotezy

Każdy wariant używa **SA sterowanego numerem iteracji**, tego samego zestawu
seedów oraz jednakowych budżetów iteracji. Czas nie wpływa na nagrodę.

| Wariant | Zmiana względem `reference` | Pytanie |
|---|---|---|
| `reference` | UCB, start zachłanny, perturbacje, reheat remaining, auto idle, RRT co 25 kroków stagnacji | Punkt odniesienia |
| `random` ¹ | Losowa selekcja | Czy uczenie daje przewagę nad samą pulą? |
| `epsilon-greedy` ¹ | Epsilon-greedy | Czy potrzebny jest kontekstowy UCB? |
| `ucb-2state` | Bez kontekstu stagnacji | Czy 6 stanów rozprasza rzadkie sukcesy? |
| `naive-start` | Start niezachłanny | Czy korzyść konstrukcji utrzymuje się po długim szukaniu? |
| `no-goal` | Bez obu operatorów GOAL | Czy nowe sąsiedztwa wnoszą wartość? |
| `no-block-swap` | Bez zamiany bloków | Wkład zamiany bloków |
| `no-rrt` | Bez RRT | Wkład kosztownego przeszukiwania permutacji |
| `rrt-selector` | RRT jako ramię selektora | Czy uczenie źle alokuje wysiłek na drogi operator? |
| `no-perturbations` | Bez perturbacji stagnacyjnych | Czy restart rzeczywiście pomaga? |
| `fixed-idle` | Próg stagnacji 3000 zamiast max(3000, 50 × zdarzenia) | Wpływ progu, bez zmiany chłodzenia |
| `legacy-perturbations` | Jak fixed-idle, ale reheat full | Izolowany wpływ historycznego błędu harmonogramu |
| `no-resource-ops` ² | Bez `resource_swap` i `repair_resource_clash` | Wkład nowych operatorów zasobów ponad `resource_reassign` |
| `no-resource-moves` ² | Bez żadnego operatora zasobów | Zasoby zostają takie, jak ustawił konstruktor: wartość przeszukiwania zasobów |

¹ Poza zestawem domyślnym (tylko UCB). ² Tylko ścieżka `resource`.

`reference` to jawnie zdefiniowany wariant badawczy, **nie** niezmieniona domyślna
konfiguracja starego `experiments.py`: używa `remaining` i automatycznego progu.
Parametry każdego wariantu i lista zadań są zapisane w manifeście.

## Dwie ścieżki instancji

Ścieżkę wybiera runner na podstawie danych (`has_resource_choices`): instancja
jest `resource`, jeśli zostawia solverowi do przypisania zasoby z dziedziną
większą niż jeden. Przestrzeń przeszukiwania i koszt iteracji są inne, więc
ścieżki mają osobne budżety, osobne zestawy wariantów, kolumnę `track` w każdym
CSV, i **nie wolno łączyć ich wyników w jednej konkluzji**.

| Ścieżka | Instancje domyślne | Budżety | Warianty domyślne |
|---|---|---|---|
| `time` | BR-SA-00, GR-H1-97, GR-PA-08, ZA-WD-09, KS-PR-11 | 100 000, 1 000 000 | `reference`, `ucb-2state`, `naive-start`, `no-goal`, `no-block-swap`, `no-rrt`, `rrt-selector`, `no-perturbations`, `fixed-idle`, `legacy-perturbations` |
| `resource` | ES-SS-08, US-WS-09, DK-FG-12, DK-HG-12, DK-VG-09 | 50 000 | `reference`, `ucb-2state`, `naive-start`, `no-goal`, `no-resource-ops`, `no-resource-moves` |

Ścieżka `time`:

- BR-SA-00, GR-H1-97, GR-PA-08: porównanie z wcześniejszymi wynikami wykonalności;
- ZA-WD-09: ciasny plan, test sąsiedztw wychodzących z lokalnego minimum;
- KS-PR-11: większa instancja i długi horyzont poprawy.

Te pięć instancji nie ma nieprzypisanych slotów zasobów. **Nie dowodzi to
ustalonego podziału zdarzeń.** Istniejąca pula nadal zawiera operatory zmieniające
podział; ta kampania bada aktualną implementację, a nie certyfikuje zgodności
całego benchmarku z docelowym zakresem pracy.

Ścieżka `resource` obejmuje instancje bez `LinkEvents` (zakres z `CLAUDE.md`).
Instancje z `LinkEvents` (AU-*, NL-KP-*, UK-SP-06) runner odrzuca z błędem.
Budżet 50 000 jest celowo mały: na US-WS-09 i DK-* iteracja trwa 15–30 ms,
więc 1 000 000 iteracji to ok. 5–8 godzin na przebieg. Po pilocie zwiększ go
przez `--budgets`. DK-* nie osiągają wykonalności w tym horyzoncie; interpretuj je
przez medianę naruszeń twardych (`median_hard`), nie przez odsetek wykonalnych.
Zgodność kosztu z HSEval jest sprawdzana tylko z `--hseval`: bez niego eksport
XML dla instancji zasobowych nie ma zewnętrznego potwierdzenia.

Manifest zawiera profil danych: ścieżkę, liczby zdarzeń, zasobów, czasów,
nieprzypisanych slotów zasobów, zdarzeń o duration > 1 i typy ograniczeń.

## Uruchomienie na maszynie obliczeniowej

Runner działa na Linux, macOS i WSL (blokada katalogu korzysta z `flock`).
W natywnym Windows użyj WSL. Zsynchronizuj istniejące zależności:

```bash
uv sync
uv run python -m pytest tests/test_research_experiments.py --no-cov
```

### 1. Pilot: czas, poprawność, rozmiar logów

```bash
uv run python research_experiments.py \
  --output data/results/pilot_badawczy \
  --instances BR-SA-00 ZA-WD-09 KS-PR-11 \
  --variants reference no-goal no-perturbations \
  --seeds 0 1 --budgets 1000 10000 --workers 4
```

Na podstawie `report/summary.csv` oszacuj czas **osobno dla każdej instancji
i wariantu**. Przybliżenie: suma czasów pilota przemnożonych przez stosunek
docelowego do pilotażowego budżetu, podzielona przez liczbę procesów.
To tylko przybliżenie: częstość RRT i perturbacji zależy od stagnacji.
Sprawdź także wolne miejsce. Domyślna duża macierz może trwać dni, nie tylko noc.

### 2. Kampania eksploracyjna

```bash
# Najpierw tylko liczba zadań, walidacja danych i suma iteracji.
uv run python research_experiments.py \
  --output data/results/kampania_01 --dry-run

# Domyślnie obie ścieżki: 1000 przebiegów `time` (10 wariantów × 5 instancji
# × 10 seedów × 2 budżety) + 300 przebiegów `resource` (6 × 5 × 10 × 1).
uv run python research_experiments.py \
  --output data/results/kampania_01 --workers 4

# Tylko jedna ścieżka (osobny katalog wyników):
uv run python research_experiments.py \
  --output data/results/kampania_zasoby --group resource --workers 4
```

Uruchom komendę we własnej sesji `tmux` albo przez harmonogram klastra.
Runner sam nie przechodzi w tło. Przykładowo:

```bash
tmux new -s timetabling
uv run python research_experiments.py \
  --output data/results/kampania_01 --workers 4
# Odłączenie: Ctrl+B, następnie D. Powrót: tmux attach -t timetabling
```

Nie używaj więcej procesów niż przydzielono rdzeni; na współdzielonej maszynie
zostaw margines na pamięć i obciążenie innych użytkowników.
Przed kampanią zachowaj kod w commicie lub własnej kopii roboczej.
Manifest zapisuje commit oraz hashe **rzeczywistych plików**, również zmian
niezacommitowanych; sam hash nie odtworzy utraconej treści pliku.

### 3. Potwierdzenie wybranej hipotezy

Po analizie eksploracyjnej wybierz z góry jeden lub dwa warianty i nowe seedy.
Nie stroisz parametrów na wynikach potwierdzających.

```bash
uv run python research_experiments.py \
  --output data/results/potwierdzenie_goal \
  --instances ZA-WD-09 KS-PR-11 \
  --variants reference no-goal \
  --seeds 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 \
  --budgets 100000 1000000 --workers 4
```

10 seedów w wielkiej macierzy to diagnoza, nie mocny dowód wszystkich efektów.
Przy 10 parach minimalne dwustronne p testu znaków to 0,001953125.
Korekta wielu porównań może uniemożliwić istotność nawet przy komplecie wygranych.
Dlatego potwierdzenie ma mniej hipotez i więcej niezależnych seedów.

## Bezpieczeństwo wielogodzinnego działania

- Każdy przebieg ma własny katalog `runs/<id>/` z logiem JSONL,
  najlepszym planem XML oraz atomowo zapisanym `result.json`.
- Po Ctrl+C dzieci są zatrzymywane; ta sama komenda wznawia wyłącznie
  nieukończone zadania. Nie ma checkpointu środka pojedynczego przebiegu:
  przerwany przebieg zaczyna się od swojego seeda.
- Blokada nie pozwala dwóm runnerom pisać do jednego katalogu.
  Po śmierci procesu blokada systemowa zwalnia się automatycznie.
- Zmiana kodu, archiwum, wersji Pythona lub konfiguracji wymaga nowego katalogu.
  Liczbę procesów można zmienić przy wznowieniu; budżet iteracji pozostaje ten sam.
- Błąd zapisuje traceback w `error.json`; inne niezależne zadania kontynuują
  pracę. Awaria procesu również jest rozpoznawana, zamiast zostawiać wiszące
  oczekiwanie na wynik.
- Nieudane zadania ponów jawnie flagą `--retry-failed`, z tą samą resztą komendy.
  Jeśli naprawiasz kod, uruchom nową kampanię, nie mieszaj wersji.
- Kod wyjścia 0 oznacza komplet wyników, 1 błędy/braki, 130 przerwanie.
- Raport można odbudować flagą `--report-only` z tą samą konfiguracją, także
  podczas pauzy w kampanii. Raport jawnie pokazuje braki.
- Domyślnie co 1000 kroków sprawdzana jest delta względem pełnej oceny
  dla operatorów przyrostowych; `--verify-every 1` sprawdza każdą taką ocenę.
  Końcowy koszt najlepszego rozwiązania zawsze jest przeliczany od zera.
- Opcjonalne `--hseval` sprawdza każdy zapisany XML i zgodność obu składowych
  kosztu z lokalnym HSEval. Wymaga gotowego pliku wykonywalnego, ewentualnie
  `KHE_HSEVAL_PATH`; jego hash trafia do manifestu. Niezgodność to błąd
  przebiegu, nie wynik sukcesu. Bez tej flagi nie ma walidacji zewnętrznej.

## Co analizować i jakie decyzje z tego wynikają

| Plik w `report/` | Jak czytać | Możliwy dalszy kierunek |
|---|---|---|
| `summary.csv` | Wykonalne / wszystkie, mediana hard; soft tylko dla wykonalnych; mediana czasu | Odróżnienie problemu wykonalności od optymalizacji miękkiej |
| `runs.csv` | Wyniki per seed, pierwsza wykonalność, ostatnia poprawa, perturbacje i escape | Ciężkie ogony czasu do sukcesu; czy warto używać wielu restartów? |
| `comparisons.csv` | Wariant vs reference na wspólnych seedach: wygrane/remisy/przegrane, p i korekta Holma | Wybrane efekty do potwierdzenia na nowych seedach |
| `convergence.csv`, PNG | Koszt hard i wykonalność co 10% budżetu | Dłuższe szukanie nadal pomaga czy następuje plateau? |
| `heuristics.csv` | Wywołania, przyjęte poprawy/ruchy równe/pogorszenia, nowe najlepsze per operator | Operator brakujący w puli vs błędna alokacja przez selektor |
| `perturbations.csv` | Odzyskanie kosztu, kroki do odzyskania, nowy najlepszy | Strojenie progu i podgrzania, nie tylko rozmiaru perturbacji |
| `incomplete.csv` | Błędy i nieukończone zadania | Kontrola kompletności przed wnioskowaniem |

Nie sumuj miękkiego kosztu z naruszeniami twardymi w raporcie. Porównanie
statystyczne używa pary `(hard, soft)` leksykograficznie i dokładnego testu
znaków, bez wykładniczej enumeracji Wilcoxona ze starego raportu.
Remisy są pomijane w teście; ich liczba pozostaje widoczna.
Brak istotności **nie** dowodzi równoważności, szczególnie przy małej próbie.

Koszt miękki dla wykonalnych planów to metryka warunkowa. Wariant, który daje
jeden dobry plan z dziesięciu, nie jest automatycznie lepszy niż wariant dający
dziesięć nieco droższych wykonalnych planów. Najpierw oceń wykonalność.
Nieosiągnięta wykonalność jest `null` w wynikach, a nie czasem równym zero;
w analizie czasu do sukcesu jest obserwacją prawostronnie cenzurowaną.
Pola kroków w `RunStats` używają indeksu od zera; punkty zbieżności liczą
wykonane kroki. Start już wykonalny ma krok i czas 0.

**Równy budżet iteracji nie oznacza równej liczby ewaluacji ani czasu.**
RRT i heurystyki wybierające najlepszy slot oceniają wielu kandydatów wewnętrznie.
Ta kampania daje powtarzalność i ocenę skuteczności na krok, nie certyfikuje
uczciwego porównania wydajności przy równym czasie. Zawsze pokaż obok
wyników czas z `runs.csv`. Po wyborze wariantów przeprowadź dodatkowe
porównanie z równym czasem na tej samej maszynie (istniejący
`experiments.py`), uwzględniając jawnie różnice reheat/progu stagnacji.
Nie porównuj czasów z różnych maszyn jako właściwości samego algorytmu.

Budżety 100 tys. i 1 mln mają inne harmonogramy chłodzenia SA. Krótszy przebieg
**nie jest prefiksem** dłuższego przebiegu, nawet przy tym samym seedzie.
Do oceny plateau pojedynczego przebiegu służą jego punkty zbieżności.
Wpływ operatora na częstotliwość wywołań po jego usunięciu nie jest efektem
przyczynowym pojedynczego ruchu: zmienia się cała trajektoria.

Jeżeli no-goal przegrywa na ZA-WD-09, ale nie na luźniejszych instancjach,
warto rozwijać sąsiedztwa dla ciasnych planów. Jeżeli UCB przegrywa z random
w długim budżecie i przestaje różnicować Q, warto zbadać zapominanie i stan
selektora. Jeżeli perturbacje często nie odrabiają strat, najpierw napraw
mechanizm restartu. Integrację LLM warto rozpocząć dopiero, gdy logi wskażą
konkretną lukę w ręcznej puli, a nie jako zamiennik diagnozy.
