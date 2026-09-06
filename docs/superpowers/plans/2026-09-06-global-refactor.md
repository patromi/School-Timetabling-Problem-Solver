# Globalny refaktoring repo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Uporządkować obecny stan repo (bez zmiany zachowania solvera) — usunąć martwy/duplikowany kod, rozbić dwa przerośnięte moduły (`evaluator_ref.py`, `html_report.py`) na mniejsze, spójne pliki, i zaktualizować przestarzałą dokumentację flow — tak, żeby kod dalej dawał się łatwo wytłumaczyć na obronie.

**Architektura:** To jest refaktoring, nie nowa funkcjonalność — żaden test nie powinien zmienić oczekiwanego wyniku. Każde zadanie jest małym, w pełni odwracalnym krokiem: przenieś/usuń kod, uruchom cały istniejący zestaw testów + mypy + ruff, commit. Podział dużych plików robimy przez wydzielenie **pakietu** (`xhstt_core/evaluator_ref/`, `xhstt_core/html_report/`) z `__init__.py` re-eksportującym dotychczasowe publiczne nazwy, więc żaden import w innych modułach (`from xhstt_core.evaluator_ref import X`) się nie zmienia.

**Tech Stack:** Python 3.12, pytest + pytest-cov, mypy --strict, ruff (E/W/F/I/B/C90/UP/RUF/ANN).

**Spec:** `CLAUDE.md` (sekcje "Architektura", "Bieżący stan implementacji", "Zasady techniczne") + `docs/flow-ukladania-planu.md` (opis obecnego flow — częściowo nieaktualny, patrz Task 4).

## Global Constraints

- **Zero zmiany zachowania.** Żadne zadanie poza Task 8 (encapsulacja globalnego stanu) nie zmienia logiki — tylko lokalizację kodu i drobne, jawnie opisane poprawki (dead code, duplikacja).
- **"Test" w każdym zadaniu = uruchomienie istniejącego zestawu testów**, nie pisanie nowych red/green testów — to jest refaktoring istniejącego, już przetestowanego kodu, nie nowa funkcja. Komenda bazowa po każdym kroku: `uv run python -m pytest -m "not slow"` (pełny zestaw z `-m "slow"` włącznie uruchamiaj tylko przed finalnym commitem całego planu — trwa ~9 min).
- **Typowanie strict wszędzie** — `uv run mypy --follow-imports=silent xhstt_core/<nowy_plik>.py` dla każdego nowego/przeniesionego pliku musi przechodzić bez nowych błędów.
- **Ruff bez nowych ostrzeżeń** — `uv run ruff check xhstt_core/` i `uv run ruff format xhstt_core/ --check` po każdym zadaniu.
- **Commity per zadanie**, nie per faza — zgodnie z konwencją repo (`git log`: `feat:`, `fix:`, `perf:`, `docs:`, `chore:`, `refactor:`).
- Priorytet projektu (z CLAUDE.md): poprawność > powtarzalność > czytelność > wydajność > liczba funkcji — w razie wątpliwości przy podziale plików wybieraj czytelność/prostotę tłumaczenia, nie "eleganckie" abstrakcje.

---

## Ustalenia z analizy obecnego stanu (kontekst dla wszystkich zadań)

Zbadany aktualny flow (`run_solver.py` → `construct.build_initial` → `lahc.run_lahc` z pulą `heuristics.MANUAL_HEURISTICS` (8 operatorów, kontrakt `apply(solution, instance, rng)` **już zaimplementowany** — to więcej niż mówi obecna sekcja CLAUDE.md "Bieżący stan implementacji" i cały `docs/flow-ukladania-planu.md`, patrz Task 4) → `xml_writer` + `html_report`.

Rozmiary modułów `xhstt_core/` (linie): `evaluator_ref.py` 918, `html_report.py` 645, `moves.py` 297, `parser.py` 297, `heuristics.py` 288, `cost.py` 165, `delta.py` 145, `model.py` 139, `construct.py` 110, `xml_writer.py` 90, `lahc.py` 65.

**Konkretne znaleziska (odpowiedź na "co jest niepotrzebne/nadmiarowe"):**

| # | Znalezisko | Status | Decyzja |
|---|---|---|---|
| 1 | `events_by_id` w `evaluator_ref.resolve_occurrences` (linia 131) — przypisywane, nigdy nieużywane (potwierdzone przez `ruff --select F841`) | martwy kod | usunąć (Task 1) |
| 2 | `cost_breakdown()` zduplikowane 1:1 w `run_solver.py:55-67` i `scripts/generate_summary.py:18-29` — obie reimplementują `evaluator_ref.evaluate_cost_components` bez optymalizacji occupancy-index | duplikacja | zamienić na `evaluate_cost_components` (Task 2) |
| 3 | `src/solver/main.py` (`TimetableSolver`) + `tests/test_solver.py` | martwy stub niepowiązany z prawdziwym pipeline'em (sam CLAUDE.md to potwierdza) | usunąć + poprawić `pyproject.toml` `--cov=solver` (Task 3) |
| 4 | `delta.delta_cost` — w pełni zaimplementowane, przetestowane (10k-ruchów invariant), zbenchmarkowane, ale **nigdzie nie wywoływane produkcyjnie** (`lahc.py` woła `evaluate_cost`, nie `delta_cost`) | nie martwe, ale niepodłączone — realna luka względem Etapu 3 CLAUDE.md ("ewaluacja przyrostowa dla ruchów z puli podstawowej") | udokumentować jako lukę (Task 4), NIE podłączać w tym planie (osobna decyzja projektowa, patrz Uwaga na końcu) |
| 5 | `min_working_days_cost`, `AdaGenSchedule`, `resources_of_type` w `cost.py` — jawnie opisane w docstringu modułu jako "opt-in, not wired into anything yet" | celowy, udokumentowany szkic pod przyszłe Etapy 5/6 | zostawić bez zmian |
| 6 | `_PAGE_CSS` w `html_report.py` (linie 309–645, ~336 linii czystego CSS jako string Pythona) | powinno być plikiem `.css`, dokładnie jak już istniejący `assets/fonts.css` w tym samym module | wydzielić (Task 5) |
| 7 | `docs/flow-ukladania-planu.md` sekcja 4 i "Czego w tym flow nie ma (jeszcze)" | nieaktualne — mówi o puli 3 ruchów i braku kontraktu `apply()`, a oba już istnieją (`heuristics.py`) | zaktualizować (Task 4) |

**Pliki, które NIE wymagają podziału** (świadoma decyzja, żeby nie dodawać abstrakcji ponad potrzebę): `moves.py` i `parser.py` (po 297 linii, ale już jedna spójna odpowiedzialność na plik — jedna funkcja na typ ruchu / jedna funkcja na tag XML), `heuristics.py` (288 linii, jedna klasa + cienkie wrappery), `cost.py`, `delta.py`, `model.py`, `construct.py`, `xml_writer.py`, `lahc.py` — wszystkie < 170 linii, jedna odpowiedzialność.

---

## Faza A — Sprzątanie (dead code, duplikacja, dokumentacja)

### Task 1: Usunięcie martwej zmiennej w `evaluator_ref.resolve_occurrences`

**Files:**
- Modify: `xhstt_core/evaluator_ref.py:131`

**Interfaces:** brak zmian publicznego API.

- [ ] **Step 1: Usuń linię**

Usuń linię `events_by_id = {e.id: e for e in instance.events}` (linia 131) — potwierdzone przez `ruff check --select F841`, że nie jest nigdzie w funkcji odczytywana.

- [ ] **Step 2: Zweryfikuj**

```bash
uv run ruff check xhstt_core/evaluator_ref.py --select F841
uv run python -m pytest tests/test_evaluator_ref.py -m "not slow"
```
Oczekiwane: brak ostrzeżeń F841, wszystkie testy przechodzą (to czysto martwy kod — zero zmiany zachowania).

- [ ] **Step 3: Commit**

```bash
git add xhstt_core/evaluator_ref.py
git commit -m "refactor: remove dead events_by_id local in resolve_occurrences"
```

---

### Task 2: Deduplikacja `cost_breakdown`

**Files:**
- Modify: `run_solver.py:55-67` (funkcja `cost_breakdown`)
- Modify: `scripts/generate_summary.py:18-29` (funkcja `cost_breakdown`)

**Interfaces:**
- Consumes: `xhstt_core.evaluator_ref.evaluate_cost_components(instance: Instance, solution: Solution) -> tuple[int, int]` (już istnieje, linia 886 evaluator_ref.py) — zwraca dokładnie `(infeasibility, objective)`, identycznie jak obie lokalne kopie, plus buduje occupancy index (szybsze).
- Produces: brak nowych nazw — obie funkcje wywołujące (`main()` w obu plikach) używają istniejącej nazwy lokalnej `cost_breakdown` albo bezpośrednio `evaluate_cost_components`.

- [ ] **Step 1: `run_solver.py` — usuń lokalną definicję, użyj bezpośrednio**

W `run_solver.py` usuń definicję funkcji `cost_breakdown` (linie 55-67) i import `resolve_occurrences` jeśli stanie się nieużywany po zmianie (sprawdź — jest jeszcze potrzebny na końcu `main()` do `html_report`). Zmień import z:
```python
from xhstt_core.evaluator_ref import evaluate_constraint, resolve_occurrences
```
na:
```python
from xhstt_core.evaluator_ref import evaluate_cost_components, resolve_occurrences
```
Zamień oba wywołania `cost_breakdown(instance, initial)` / `cost_breakdown(instance, best)` na `evaluate_cost_components(instance, initial)` / `evaluate_cost_components(instance, best)`.

- [ ] **Step 2: `scripts/generate_summary.py` — to samo**

Usuń lokalną definicję `cost_breakdown` (linie 18-29). Zmień import:
```python
from xhstt_core.evaluator_ref import resolve_occurrences, evaluate_constraint
```
na:
```python
from xhstt_core.evaluator_ref import evaluate_cost_components
```
(usuń `resolve_occurrences`/`evaluate_constraint` jeśli nieużywane gdzie indziej w pliku — sprawdź resztę `main()`). Zamień wywołanie `cost_breakdown(instance, sol)` na `evaluate_cost_components(instance, sol)`.

- [ ] **Step 3: Zweryfikuj**

```bash
uv run python -m pytest -m "not slow"
uv run python run_solver.py --list
uv run python run_solver.py AU-BG-98 --iterations 200 --seed 0
uv run python scripts/generate_summary.py --instances data/xhstt2014/XHSTT-2014.xml --solutions output/AU-BG-98_solution.xml --csv-output /tmp/summary.csv --md-output /tmp/summary.md
```
Oczekiwane: identyczne liczby infeasibility/objective jak przed zmianą (to jest ta sama funkcja, tylko jedna kopia).

- [ ] **Step 4: Commit**

```bash
git add run_solver.py scripts/generate_summary.py
git commit -m "refactor: deduplicate cost_breakdown, use evaluator_ref.evaluate_cost_components"
```

---

### Task 3: Usunięcie martwego stubu `src/solver`

> **Uwaga przed wykonaniem:** ten krok zmienia `pyproject.toml` (coverage/testpaths), co wpływa na CI/pre-commit. Potwierdź z użytkownikiem przed commitem, jeśli plan wykonywany jest automatycznie bez nadzoru.

**Files:**
- Delete: `src/solver/main.py`, `src/solver/__init__.py`, `tests/test_solver.py`
- Modify: `pyproject.toml` (`[tool.pytest.ini_options]` sekcja `addopts` i `pythonpath`)

**Interfaces:** brak — ten kod nigdzie nie jest importowany przez prawdziwy pipeline (`run_solver.py`, `xhstt_core/*`).

- [ ] **Step 1: Usuń pliki**

```bash
git rm -r src/solver tests/test_solver.py
```

- [ ] **Step 2: Popraw `pyproject.toml`**

Zmień:
```toml
addopts = "-ra -q --tb=short --cov=solver --cov-report=term-missing"
```
na:
```toml
addopts = "-ra -q --tb=short --cov=xhstt_core --cov-report=term-missing"
```
`pythonpath = ["src", "."]` → `pythonpath = ["."]` (katalog `src/` nie zawiera już nic importowalnego; `xhstt_core` leży w katalogu głównym, już objętym przez `"."`).

Jeśli po tej zmianie katalog `src/` jest pusty — usuń go (`git rm -r src`); jeśli zawiera coś jeszcze (sprawdź `git status`/`ls src`), zostaw resztę.

- [ ] **Step 3: Zweryfikuj**

```bash
uv run python -m pytest -m "not slow"
```
Oczekiwane: brak błędów importu, coverage report pokazuje `xhstt_core/*.py`, nie `solver/main.py`.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "chore: remove unrelated src/solver placeholder stub, fix coverage target"
```

---

### Task 4: Aktualizacja przestarzałej dokumentacji flow

**Files:**
- Modify: `docs/flow-ukladania-planu.md`
- Modify: `CLAUDE.md` (tylko sekcja "Bieżący stan implementacji" na końcu pliku)

**Interfaces:** brak (dokumentacja).

- [ ] **Step 1: `docs/flow-ukladania-planu.md` sekcja 4**

Zamień opis puli ruchów (obecnie: "losowo wybierany jest jeden ruch (`move_fn`) z puli `[time_reassign_move, time_swap_move, resource_reassign_move]` (`moves.py`)") na opis rzeczywistej puli: `lahc.run_lahc` losuje jedną z 8 pozycji `xhstt_core.heuristics.MANUAL_HEURISTICS` (kontrakt `apply(solution, instance, rng) -> Solution`) — wymień 8 operatorów (`move_random`, `move_best`, `swap`, `repair_hard_violation`, `resource_reassign`, `kempe_chain`, `ruin_and_recreate`, `large_perturbation`) z jednozdaniowym opisem każdego (z docstringów w `heuristics.py`).

- [ ] **Step 2: Sekcja "Czego w tym flow nie ma (jeszcze)"**

Zamień na aktualny stan:
- kontrakt heurystyk `apply(solution, instance, rng)` **już istnieje** (`xhstt_core/heuristics.py`, 8 operatorów) i jest podłączony do `lahc.py`;
- ewaluacja przyrostowa (`xhstt_core/delta.py:delta_cost`) **już istnieje**, jest przetestowana (10 000-ruchów invariant) i zbenchmarkowana, ale **nie jest jeszcze wywoływana przez `lahc.py`** — pętla LAHC nadal liczy pełny koszt (`evaluate_cost`) na każdej iteracji; podłączenie `delta_cost` do `run_lahc` to osobne zadanie (Etap 3→5 CLAUDE.md), poza zakresem tego planu;
- nadal brak: selektora RL (UCB) — wybór heurystyki to `rng.choice`, nie `argmax Q + c*sqrt(...)`; generatora LLM i sandboxa.

- [ ] **Step 3: CLAUDE.md — sekcja "Bieżący stan implementacji"**

Dopisz jedno zdanie: kontrakt `apply(solution, instance, rng)` z Etapu 4 jest już zaimplementowany (`xhstt_core/heuristics.py`, `MANUAL_HEURISTICS`) i podłączony do pętli LAHC; delta evaluation (Etap 3) istnieje w `xhstt_core/delta.py`, ale nie jest jeszcze wywoływane przez `lahc.py`.

- [ ] **Step 4: Commit**

```bash
git add docs/flow-ukladania-planu.md CLAUDE.md
git commit -m "docs: update flow doc and CLAUDE.md status to match implemented heuristics pool + delta module"
```

---

## Faza B — Podział dużych plików

### Task 5: Wydzielenie `_PAGE_CSS` do pliku `.css`

**Files:**
- Create: `xhstt_core/assets/timetable.css`
- Modify: `xhstt_core/html_report.py` (usunięcie stałej `_PAGE_CSS`, zmiana ładowania)

**Interfaces:**
- Produces: `_PAGE_CSS` pozostaje nazwą modułową w `html_report.py` (żeby `render_timetable_page` nie musiał się zmieniać poza sposobem inicjalizacji), ale teraz wczytywana z pliku zamiast literału stringa.

- [ ] **Step 1: Przenieś zawartość**

Skopiuj całą zawartość trójcudzysłowowego stringa `_PAGE_CSS = """..."""` (linie 309-645 `html_report.py`, sam CSS bez cudzysłowów/przypisania Python) do nowego pliku `xhstt_core/assets/timetable.css`.

- [ ] **Step 2: Zamień definicję w `html_report.py`**

Usuń literał `_PAGE_CSS = """...""" ` i zastąp go, obok istniejącego `_ASSETS_DIR = Path(__file__).parent / "assets"` (linia 8):
```python
_PAGE_CSS = (_ASSETS_DIR / "timetable.css").read_text(encoding="utf-8")
```
(dokładnie ten sam wzorzec, jakim `render_timetable_page` już wczytuje `fonts.css` w linii 231 — trzymaj tę definicję blisko `_ASSETS_DIR`, na górze pliku, żeby `_PAGE_CSS` zachowywało się jak stała modułowa, nie coś liczone przy każdym wywołaniu `render_timetable_page`).

- [ ] **Step 3: Zweryfikuj**

```bash
uv run python -m pytest tests/test_html_report.py -m "not slow"
uv run python run_solver.py AU-BG-98 --iterations 100 --seed 0
```
Otwórz wygenerowany `output/AU-BG-98_timetable.html` w przeglądarce i porównaj wygląd z wersją sprzed zmiany (git stash/diff jeśli trzeba) — CSS musi renderować się identycznie.

- [ ] **Step 4: Commit**

```bash
git add xhstt_core/assets/timetable.css xhstt_core/html_report.py
git commit -m "refactor: extract embedded page CSS from html_report.py into timetable.css"
```

---

### Task 6: Rozbicie `html_report.py` na pakiet

**Files:**
- Create: `xhstt_core/html_report/__init__.py`
- Create: `xhstt_core/html_report/grid.py`
- Create: `xhstt_core/html_report/evaluation.py`
- Delete: `xhstt_core/html_report.py` (po przeniesieniu zawartości)

**Interfaces:**
- Consumes (z `xhstt_core.evaluator_ref`): `Occurrence`, `evaluate_constraint` (bez zmian).
- Produces (re-eksportowane z `__init__.py`, dokładnie te same nazwy co dziś z `xhstt_core.html_report`): `render_timetable_page`, `render_evaluation_section`, `build_days`, `build_resource_grid`, `build_constraint_scores`, `DayColumn`, `TimetableCell`, `ConstraintScore` — każdy import w `run_solver.py`/testach (`from xhstt_core.html_report import render_timetable_page`) działa bez zmian.

- [ ] **Step 1: `grid.py` — siatka dnia/zasobu**

Przenieś (bez zmian w treści) z `html_report.py` do nowego `xhstt_core/html_report/grid.py`:
`DayColumn` (dataclass), `TimetableCell` (dataclass), `build_days`, `build_resource_grid`, `_render_cell_card`, `_render_resource_table`. Import na górze pliku: `from html import escape`, `from xhstt_core.evaluator_ref import Occurrence`, `from xhstt_core.model import Instance, Time`.

- [ ] **Step 2: `evaluation.py` — sekcja oceny ograniczeń**

Przenieś: `ConstraintScore` (dataclass), `build_constraint_scores`, `_render_eval_row`, `_render_eval_group`, `render_evaluation_section`. Import: `from html import escape`, `from xhstt_core.evaluator_ref import Occurrence, evaluate_constraint`, `from xhstt_core.model import Instance`.

- [ ] **Step 3: `__init__.py` — orkiestracja strony + re-eksport**

Zawiera: `_ASSETS_DIR`, `_PAGE_CSS` (z Task 5), `render_timetable_page` (bez zmian w treści — teraz woła `grid.build_days`, `grid.build_resource_grid`, `grid._render_resource_table`, `evaluation.build_constraint_scores`, `evaluation.render_evaluation_section` przez import), oraz jawny re-eksport:
```python
from xhstt_core.html_report.evaluation import (
    ConstraintScore,
    build_constraint_scores,
    render_evaluation_section,
)
from xhstt_core.html_report.grid import (
    DayColumn,
    TimetableCell,
    build_days,
    build_resource_grid,
)

__all__ = [
    "ConstraintScore",
    "DayColumn",
    "TimetableCell",
    "build_constraint_scores",
    "build_days",
    "build_resource_grid",
    "render_evaluation_section",
    "render_timetable_page",
]
```

- [ ] **Step 4: Usuń stary plik, zaktualizuj wewnętrzne importy w testach jeśli potrzeba**

```bash
git rm xhstt_core/html_report.py
```
`tests/test_html_report.py` importuje dziś prawdopodobnie `from xhstt_core.html_report import ...` albo `from xhstt_core import html_report` z dostępem do prywatnych `_render_cell_card` itp. — sprawdź `grep -n "^from xhstt_core.html_report\|^import xhstt_core.html_report" tests/test_html_report.py` i jeśli test odwołuje się do prywatnej funkcji bezpośrednio (np. `html_report._render_cell_card`), zmień import na `from xhstt_core.html_report import grid as html_report_grid` (albo analogicznie `evaluation`) w tym jednym miejscu.

- [ ] **Step 5: Zweryfikuj**

```bash
uv run mypy --follow-imports=silent xhstt_core/html_report/__init__.py xhstt_core/html_report/grid.py xhstt_core/html_report/evaluation.py
uv run ruff check xhstt_core/html_report/
uv run python -m pytest tests/test_html_report.py -m "not slow"
uv run python -m pytest -m "not slow"
uv run python run_solver.py AU-BG-98 --iterations 100 --seed 0
```
Oczekiwane: wszystkie testy przechodzą bez modyfikacji treści testów (poza ewentualną poprawką importu z kroku 4); wygenerowany HTML identyczny jak przed podziałem.

- [ ] **Step 6: Commit**

```bash
git add xhstt_core/html_report tests/test_html_report.py
git commit -m "refactor: split html_report.py into html_report/ package (grid, evaluation, page shell)"
```

---

### Task 7: Rozbicie `evaluator_ref.py` na pakiet

**Files:**
- Create: `xhstt_core/evaluator_ref/__init__.py`
- Create: `xhstt_core/evaluator_ref/_cache.py`
- Create: `xhstt_core/evaluator_ref/occurrences.py`
- Create: `xhstt_core/evaluator_ref/constraints.py`
- Delete: `xhstt_core/evaluator_ref.py` (po przeniesieniu zawartości)

**Interfaces:**
- Produces (re-eksportowane z `__init__.py`, identyczne z dzisiejszymi publicznymi i "prywatnymi, ale importowanymi gdzie indziej" nazwami — sprawdzone przez `grep -rn "from xhstt_core.evaluator_ref import" xhstt_core tests scripts`): `Occurrence`, `resolve_occurrences`, `evaluate_constraint`, `evaluate_cost_components`, `total_cost`, `INFEASIBILITY_WEIGHT`, `apply_cost_function`, `valid_start_time_ids`, `_assigned_resource_ids`, `_events_in_applies_to`, `_resources_in_applies_to`, `_build_occupancy_index`.
- **Uwaga na globalny stan:** `_current_occupancy_index` (mutowalna zmienna modułowa, dziś czytana/pisana bezpośrednio jako `evaluator_ref._current_occupancy_index` z `delta.py`) NIE może zostać zwykłym re-eksportem `from ._occurrences import _current_occupancy_index` — takie przypisanie tworzy nowe wiązanie w `__init__.py`, więc zapis `evaluator_ref._current_occupancy_index = X` z zewnątrz przestałby działać (czytelnicy w `occurrences.py`/`constraints.py` czytaliby swoją, nieaktualizowaną kopię). Rozwiązanie: Task 8 zamienia to na context manager (`occupancy_index(...)`) — wykonaj Task 8 od razu po tym zadaniu, przed commitem, żeby nie zostawić repo w stanie z ukrytym bugiem.

- [ ] **Step 1: `_cache.py` — cache statycznych danych instancji**

Przenieś: `_event_group_refs_cached`, `_INSTANCE_REGISTRY`, `_register_instance`, `_event_group_members_cached`, `_event_group_members`, `_event_group_refs`, `_shortfall_or_excess`, `_referenced_ids`, `apply_cost_function`, `_time_ids_ordered_cached`, `_time_positions_cached`, `_time_ids_ordered`, `_time_positions`, `_day_group_ids_cached`, `_day_group_ref`, `_valid_start_time_ids`, `valid_start_time_ids` (linie 1-120 i 238-260, 838-881 oryginalnego pliku). Import: `from functools import cache`, `from xhstt_core.model import Event, Instance`.

- [ ] **Step 2: `occurrences.py` — rozwiązywanie Occurrence + AppliesTo + occupancy index**

Przenieś: `Occurrence` (dataclass), `_assigned_resource`, `_assigned_resource_ids`, `resolve_occurrences`, `_events_in_applies_to_cached`, `_events_in_applies_to`, `_resources_in_applies_to_cached`, `_resources_in_applies_to`, `_occupied_time_ids`, `_build_occupancy_index`, `_full_span_busy_times` (linie 66-98, 122-260, 260-325 oryginalnego pliku). Import: `from xhstt_core.evaluator_ref._cache import _INSTANCE_REGISTRY, _register_instance, _event_group_refs`, `from xhstt_core.model import AppliesTo, Instance, Solution`. Tu też ląduje `_current_occupancy_index` i (po Task 8) `occupancy_index()`.

- [ ] **Step 3: `constraints.py` — ewaluatory ograniczeń + dispatch**

Przenieś wszystkie `_evaluate_*_constraint` (16 funkcji, linie ~201-824 oryginalnego pliku: `_evaluate_assign_time_constraint`, `_evaluate_avoid_clashes_constraint`, `_evaluate_assign_resource_constraint`, `_preferred_resource_ids`, `_evaluate_prefer_resources_constraint`, `_time_group_refs`, `_evaluate_cluster_busy_times_constraint`, `_evaluate_avoid_unavailable_times_constraint`, `_idle_count_in_group`, `_evaluate_limit_idle_times_constraint`, `_busy_count_in_group`, `_evaluate_limit_busy_times_constraint`, `_event_resource_workload`, `_evaluate_limit_workload_constraint`, `_evaluate_split_events_constraint`, `_evaluate_distribute_split_events_constraint`, `_preferred_time_ids`, `_evaluate_prefer_times_constraint`, `_evaluate_spread_events_constraint`, `_evaluate_avoid_split_assignments_constraint`, `_evaluate_link_events_constraint`, `_first_and_last_occupied_indices`, `_evaluate_order_events_constraint`), plus `_EVALUATORS` dispatch dict i `evaluate_constraint`. Import: helpery z `_cache.py` i `occurrences.py` (`Occurrence`, `_events_in_applies_to`, `_resources_in_applies_to`, `apply_cost_function`, `_day_group_ref`, itd. — dokładnie te, które dana funkcja dziś już importuje z góry pliku), `from xhstt_core.model import Constraint, Event, Instance`.

- [ ] **Step 4: `__init__.py` — fasada**

```python
from xhstt_core.evaluator_ref._cache import apply_cost_function, valid_start_time_ids
from xhstt_core.evaluator_ref.constraints import evaluate_constraint
from xhstt_core.evaluator_ref.occurrences import (
    Occurrence,
    _assigned_resource_ids,
    _build_occupancy_index,
    _events_in_applies_to,
    _resources_in_applies_to,
    occupancy_index,
    resolve_occurrences,
)
from xhstt_core.model import Instance, Solution

INFEASIBILITY_WEIGHT = 1_000_000


def evaluate_cost_components(instance: Instance, solution: Solution) -> tuple[int, int]:
    occurrences = resolve_occurrences(instance, solution)
    with occupancy_index(_build_occupancy_index(instance, occurrences)):
        infeasibility = 0
        objective = 0
        for c in instance.constraints:
            cost = evaluate_constraint(instance, occurrences, c)
            if c.required:
                infeasibility += cost
            else:
                objective += cost
        return infeasibility, objective


def total_cost(instance: Instance, solution: Solution) -> int:
    infeasibility, objective = evaluate_cost_components(instance, solution)
    return infeasibility * INFEASIBILITY_WEIGHT + objective
```
(treść `evaluate_cost_components`/`total_cost` skopiowana z oryginału, tylko `_current_occupancy_index = ...` zamienione na `with occupancy_index(...)` z Task 8 — patrz tam po dokładną definicję).

- [ ] **Step 5: Usuń stary plik**

```bash
git rm xhstt_core/evaluator_ref.py
```

- [ ] **Step 6: Zweryfikuj**

```bash
uv run mypy --follow-imports=silent xhstt_core/evaluator_ref/__init__.py xhstt_core/evaluator_ref/_cache.py xhstt_core/evaluator_ref/occurrences.py xhstt_core/evaluator_ref/constraints.py
uv run ruff check xhstt_core/evaluator_ref/
uv run python -m pytest -m "not slow"
uv run python run_solver.py AU-BG-98 --iterations 100 --seed 0
```
Oczekiwane: wszystkie testy przechodzą bez zmian treści (`tests/test_evaluator_ref.py` importuje dziś prywatne nazwy typu `_evaluate_avoid_clashes_constraint` bezpośrednio z `xhstt_core.evaluator_ref` — sprawdź `grep -n "^from xhstt_core.evaluator_ref import" tests/test_evaluator_ref.py` i dodaj brakujące nazwy do `__init__.py`'s re-export listy, zamiast zmieniać testy, żeby zachować zero zmian w plikach testowych tam gdzie to możliwe).

- [ ] **Step 7: Commit**

```bash
git add xhstt_core/evaluator_ref
git commit -m "refactor: split evaluator_ref.py into evaluator_ref/ package (cache, occurrences, constraints)"
```

---

### Task 8: Zamiana globalnego `_current_occupancy_index` na context manager

**Files:**
- Modify: `xhstt_core/evaluator_ref/occurrences.py` (dodanie `occupancy_index`)
- Modify: `xhstt_core/delta.py` (użycie context managera zamiast bezpośredniego przypisania)

**Interfaces:**
- Produces: `occupancy_index(index: OccupancyIndex | None) -> AbstractContextManager[None]` w `xhstt_core.evaluator_ref` (re-eksportowane z `__init__.py`, patrz Task 7 Step 4).
- Consumes: nic nowego z zewnątrz.

- [ ] **Step 1: Dodaj context manager w `occurrences.py`**

```python
from contextlib import contextmanager
from collections.abc import Iterator

_current_occupancy_index: dict | None = None


@contextmanager
def occupancy_index(index: dict | None) -> Iterator[None]:
    """Ustawia _current_occupancy_index na czas bloku `with`, po czym
    przywraca POPRZEDNIĄ wartość (nie zawsze None) -- poprawka względem
    starego kodu w delta.py, który w `finally` twardo ustawiał None,
    więc zagnieżdżone wywołania cicho zerowały index zewnętrznego
    wywołania. W obecnym kodzie zagnieżdżenie się nie zdarza (delta_cost
    nie jest wywoływane rekurencyjnie), ale ten kształt jest poprawny
    także wtedy, gdyby się zdarzyło."""
    global _current_occupancy_index
    previous = _current_occupancy_index
    _current_occupancy_index = index
    try:
        yield
    finally:
        _current_occupancy_index = previous
```

- [ ] **Step 2: `delta.py` — użyj context managera**

Zamień (dwa wystąpienia w `delta_cost`, oryginalne linie 120-135):
```python
evaluator_ref._current_occupancy_index = old_index
try:
    old_contribution = evaluate_constraint(instance, old_occurrences, constraint)
finally:
    evaluator_ref._current_occupancy_index = None

evaluator_ref._current_occupancy_index = new_index
try:
    new_contribution = evaluate_constraint(instance, new_occurrences, constraint)
finally:
    evaluator_ref._current_occupancy_index = None
```
na:
```python
with evaluator_ref.occupancy_index(old_index):
    old_contribution = evaluate_constraint(instance, old_occurrences, constraint)
with evaluator_ref.occupancy_index(new_index):
    new_contribution = evaluate_constraint(instance, new_occurrences, constraint)
```

- [ ] **Step 3: Zweryfikuj**

```bash
uv run python -m pytest tests/test_delta.py tests/test_cost.py tests/test_evaluator_ref.py -m "not slow"
```
Oczekiwane: identyczny wynik jak przed zmianą — `delta_cost` nadal zwraca te same liczby (context manager tylko formalizuje ten sam ciąg przypisań, z poprawką na przywracanie poprzedniej wartości zamiast twardego `None`, co przy braku zagnieżdżeń nie zmienia obserwowalnego zachowania).

- [ ] **Step 4: Commit**

```bash
git add xhstt_core/evaluator_ref/occurrences.py xhstt_core/delta.py
git commit -m "refactor: encapsulate _current_occupancy_index behind occupancy_index() context manager"
```

---

## Finalna weryfikacja całego planu

- [ ] **Step 1: Pełny zestaw testów, włącznie z wolnymi**

```bash
uv run python -m pytest
```
Oczekiwane: wszystkie testy przechodzą, w tym `tests/test_delta.py`'s 10 000-ruchów invariant (`-m slow`).

- [ ] **Step 2: Statyczna analiza całego `xhstt_core/`**

```bash
uv run mypy xhstt_core/
uv run ruff check xhstt_core/
uv run ruff format xhstt_core/ --check
uv run pre-commit run --all-files
```

- [ ] **Step 3: Ręczny przebieg end-to-end**

```bash
uv run python run_solver.py AU-BG-98 --iterations 5000 --seed 0
```
Porównaj `infeasibility`/`objective` na wyjściu z wynikiem sprzed rozpoczęcia planu (ten sam seed → identyczne liczby, bo żadne zadanie nie zmienia logiki solvera).

## Self-Review (wykonane podczas pisania planu)

- **Pokrycie:** wszystkie 7 znalezisk z tabeli mają odpowiadające zadanie (1↔Task1, 2↔Task2, 3↔Task3, 4↔Task4, 6↔Task5, 7↔Task4; 5 celowo NIE dostaje zadania — zostaje bez zmian, decyzja uzasadniona w tabeli).
- **Placeholders:** brak "TODO"/"podobnie jak wyżej" — każde zadanie ma jawną listę nazw funkcji do przeniesienia (z konkretnych numerów linii zebranych podczas analizy) zamiast odsyłacza do innego zadania.
- **Spójność nazw:** `occupancy_index()` (Task 8) używane identycznie w Task 7 Step 4 (`__init__.py`) i Task 8 Step 2 (`delta.py`) — ta sama sygnatura wszędzie.

---

## Uwaga poza zakresem tego planu

`delta_cost` (Task 4, znalezisko #4) jest gotowe, przetestowane i zbenchmarkowane, ale nie jest wywoływane przez `lahc.run_lahc`. Podłączenie go do pętli LAHC (żeby faktycznie przyspieszyć solver, nie tylko istnieć obok) to zmiana **zachowania/wydajności**, nie refaktoring — świadomie zostawiona poza tym planem. Jeśli chcesz to zrobić jako kolejny krok, to osobny plan (nowy branch `issue#<n>-wire-delta-into-lahc`), bo wymaga decyzji: `Heuristic.apply` dziś nie zwraca informacji "co się zmieniło" w formie, jakiej `delta_cost` oczekuje (rekonstruuje to z różnicy `old_solution`/`new_solution` przez pełne `resolve_occurrences` obu — do realnego przyspieszenia trzeba by rozważyć, czy to wystarczy, czy heurystyki powinny zwracać też listę zmienionych indeksów).
