# Pula heurystyk ręcznych — pierwszy krok (move, swap, operator naprawczy)

## Kontekst

`CLAUDE.md` (sekcja "Projekt: LLM-Driven RL Hyper-Heuristic...") definiuje docelową
architekturę solvera: pętla lokalnego przeszukiwania, w której **selektor RL wybiera
heurystykę z puli**, a każda heurystyka (ręczna lub wygenerowana przez LLM) implementuje
identyczny kontrakt `apply(solution, instance, rng) -> Solution`. Obecny kod (`lahc.py` +
`moves.py`) tego kontraktu jeszcze nie ma: `moves.py` udostępnia trzy zwykłe funkcje o
sygnaturze `move_fn(instance, solution, rng)`, twardo wpisane na listę `_DEFAULT_MOVES` w
`lahc.py`, bez żadnej metadanej (id, nazwa, flaga `protected`) i bez pojęcia "puli".

Odpowiada to GitHub Epic 4 "[EPIC 4] Pula heurystyk ręcznych" (#27), a konkretnie trzem z
jego siedmiu zadań: E4-T1 "Heurystyka move" (#28), E4-T2 "Heurystyka swap" (#29) i E4-T5
"Operator naprawczy naruszeń twardych" (#32). Pozostałe zadania epika (Kempe chain,
ruin-and-recreate, duża perturbacja) są świadomie poza zakresem tego kroku — patrz
"Poza zakresem" niżej.

Etap 3 specyfikacji ("Delta evaluation") formalnie poprzedza Etap 4, ale nie jest jeszcze
zaimplementowany. Decyzja: robimy Etap 4 teraz i świadomie akceptujemy, że heurystyki
oceniające kandydatów (`move_best`, operator naprawczy) będą wołać pełny
`evaluator_ref.total_cost` per kandydat, zamiast przyrostowej oceny. To nie jest błąd —
to świadomy koszt wydajnościowy, który zniknie, gdy powstanie delta evaluation.

## Zakres

W zakresie:
- Nowy plik `xhstt_core/heuristics.py`: dataclass `Heuristic` (kontrakt-wrapper) + cztery
  heurystyki (`move_random`, `move_best`, `swap`, `repair_hard_violation`) + lista
  `MANUAL_HEURISTICS`.
- Nowy plik `tests/test_heuristics.py`: test strukturalny (parametryczny nad
  `MANUAL_HEURISTICS`) + testy specyficzne dla `move_best` i `repair_hard_violation`.

Poza zakresem:
- Kempe chain (E4-T3), ruin-and-recreate (E4-T4), duża perturbacja (E4-T6) — pozostałe
  zadania Epic 4, osobne kroki.
- Jakakolwiek zmiana `moves.py` lub `lahc.py` — stare trzy funkcje (`time_reassign_move`,
  `time_swap_move`, `resource_reassign_move`) zostają nietknięte, pula z tego kroku **nie
  jest** jeszcze podpięta do pętli solvera. Podpięcie puli (i ewentualna migracja starych
  ruchów na nowy kontrakt) to część Etapu 5/6.
- Delta evaluation (Etap 3) — świadomie pominięte, patrz "Kontekst".
- Selektor UCB, flaga `protected` w akcji (usuwanie heurystyk) — Etap 6, tu tylko pole na
  dataclassie, ustawione na `True`.

## Kontrakt (`xhstt_core/heuristics.py`)

```python
@dataclass(frozen=True)
class Heuristic:
    id: str
    name: str
    protected: bool
    apply: Callable[[Solution, Instance, random.Random], Solution]
```

- `apply(solution, instance, rng) -> Solution` — **kolejność argumentów odwrócona**
  względem `moves.py` (`move_fn(instance, solution, rng)`), zgodnie z issue #28. To
  świadoma niespójność między dwoma modułami do czasu przyszłej migracji `moves.py`.
- Nigdy nie modyfikuje `solution` in-place (jak dziś `moves.py`: `dataclasses.replace` +
  strukturalne współdzielenie reszty listy `events`).
- Gdy ruch jest niemożliwy w danym stanie (np. brak naruszeń do naprawy), podnosi
  `ValueError` — ta sama konwencja co dziś (`lahc.py`'s `run_lahc` już łapie `ValueError`
  i pomija iterację; tu nikt jeszcze tego nie woła z pętli solvera, ale konwencja jest
  zachowana na przyszłość).
- `protected=True` dla wszystkich czterech wpisów (heurystyki ręczne — w przyszłym
  selektorze z Etapu 6 nigdy nie są usuwane z puli).
- `MANUAL_HEURISTICS: list[Heuristic]` — lista czterech instancji, w kolejności
  `move_random`, `move_best`, `swap`, `repair_hard_violation`.

## Heurystyki

### `move_random(solution, instance, rng)`

Cienki wrapper: `return moves.time_reassign_move(instance, solution, rng)`. Losowe
zdarzenie → losowy strukturalnie poprawny czas (patrz `valid_start_time_ids`).

### `move_best(solution, instance, rng)`

1. Wybór zdarzenia — identyczna metoda co `time_reassign_move`
   (`rng.randrange(len(solution.events))`), żeby zużycie losowości było analogiczne.
2. Kandydaci: **cały** `valid_start_time_ids(instance, event.duration)`, **włącznie**
   z obecnym `time_ref` (poprawka względem wcześniejszej wersji tego dokumentu, która
   wykluczała obecny czas — patrz "Korekta" niżej). Pusta lista (zdarzenie w ogóle nie ma
   poprawnego czasu — nie powinno się zdarzyć dla zdarzenia już umieszczonego w
   rozwiązaniu, ale sprawdzane jako guard) → `ValueError`.
3. Spośród kandydatów wybierany jest ten, który po podstawieniu (przez
   `dataclasses.replace`) daje **najniższy `evaluator_ref.total_cost`** całego
   rozwiązania. Remisy rozstrzygane deterministycznie — pierwszy w kolejności zwróconej
   przez `valid_start_time_ids` (bez dodatkowego losowania: `min()` po liście w stałej
   kolejności). Skoro obecny czas jest jednym z kandydatów, wynik **nigdy nie jest gorszy**
   niż wejście — to gwarantuje test "`move_best` nie pogarsza kosztu" (patrz "Testy") oraz
   monotoniczność, na której polega `repair_hard_violation`.
4. Logika kroków 2–3 wydzielona jako prywatny helper
   `_best_time_for_event(instance, solution, event_index) -> Solution`, reużywany też
   przez `repair_hard_violation`.

Koszt: O(liczba kandydatów) pełnych `total_cost` — patrz uwaga o delta evaluation w
"Kontekst".

### `swap(solution, instance, rng)`

Cienki wrapper: `return moves.time_swap_move(instance, solution, rng)`. Przypadek "dwa
zdarzenia mają ten sam `time_ref`" nie wymaga specjalnej obsługi — zamiana dwóch
identycznych czasów daje strukturalnie poprawny no-op (nie błąd, nie naruszenie
niezmienników), więc subtask E4-T2 "Obsługa przypadku gdy zdarzenia mają ten sam slot"
jest spełniony przez to, że taki przypadek po prostu nie psuje niczego.

### `repair_hard_violation(solution, instance, rng)`

1. `occurrences = resolve_occurrences(instance, solution)`; dla każdego ograniczenia
   `Required=true` policz koszt (`evaluate_constraint`); zbierz te z kosztem > 0.
2. Brak naruszonych ograniczeń wymaganych → `ValueError`.
3. Zbiór id zdarzeń "uczestniczących w naruszeniu" = suma
   `evaluator_ref._events_in_applies_to(instance, c.applies_to)` po wszystkich
   naruszonych ograniczeniach (ten sam prywatny helper, który już dziś importuje
   `moves.py` — ustalony precedens korzystania z niego poza `evaluator_ref.py`).
4. Zawężenie do **indeksów** w `solution.events` (nie samych id!) — ten sam poziom
   granularności, na którym operują `move_random`/`move_best` — których `event_ref`
   należy do zbioru z kroku 3 i mają ustawione `time_ref`. Rozróżnienie indeks-vs-id ma
   znaczenie dla zdarzeń podzielonych (`SplitEventsConstraint`): każdy kawałek to osobny,
   niezależnie przesuwalny wpis w `solution.events` pod tym samym `event_ref`. Zdarzenia
   w pełni preprzypisane nie mają w ogóle wpisu w `Solution` (patrz
   `construct.build_initial`), więc są automatycznie wykluczone.
5. Pusty wynik (wszystkie naruszające zdarzenia są nieruchome) → `ValueError`.
6. Wybór jednego indeksu losowo spośród pozostałych kandydatów, przesunięcie na
   najlepszy czas przez `_best_time_for_event(instance, solution, index)` (jak w
   `move_best`) — nie czysto losowe przesunięcie, żeby operator faktycznie miał charakter
   naprawczy: całościowy koszt rozwiązania (uwzględniający wagę infeasibility
   ×1 000 000) nie może wzrosnąć.

## Testy (`tests/test_heuristics.py`)

- Fixture: `tests/fixtures/ArtificialSudoku4x4.xml` (już używana w `test_moves.py`; ma
  `AvoidClashesConstraint Required="true"`, więc `build_initial` regularnie produkuje
  naruszenia twarde na losowym seedzie — dobra baza pod testy `repair_hard_violation`).
- **Test strukturalny** (parametryczny nad `MANUAL_HEURISTICS`, część E4-T7 dla tych
  czterech heurystyk): dla każdej — te same zdarzenia co na wejściu (żaden nie
  zgubiony/zduplikowany, po `event_ref`), wejściowy `Solution` niezmutowany (porównanie
  `time_ref`/`resources` przed i po), wynik jest instancją `Solution`. Heurystyki mogące
  podnieść `ValueError` na konkretnym seedzie próbują serii seedów (0..N), tak jak dziś
  robi to `test_moves.py` dla `time_swap_move`/`resource_reassign_move`.
- **Test `move_best` nie pogarsza kosztu**: `total_cost` po zastosowaniu ≤ `total_cost`
  przed, na kilku seedach.
- **Test `repair_hard_violation` — statystyczny**: N uruchomień (różne seedy) na
  rozwiązaniu z ≥1 naruszeniem `Required` (z `build_initial` na fixture'ze Sudoku) —
  `infeasibility` po (suma kosztów ograniczeń `Required`) nie jest większa niż przed, dla
  każdego uruchomienia.
- **Test `repair_hard_violation` podnosi `ValueError` bez naruszeń**: ręcznie skonstruowany
  `Solution` spełniający wszystkie ograniczenia wymagane (albo instancja bez ograniczeń
  `Required` w ogóle) → `pytest.raises(ValueError)`.

## Ryzyka / otwarte pytania rozstrzygnięte podczas brainstormingu

- **Kolejność Etap 3 vs Etap 4**: świadomie pominięte, patrz "Kontekst".
- **Rozjazd sygnatur `moves.py` vs `heuristics.py`**: świadomie zaakceptowany, jako
  przejściowy stan do migracji w Etapie 5/6.
- **Duplikacja logiki `move_random`/`swap` względem `moves.py`**: brak — to cienkie
  wrappery, jedno wywołanie deleguje do drugiego, więc nie ma dwóch kopii tej samej
  logiki do utrzymania.

## Korekta wprowadzona podczas pisania planu implementacji

Pierwotna wersja `_best_time_for_event` (użyta przez `move_best` i
`repair_hard_violation`) wykluczała obecny `time_ref` zdarzenia ze zbioru kandydatów —
tak jak robi to `time_reassign_move`. To była **sprzeczność wewnętrzna** ze specyfikacją
testów w tym samym dokumencie ("`move_best` nie pogarsza kosztu",
"`repair_hard_violation` nie zwiększa infeasibility"): jeśli wszystkie alternatywne
czasy są gorsze niż obecny, wykluczenie obecnego czasu zmusza heurystykę do pogorszenia
rozwiązania, łamiąc obie te gwarancje. Poprawka: kandydaci obejmują **też** obecny
`time_ref`, więc wynik nigdy nie jest gorszy niż wejście (sekcja `move_best` wyżej już
zawiera poprawioną wersję).
