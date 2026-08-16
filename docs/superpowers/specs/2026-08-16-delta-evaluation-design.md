# Ewaluacja przyrostowa (delta evaluation) — Etap 3

## Kontekst

CLAUDE.md, Etap 3: "Przyrostowe przeliczanie kosztu przy przesunięciu jednego zdarzenia. DoD:
test pytest: 10 000 losowych ruchów, po każdym asercja delta == pełna ewaluacja; benchmark
pokazujący przyspieszenie." Punkt 6 architektury rozszerza to na "Ewaluacja przyrostowa (delta)
dla ruchów z puli podstawowej; po heurystykach od LLM-a pełna ewaluacja. Inwariant testowany w
pytest: po dowolnej sekwencji ruchów koszt liczony przyrostowo == koszt liczony od zera."

Repo dziś (`xhstt_core/evaluator_ref.py`) liczy koszt zawsze od zera:
`evaluate_cost_components`/`total_cost` przechodzą po **wszystkich** ograniczeniach instancji i
dla każdego przeliczają jego pełny zakres (`AppliesTo` — zasoby/zdarzenia/grupy/pary), niezależnie
od tego, jak mały był poprzedni ruch. To już dziś jest kosztem świadomie zaakceptowanym w
`docs/superpowers/specs/2026-08-01-wire-heuristic-pool-into-lahc-design.md` ("Zniknie po Etapie 3,
nie teraz") — `heuristics.py::move_best`/`repair_hard_violation` wołają pełny koszt per kandydat
czasu.

Ten krok buduje moduł ewaluacji przyrostowej **w izolacji** — bez podpinania go do `lahc.py` czy
`heuristics.py`. Podpięcie (zamiana wywołań `evaluate_cost(...)` na `delta_cost(...)` w gorących
ścieżkach) to świadomie osobny, następny krok — decyzja użytkownika podjęta podczas
brainstormingu.

## Zakres

W zakresie:
- Nowy moduł `xhstt_core/delta.py` z publiczną funkcją `delta_cost(instance, old_solution,
  old_cost, new_solution) -> Cost`.
- Generyczny interfejs oparty o diff dwóch rozwiązań (nie o typ ruchu) — pokrywa automatycznie
  wszystkie 8 heurystyk z `MANUAL_HEURISTICS` (`move_random`, `move_best`, `swap`,
  `repair_hard_violation`, `kempe_chain`, `ruin_and_recreate`, `large_perturbation`,
  `resource_reassign`), bo żadna z nich nie zmienia `event_ref` ani długości `solution.events` —
  tylko `time_ref`/`resources` wybranych pozycji.
- `tests/test_delta.py`: invariant test na 10 000 ruchach (łańcuchowanych, nie od stałego punktu
  startowego) + testy brzegowe.
- `scripts/benchmark_delta_evaluation.py`: prosty pomiar `time.perf_counter`, bez nowej zależności.

Poza zakresem:
- Podpięcie `delta_cost` do `lahc.py` (pętla akceptacji) i `heuristics.py`
  (`move_best`/`repair_hard_violation`) — osobny następny krok.
- Zmiany w `evaluator_ref.py`, `moves.py`, `heuristics.py` — zero zmian, moduł tylko konsumuje
  istniejące (już współdzielone z `heuristics.py`) helpery z podkreśleniem.
- Prawdziwa delta *per punkt aplikacji* (np. tylko jeden zasób z listy w
  `AvoidClashesConstraint`, nie cała lista) — rozważona i odrzucona na etapie brainstormingu:
  wymagałaby przepisania 13 funkcji `_evaluate_*_constraint`, wysokie ryzyko rozjazdu z pełnym
  ewaluatorem wbrew priorytetowi #1 (poprawność). Możliwa przyszła optymalizacja, jeśli benchmark
  z tego kroku pokaże, że to za mało.

## Projekt

### `xhstt_core/delta.py`

```python
from dataclasses import dataclass

from xhstt_core.cost import Cost
from xhstt_core.evaluator_ref import (
    _assigned_resource_ids,
    _build_occupancy_index,
    _events_in_applies_to,
    _resources_in_applies_to,
    evaluate_constraint,
    resolve_occurrences,
)
from xhstt_core.model import Instance, Solution
from xhstt_core import evaluator_ref


def delta_cost(
    instance: Instance,
    old_solution: Solution,
    old_cost: Cost,
    new_solution: Solution,
) -> Cost:
    """Koszt new_solution, liczony przyrostowo względem old_solution i jego
    znanego kosztu old_cost -- numerycznie identyczny z
    evaluate_cost(instance, new_solution), ale przelicza tylko te
    ograniczenia, których AppliesTo dotyka czegoś, co się faktycznie
    zmieniło. Zakłada, że new_solution powstało z old_solution przez ruch z
    MANUAL_HEURISTICS (ten sam event_ref na każdej pozycji, ta sama liczba
    SolutionEvent) -- rzuca ValueError, jeśli to założenie nie trzyma."""
    old_occurrences = resolve_occurrences(instance, old_solution)
    new_occurrences = resolve_occurrences(instance, new_solution)
    if len(old_occurrences) != len(new_occurrences):
        raise ValueError(
            "old_solution and new_solution resolve to a different number of "
            "occurrences -- delta_cost only supports moves that preserve "
            "event/split structure (see MANUAL_HEURISTICS)"
        )

    changed = [
        k for k in range(len(old_occurrences)) if old_occurrences[k] != new_occurrences[k]
    ]
    if not changed:
        return old_cost

    touched_events = {old_occurrences[k].event_ref for k in changed}
    touched_resources = {
        r
        for k in changed
        for r in _assigned_resource_ids(old_occurrences[k])
        + _assigned_resource_ids(new_occurrences[k])
        if r is not None
    }

    old_index = _build_occupancy_index(instance, old_occurrences)
    new_index = _build_occupancy_index(instance, new_occurrences)

    infeasibility, objective = old_cost.infeasibility, old_cost.objective
    for c in instance.constraints:
        if not _constraint_touches(instance, c, touched_events, touched_resources):
            continue
        evaluator_ref._current_occupancy_index = old_index
        try:
            old_contribution = evaluate_constraint(instance, old_occurrences, c)
        finally:
            evaluator_ref._current_occupancy_index = None
        evaluator_ref._current_occupancy_index = new_index
        try:
            new_contribution = evaluate_constraint(instance, new_occurrences, c)
        finally:
            evaluator_ref._current_occupancy_index = None
        delta = new_contribution - old_contribution
        if delta == 0:
            continue
        if c.required:
            infeasibility += delta
        else:
            objective += delta

    return Cost(infeasibility, objective)


def _constraint_touches(instance, constraint, touched_events, touched_resources) -> bool:
    if touched_events & _events_in_applies_to(instance, constraint.applies_to):
        return True
    if touched_resources & _resources_in_applies_to(instance, constraint.applies_to):
        return True
    return any(
        pair.first_event in touched_events or pair.second_event in touched_events
        for pair in constraint.applies_to.event_pairs
    )
```

(Sygnatury pomocnicze orientacyjne — dokładne typy/importy do dopracowania w planie
implementacji, np. `_constraint_touches` przyjmie `Constraint`, `frozenset[str]`.)

**Dlaczego to jest poprawne:** `evaluate_constraint` dla ograniczenia, którego zakres NIE
przecina się z `touched_events`/`touched_resources`, dostałoby identyczne dane wejściowe (te same
`Occurrence` na każdej niezmienionej pozycji) w starej i nowej wersji — więc jego wkład do kosztu
jest gwarantowanie identyczny i pomijanie go jest bezpieczne, nie tylko szybkie. To jedyny
"nowy" element logiki w tym module; sama matematyka ograniczeń (13 funkcji
`_evaluate_*_constraint`) pozostaje nietknięta i wołana bez zmian.

**Zarządzanie `_current_occupancy_index`:** to już istniejący, modułowy (nie thread-safe z
założenia) mechanizm w `evaluator_ref.py`, którego dziś używa tylko
`evaluate_cost_components` przez `try/finally`. `delta_cost` używa dokładnie tego samego wzorca,
tylko dwa razy (raz per `old`/`new`) i tylko dla dotkniętych ograniczeń — bez zmian w
`evaluator_ref.py`.

### `tests/test_delta.py`

- `test_delta_cost_matches_full_evaluation_over_10000_random_moves` — instancja
  `BrazilInstance1.xml` (realna, różnorodne ograniczenia, już używana w
  `tests/test_evaluator_ref.py` do weryfikacji przez HSEval). Start: `build_initial(instance,
  random.Random(0))`, `old_cost = evaluate_cost(instance, initial)`. Pętla 10 000 iteracji:
  losuje heurystykę z `MANUAL_HEURISTICS`, próbuje `heuristic.apply(old_solution, instance,
  rng)` (łapiąc `ValueError` — pomija iterację, part tej samej pętli co `lahc.py`), liczy
  `delta_cost(instance, old_solution, old_cost, new_solution)`, asercja `==
  evaluate_cost(instance, new_solution)` (pełna, od zera), **łańcuchuje**:
  `old_solution, old_cost = new_solution, new_cost` — testuje więc dowolną *sekwencję* ruchów,
  zgodnie z inwariantem z CLAUDE.md, a nie 10 000 niezależnych ruchów od tego samego punktu.
- `test_delta_cost_returns_old_cost_unchanged_when_nothing_changed` — `delta_cost(instance, s,
  cost, s)` (to samo rozwiązanie) zwraca `cost` bez wołania `evaluate_constraint` ani razu
  (weryfikowalne np. przez monkeypatch/licznik wywołań, albo po prostu asercją wartości — do
  ustalenia w planie).
- `test_delta_cost_raises_on_mismatched_occurrence_structure` — sztucznie skonstruowane
  `new_solution` z inną liczbą `SolutionEvent` niż `old_solution` → `ValueError`.

### `scripts/benchmark_delta_evaluation.py`

Wzorowany na istniejących skryptach w `scripts/` (proste, bezargumentowe albo z minimalnym CLI).
Ładuje `AU-BG-98` z `data/xhstt2014/XHSTT-2014.xml` (ta sama instancja profilowana wcześniej w
`moves.py`/`heuristics.py`, ~765 zdarzeń — realny rozmiar, nie sztuczna mikro-instancja).
Buduje `initial`, wykonuje N (np. 2000) losowych ruchów z `MANUAL_HEURISTICS`, mierzy
`time.perf_counter()` osobno dla ścieżki "pełna ewaluacja za każdym razem" i "delta_cost
łańcuchowany", wypisuje czasy i współczynnik przyspieszenia na stdout. Bez asercji (to skrypt
diagnostyczny do rozdziału pracy, nie test) i bez nowej zależności (`pytest-benchmark` etc.).

## Ryzyka / decyzje rozstrzygnięte podczas brainstormingu

- **Zakres kroku**: tylko `delta.py` + testy + benchmark, bez podpięcia do `lahc.py`/
  `heuristics.py` — decyzja użytkownika, spójna z tym, jak poprzednie etapy dzielono w tym
  repo (buduj nietknięte, podepnij osobno).
- **Kształt API**: generyczny `diff(old_solution, new_solution)` zamiast API dedykowanego dla
  pojedynczego przesunięcia zdarzenia — decyzja użytkownika; pokrywa wszystkie 8 heurystyk bez
  zmian w `moves.py`/`heuristics.py`.
- **Algorytm**: "filtrowana pełna reewaluacja" (opcja A z brainstormingu) zamiast prawdziwej
  delty per punkt aplikacji (opcja B) czy cache'u per punkt z inwalidacją (opcja C) — priorytet
  poprawności nad maksymalną wydajnością; opcje B/C zostają jako możliwa przyszła optymalizacja,
  jeśli benchmark z tego kroku pokaże niewystarczające przyspieszenie.
- **Brak zmian w `evaluator_ref.py`**: świadome — mniejsza powierzchnia zmiany, zero ryzyka dla
  już zweryfikowanej przez HSEval pełnej ścieżki ewaluacji.
