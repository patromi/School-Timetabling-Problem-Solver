# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projekt: LLM-Driven RL Hyper-Heuristic for the School Timetabling Problem

### Kontekst

To jest praca inżynierska. Budujemy system rozwiązujący problem układania planów lekcji (school timetabling) na benchmarku XHSTT, oparty na selekcyjnej hiper-heurystyce sterowanej przez RL, w której LLM generuje nowe heurystyki niskiego poziomu rozszerzające pulę operatorów.

Priorytety projektu (w tej kolejności): poprawność > powtarzalność eksperymentów > czytelność kodu > wydajność > liczba funkcji. Kod będzie opisywany w pracy dyplomowej i pokazywany na obronie — ma być prosty do wytłumaczenia.

### Problem

- Benchmark: XHSTT (archiwum XHSTT-2014, instancje XML ze strony projektu HSTT Uniwersytetu Twente).
- Instancja = Times (sloty czasowe pogrupowane w dni), Resources (nauczyciele, klasy, sale), Events (lekcje z czasem trwania i przypisanymi zasobami), Constraints (~16 typów, twarde = Required i miękkie).
- **Ograniczenie zakresu (celowe):** pracujemy wyłącznie na instancjach, w których podział zdarzeń jest ustalony, a zasoby preprzypisane. Jedyna decyzja to przypisanie czasu startu każdemu zdarzeniu. Rozwiązanie = tablica `event_id -> start_time`.
- Koszt rozwiązania to para (infeasibility, objective) porównywana leksykograficznie: suma kar za ograniczenia twarde, potem miękkie. Wagi i funkcje kosztu (Linear/Quadratic/Step) są zdefiniowane w XML-u instancji — implementujemy specyfikację, nie projektujemy własnej oceny.
- Dla selektora RL koszt spłaszczamy do skalaru: `HARD_MULTIPLIER * infeasibility + objective`, ale w logach i raportach zawsze trzymamy parę osobno.

### Architektura (decyzje już podjęte — nie zmieniaj ich bez pytania)

1. **Solver** — pętla lokalnego przeszukiwania. Heurystyka proponuje kandydata, solver decyduje o akceptacji (reguła symulowanego wyżarzania). Akceptacja jest oddzielona od selekcji i od heurystyk.
2. **Pula heurystyk** — każda heurystyka (ręczna i wygenerowana przez LLM) implementuje identyczny kontrakt:
   `apply(solution: Solution, instance: Instance, rng: random.Random) -> Solution`
   Heurystyka zwraca kandydata; nigdy nie modyfikuje wejścia in-place; nie podejmuje decyzji o akceptacji.
3. **Selektor RL** — kontekstowy UCB na niezależnych statystykach per heurystyka:
   - stan dyskretny: `(naruszenia_twarde: {0,1}, stagnacja: {niska, srednia, wysoka})` — 6 stanów,
   - statystyki: słownik `(stan, h_id) -> [Q, n]`,
   - wybór: `argmax Q + c*sqrt(ln N / n)`, dla `n == 0` priorytet nieskończony,
   - aktualizacja Q: średnia wykładnicza `Q <- (1-alpha)*Q + alpha*r`, alpha ~0.08,
   - nagroda: `r = max(0, (koszt_przed - koszt_po)/koszt_przed)` + bonus za pobicie najlepszego globalnego rozwiązania,
   - optymistyczna inicjalizacja Q dla nowych heurystyk, okres ochronny min. 100 wywołań przed usuwaniem,
   - usuwanie: tylko gdy pula > limit (20), heurystyka ma n > 100 i najgorsze Q; heurystyki ręczne mają flagę `protected` i nie są usuwane.
   - ŻADNEGO deep RL (sieci neuronowe) — świadoma decyzja projektowa.
4. **Generator LLM** — wyzwalany stagnacją (brak poprawy najlepszego rozwiązania przez K iteracji). Dostaje diagnostykę (NIE cały plan): rozkład naruszeń per typ ograniczenia, ranking skuteczności heurystyk w puli, opis stagnacji, sygnatury istniejących heurystyk. Generuje kod Pythona nowej heurystyki zgodnej z kontraktem. Kilku kandydatów na wywołanie (np. 5).
5. **Sandbox i walidacja 4-stopniowa** kodu od LLM-a:
   - (1) parsowanie + analiza statyczna (whitelist importów, zakaz io/os/sys/eval/exec),
   - (2) wykonanie w osobnym procesie z twardym timeoutem,
   - (3) walidacja semantyczna wyniku (struktura rozwiązania poprawna, żadne zdarzenie nie zgubione/zduplikowane),
   - (4) test skuteczności: N uruchomień na instancjach testowych, statystyczna poprawa względem stanu wejściowego.
   Heurystyka wchodzi do puli tylko po przejściu wszystkich bramek i tylko w punkcie synchronizacji (stagnacja/restart), nigdy w środku epizodu. Logujemy acceptance rate.
6. **Ewaluacja przyrostowa (delta)** dla ruchów z puli podstawowej; po heurystykach od LLM-a pełna ewaluacja. Inwariant testowany w pytest: po dowolnej sekwencji ruchów koszt liczony przyrostowo == koszt liczony od zera.
7. **Weryfikacja zewnętrzna:** eksport rozwiązań do XML XHSTT i sprawdzanie ewaluatorem HSEval. Zgodność kosztów z HSEval to warunek zaliczenia etapu ewaluatora.

### Etapy (realizuj po kolei; nie zaczynaj następnego przed spełnieniem definition of done)

#### Etap 1 — Parser i model danych
Parser XML (lxml/ElementTree) dla wybranych instancji XHSTT. Klasy `Instance`, `Solution`. Po parsowaniu wszystko przeliczone na indeksy int (czasy 0..T-1, zasoby 0..R-1) + struktury pomocnicze (zdarzenia per zasób, mapowania grup).
**DoD:** wczytuje min. 3 instancje z archiwum bez błędów; testy jednostkowe na liczbach zdarzeń/zasobów/czasów zgodnych z opisem instancji.

#### Etap 2 — Pełny ewaluator + eksport + HSEval
Implementacja funkcji kosztu dla typów ograniczeń występujących w wybranych instancjach (nie wszystkich 16 — tylko potrzebnych). Eksport rozwiązania do XML.
**DoD:** dla kilku rozwiązań (w tym losowych i najlepszych znanych z archiwum) koszt naszego ewaluatora == koszt HSEval.

#### Etap 3 — Delta evaluation
Przyrostowe przeliczanie kosztu przy przesunięciu jednego zdarzenia.
**DoD:** test pytest: 10 000 losowych ruchów, po każdym asercja delta == pełna ewaluacja; benchmark pokazujący przyspieszenie.

#### Etap 4 — Pula heurystyk ręcznych
6–10 operatorów zgodnych z kontraktem: move (jedno zdarzenie na losowy/najlepszy slot), swap (zamiana czasów dwóch zdarzeń), Kempe chain, ruin-and-recreate (mała porcja), operator naprawczy naruszeń twardych, perturbacja duża.
**DoD:** każda heurystyka ma test: zwraca strukturalnie poprawne rozwiązanie, nie modyfikuje wejścia.

#### Etap 5 — Pętla solvera
Symulowane wyżarzanie jako mechanizm akceptacji + losowa selekcja heurystyk (baseline). Śledzenie najlepszego rozwiązania, sygnał stagnacji, logowanie (koszt w czasie, użycia heurystyk).
**DoD:** na małej instancji solver redukuje naruszenia twarde do 0 i poprawia miękkie; przebieg powtarzalny przy ustalonym seedzie.

#### Etap 6 — Selektor UCB
Implementacja selektora wg specyfikacji powyżej (klasa `Selector`: `add_heuristic`, `select`, `update`), definicja stanu, progi stagnacji.
**DoD:** eksperyment porównawczy losowa selekcja vs epsilon-greedy vs UCB na min. 3 instancjach, 10+ seedów, wykres udziału wywołań per heurystyka w czasie.

#### Etap 7 — Integracja LLM
Budowa promptu diagnostycznego, klient API (z cache odpowiedzi na dysku!), sandbox (osobny proces + timeout + whitelist importów), pipeline walidacji 4-stopniowej, wejście heurystyk do puli z optymistyczną inicjalizacją.
**DoD:** pełny przebieg end-to-end: stagnacja -> generacja -> walidacja -> min. 1 heurystyka zaakceptowana i używana przez selektor; logi acceptance rate i pochodzenia heurystyk.

#### Etap 8 — Framework eksperymentów
Skrypt uruchamiający macierz konfiguracji: {losowy, epsilon-greedy, UCB} x {pula statyczna, LLM-offline, LLM-online} x instancje x seedy. Równe budżety (limit ewaluacji lub czasu; czas LLM raportowany osobno). Wyniki do CSV, wykresy (matplotlib), test Wilcoxona dla porównań par konfiguracji.
**DoD:** jedna komenda odtwarza wszystkie wyniki i wykresy do rozdziału eksperymentalnego.

### Zasady techniczne

- Python 3.11+, standardowa struktura pakietu (`src/` lub płaski pakiet), `pytest`, type hints wszędzie, docstringi po polsku lub angielsku — konsekwentnie.
- Zależności minimalne: lxml, numpy, matplotlib, pytest; klient API LLM. Bez ciężkich frameworków RL.
- Wszędzie jawny `random.Random(seed)` przekazywany przez parametry — nigdy globalny random. Każdy eksperyment w pełni odtwarzalny z seeda i pliku konfiguracyjnego.
- Logowanie do plików (JSON lines): każdy ruch NIE musi być logowany, ale każda zmiana najlepszego rozwiązania, każda decyzja selektora w agregatach okienkowych, każde wywołanie LLM (prompt, odpowiedź, wynik walidacji) — tak.
- Kod generowany przez LLM zapisujemy na dysk (katalog `generated/`) z metadanymi (kiedy, z jakiej diagnostyki, wynik walidacji) — to materiał do rozdziału pracy.
- Commity per etap, README z instrukcją odtworzenia eksperymentów.

### Jak pracować

Pracuj etapami. Przed rozpoczęciem etapu przedstaw krótki plan plików i interfejsów do akceptacji. Po zakończeniu etapu uruchom testy i pokaż wyniki DoD. Jeśli coś w specyfikacji jest niejednoznaczne albo widzisz lepsze rozwiązanie sprzeczne z decyzjami powyżej — zapytaj, zanim zmienisz.

## Bieżący stan implementacji (repo)

Obecnie zaimplementowany pipeline to construct + Late Acceptance Hill Climbing (LAHC), czyli **etap
przejściowy sprzed powyższej specyfikacji** — nie ma jeszcze selektora RL, generatora LLM, delta evaluation
ani formalnego kontraktu heurystyk `apply(solution, instance, rng)`; akceptacja kandydatów działa wg reguły
LAHC (Burke & Bykov), nie symulowanego wyżarzania z Etapu 5 powyżej. Sekcje poniżej opisują *ten* istniejący
kod, żeby móc się w nim poruszać — traktuj powyższą specyfikację etapów jako docelową mapę drogową do realizacji,
nie jako opis obecnego stanu.

## Commands

Dependency/environment management is via `uv`; Python 3.12 is pinned (`.python-version`).

```bash
uv sync                                    # install/sync dependencies
uv run python run_solver.py --list         # list instances in the default XHSTT archive
uv run python run_solver.py AU-BG-98       # solve one instance (writes output/<id>_solution.xml + _timetable.html)
uv run python run_solver.py BR-SA-00 --iterations 50000 --seed 1 --history 30
uv run snakemake --cores all               # auto-detects data/raw/*.xml, runs the solver on each, writes data/results/summary.{csv,md}

uv run python -m pytest                              # full test suite, with coverage
uv run python -m pytest -m "not slow"          # skip long-running DoD tests (~9 min 10k-move invariant test)
uv run python -m pytest tests/test_evaluator_ref.py  # single test file
uv run python -m pytest tests/test_evaluator_ref.py::test_name -v   # single test

uv run mypy src/                           # type check (strict mode)
uv run ruff check src/                     # lint
uv run ruff format src/                    # format
uv run pre-commit run --all-files          # run all pre-commit hooks manually
```

Pre-commit hooks (`.pre-commit-config.yaml`) run ruff (fix + format), mypy, and a branch-name check
(`scripts/check_branch_name.py`) on every commit. **Branches must be named `issue#<number>[-description]`**
(e.g. `issue#45-add-solver`); `main`/`master`/`develop`/`release` are exempt.

Note: use `uv run python -m pytest`, not bare `uv run pytest` — `pyproject.toml` has no `[build-system]`
table, so `uv run pytest`'s console-script entry point never gets the repo root on `sys.path`, and importing
`src` (the package directory at the repo root) fails with `ModuleNotFoundError`. `python -m
pytest` adds the current directory to `sys.path`, which fixes it. Same reasoning applies to `mypy`: plain
`uv run mypy src/<file>.py` follows imports into the rest of `src` and surfaces ~37 pre-existing
`mypy --strict` errors unrelated to whatever you're checking — pass `--follow-imports=silent` to scope the
report to just the file you're checking.

## Architecture

**`src/` is the actual solver engine** — a flat package directory at the repo root (not a `src/<pkg>/`
layout; `import src.model` etc.). The real CLI entry point is `run_solver.py` at the repo root.

Pipeline (see `run_solver.py`):

```
parser.parse_archive(xml)  -> list[Instance]
construct.build_initial(instance, rng)  -> Solution           (naive, constraint-blind, structurally valid)
lahc.run_lahc(instance, initial, rng, moves=...)  -> (best Solution, cost)
    - each step: pick a move from moves.py, apply it, score with evaluator_ref.total_cost
xml_writer.render_archive_with_solution_groups(...)  -> XHSTT XML  (for submission/HSEval validation)
html_report.render_timetable_page(...)  -> human-readable HTML timetable + per-constraint cost breakdown
```

Module responsibilities inside `src/`:
- `model.py` — plain dataclasses for the XHSTT object model (`Instance`, `Event`, `Constraint`, `Solution`, ...).
  `Constraint` keeps type-specific parameters in a generic `params: dict` rather than per-type fields, since
  XHSTT defines 16 constraint types and explicitly allows extension.
- `parser.py` — parses XHSTT archive XML (`<HighSchoolTimetableArchive>`) into `Instance`/`SolutionGroup` objects.
- `construct.py` — greedy initial-solution builder; fills every unassigned time/resource slot arbitrarily but
  structurally validly (respects `SplitEventsConstraint` bounds, resource types, non-overflowing time spans).
- `moves.py` — local-search moves (`time_reassign_move`, `time_swap_move`, `resource_reassign_move`) used by
  LAHC. Each returns a *new* `Solution` built via `dataclasses.replace` + structural sharing rather than a deep
  copy — deep-copying the whole solution was profiled as a major hot spot on large instances.
- `lahc.py` — Late Acceptance Hill Climbing search loop (Burke & Bykov).
- `evaluator_ref.py` — the reference cost evaluator: one `_evaluate_*_constraint` function per XHSTT constraint
  type, dispatched by `evaluate_constraint`; `total_cost` combines them lexicographically
  (`infeasibility * 1_000_000 + objective`, so any Required-constraint violation dominates all objective cost).
  Heavily uses `functools.lru_cache` keyed off `id(instance)` (via an in-process `_INSTANCE_REGISTRY`) to avoid
  recomputing static per-instance data (event group membership, valid start times, etc.) on every evaluation —
  these functions get called hundreds of thousands of times per LAHC run.
- `xml_writer.py` — renders `Solution`/`SolutionGroup` back into XHSTT-compliant XML, and can slice a single
  `<Instance>` out of a multi-instance archive for standalone submission.
- `html_report.py` — renders a human-readable timetable grid + constraint scoring page from a solved instance.

Other top-level paths:
- `data/xhstt2014/XHSTT-2014.xml` — the real 25-instance XHSTT archive used as `run_solver.py`'s default input.
- `data/raw/` — individual instance XML files auto-detected by the `Snakefile` (`glob_wildcards`); `data/processed/`
  and `data/results/` are its output directories (`data/results/summary.csv`/`summary.md` are committed as a
  checked-in example of that output).
- `archive/itc2019_parser/` — legacy parser for the older ITC-2019 format; superseded by `src/parser.py`
  and kept only for reference.

## Conventions

- User-facing CLI output (`run_solver.py`) and some docstrings/comments are in Polish; keep new user-facing
  CLI text in Polish for consistency. Code identifiers stay in English.
- `mypy` runs in `strict` mode; `ruff` has `ANN` (annotation coverage) enabled — new code needs full type hints.
