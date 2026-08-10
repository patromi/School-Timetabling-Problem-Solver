# Podpięcie puli heurystyk do pętli solvera (LAHC)

## Kontekst

`docs/superpowers/specs/2026-07-31-manual-heuristic-pool-design.md` (i wykonany wg niego
plan) zbudował `xhstt_core/heuristics.py`: dataclass `Heuristic` (kontrakt
`apply(solution, instance, rng) -> Solution`) i cztery heurystyki (`move_random`,
`move_best`, `swap`, `repair_hard_violation`) w liście `MANUAL_HEURISTICS`. Tamten krok
świadomie **nie** podpinał puli do pętli solvera — `xhstt_core/lahc.py` (i przez nią
`run_solver.py`, jedyny prawdziwy CLI) nadal używa wyłącznie trzech starych funkcji z
`xhstt_core/moves.py` (`time_reassign_move`, `time_swap_move`, `resource_reassign_move`),
z listy `_DEFAULT_MOVES`.

Ten krok podłącza `MANUAL_HEURISTICS` do rzeczywistego przebiegu solvera. Zgodnie z
decyzją użytkownika: **czysto losowy wybór, bez żadnego większego mechanizmu** (bez
selektora UCB, bez wag, bez uczenia się) — to jest Etap 6 ("Selektor UCB") ze
specyfikacji w `CLAUDE.md` i świadomie nie wchodzi w zakres tego kroku.

## Zakres

W zakresie:
- Dodanie piątej heurystyki do `xhstt_core/heuristics.py`: `resource_reassign` (wrapper
  na `moves.resource_reassign_move`), żeby solver nie stracił zdolności zmiany
  przypisanych zasobów (nauczyciel/sala) — decyzja użytkownika: włączyć, nie pomijać.
- Zmiana `xhstt_core/lahc.py::run_lahc`, żeby domyślnie losowała z `MANUAL_HEURISTICS`
  (przez `Heuristic.apply`) zamiast ze starych funkcji `moves.py`.
- Ewentualne przetuningowanie budżetu iteracji w
  `tests/test_lahc.py::test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget`,
  jeśli nowa (średnio droższa) pula tego wymaga — zweryfikowane empirycznie, nie
  zgadywane z góry.

Poza zakresem:
- Jakikolwiek mechanizm selekcji poza `rng.choice` (wagi, UCB, uczenie) — to Etap 6.
- Delta evaluation (Etap 3) — nadal nie istnieje; `move_best`/`repair_hard_violation`
  nadal liczą pełny `evaluator_ref.total_cost` per kandydat. Świadomie zaakceptowany
  koszt wydajnościowy (decyzja użytkownika: nowa, wolniejsza domyślna pula jest OK).
- Kempe chain, ruin-and-recreate, duża perturbacja (pozostałe zadania Epic 4) — nadal
  osobne, niezaczęte kroki.
- Zmiana `moves.py` — jego trzy funkcje zostają nietknięte; nadal są bazą dla wrapperów
  w `heuristics.py` (`move_random`→`time_reassign_move`, `swap`→`time_swap_move`, nowy
  `resource_reassign`→`resource_reassign_move`).

## Zmiany

### `xhstt_core/heuristics.py` — piąta heurystyka

Import: dodać `resource_reassign_move` do istniejącego
`from xhstt_core.moves import time_reassign_move, time_swap_move`.

Nowa funkcja (obok `move_random`/`swap`, ten sam wzorzec cienkiego wrappera):

```python
def resource_reassign(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    """Thin wrapper around moves.resource_reassign_move, adapted to the pool's
    apply(solution, instance, rng) argument order. Raises ValueError (via
    resource_reassign_move) if no event resource has a same-type alternative."""
    return resource_reassign_move(instance, solution, rng)
```

Nowy wpis w `MANUAL_HEURISTICS` (na końcu listy, `protected=True` jak pozostałe):

```python
    Heuristic(
        id="resource_reassign",
        name="Reassign an event resource to a same-type alternative",
        protected=True,
        apply=resource_reassign,
    ),
```

Zero zmian w `tests/test_heuristics.py` — parametryczny test strukturalny
(`test_heuristic_preserves_events_and_does_not_mutate_input`) automatycznie obejmie ten
piąty wpis, bo iteruje bezpośrednio po `MANUAL_HEURISTICS` (ten sam mechanizm, który już
"za darmo" obejmował `move_best`/`swap`/`repair_hard_violation` w poprzednim kroku).

### `xhstt_core/lahc.py` — domyślna pula

Obecnie:
```python
from xhstt_core.evaluator_ref import total_cost
from xhstt_core.model import Instance, Solution
from xhstt_core.moves import resource_reassign_move, time_reassign_move, time_swap_move

_DEFAULT_MOVES = [time_reassign_move, time_swap_move, resource_reassign_move]

def run_lahc(
    instance: Instance,
    initial: Solution,
    rng: random.Random,
    history_length: int = 30,
    max_iterations: int = 1000,
    moves: list | None = None,
    on_progress: Callable[[int, int], None] | None = None,
    progress_every: int = 1000,
    progress_seconds: float = 2.0,
) -> tuple[Solution, int]:
    ...
    move_fns = moves if moves is not None else _DEFAULT_MOVES
    ...
    for step in range(max_iterations):
        move_fn = rng.choice(move_fns)
        try:
            candidate = move_fn(instance, current, rng)
        except ValueError:
            continue
        ...
```

Po zmianie:
```python
from xhstt_core.evaluator_ref import total_cost
from xhstt_core.heuristics import MANUAL_HEURISTICS, Heuristic
from xhstt_core.model import Instance, Solution

def run_lahc(
    instance: Instance,
    initial: Solution,
    rng: random.Random,
    history_length: int = 30,
    max_iterations: int = 1000,
    heuristics: list[Heuristic] | None = None,
    on_progress: Callable[[int, int], None] | None = None,
    progress_every: int = 1000,
    progress_seconds: float = 2.0,
) -> tuple[Solution, int]:
    ...
    pool = heuristics if heuristics is not None else MANUAL_HEURISTICS
    ...
    for step in range(max_iterations):
        heuristic = rng.choice(pool)
        try:
            candidate = heuristic.apply(current, instance, rng)
        except ValueError:
            continue
        ...
```

`_DEFAULT_MOVES` i import z `moves.py` znikają z `lahc.py` całkowicie — `moves.py` nie
jest już używany bezpośrednio przez `lahc.py`, tylko pośrednio przez `heuristics.py`'s
wrappery. Parametr zmienia nazwę `moves` → `heuristics`; potwierdzone grepem, że nikt w
repo dziś nie przekazuje `moves=` jawnie (ani `run_solver.py`, ani żaden test), więc to
bezpieczna zmiana.

Reszta pętli `run_lahc` (akceptacja LAHC, bufor `history`, `on_progress`) — bez zmian.

### `run_solver.py`

Zero zmian. Woła `run_lahc(instance, initial, rng, history_length=..., max_iterations=...,
on_progress=..., progress_every=..., progress_seconds=2.0)` bez `moves=`/`heuristics=`,
więc automatycznie dostaje nową domyślną pulę.

### `tests/test_lahc.py`

Cztery z pięciu istniejących testów (`never_returns_a_worse_solution`,
`is_deterministic_given_the_same_seed`, oba testy `on_progress`) używają małych budżetów
iteracji (100–250) na malutkiej instancji `ArtificialSudoku4x4` (16 zdarzeń, 4 czasy, 4
zasoby) — `total_cost` jest tam tani niezależnie od tego, która heurystyka go woła, więc
oczekiwane jest, że przejdą bez zmian.

`test_lahc_reaches_full_feasibility_on_sudoku4x4_within_a_modest_budget` (40 000
iteracji, oczekuje `best_cost == 0`) ma już w komentarzu zastrzeżenie "empirically tuned
for the current construct.py/moves.py behavior" — czyli sam test zakłada, że budżet
może wymagać przetuningowania przy zmianie zestawu ruchów. Nowa pula ma 2 z 5 heurystyk
istotnie droższych (`move_best`, `repair_hard_violation` liczą pełny `total_cost` per
kandydat czasu) — to może zmienić zarówno czas ściany (wolniej per iterację), jak i
**jakość** zbieżności per iterację (te dwie heurystyki nigdy nie pogarszają, więc mogą
zbiegać w mniejszej liczbie iteracji niż czysto losowe stare ruchy). Kierunek zmiany nie
jest oczywisty z góry — do zweryfikowania empirycznie przez faktyczne uruchomienie testu
i obserwację: czy nadal przechodzi w 40 000 iteracji w rozsądnym czasie (sekundy, nie
dziesiątki sekund — to mała instancja); jeśli nie, dostosować `max_iterations` do
najmniejszej wartości, przy której test przechodzi stabilnie (np. przez kilka przebiegów
z różnymi seedami w trakcie developmentu, nie w samym teście).

## Ryzyka / decyzje rozstrzygnięte podczas brainstormingu

- **Performance**: świadomie zaakceptowane wolniejsze domyślne LAHC (decyzja
  użytkownika) — `move_best`/`repair_hard_violation` liczą pełny koszt per kandydat.
  Zniknie po Etapie 3 (delta evaluation), nie teraz.
- **`resource_reassign_move` bez odpowiednika w puli**: rozwiązane — dodany jako piąty
  wpis (decyzja użytkownika: włączyć, dla parytetu jakości rozwiązań względem obecnego
  stanu).
- **Zmiana nazwy parametru `moves`→`heuristics` w `run_lahc`**: świadoma, bezpieczna
  (brak istniejących wywołań z jawnym `moves=`).
