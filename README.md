# LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem

## Development

Nad zmianami pracujemy na swoich branchach. Branch nazywamy według schematu:

```
issue#<issue>
```

> [!TIP]
> **Zadania w VS Code (Tasks)**:
> Jeśli korzystasz z VS Code, wszystkie opisane poniżej komendy (instalacja środowiska, uruchamianie solvera, testy, lintery) możesz uruchamiać wygodnie z poziomu edytora.
> Aby to zrobić, otwórz menu `Terminal` -> `Run Task...` (lub użyj skrótu klawiszowego `Ctrl+Shift+B` / `Cmd+Shift+B`) i wybierz odpowiednie zadanie z listy.

Do zarządzania zależnościami i środowiskiem w Pythonie używamy narzędzia `uv`.

### Uruchomienie i synchronizacja projektu

* **Synchronizacja środowiska (instalacja zależności)**:
  ```bash
  uv sync
  ```
* **Uruchomienie solvera**:
  ```bash
  uv run python run_solver.py --list              # wypisz dostępne instancje
  uv run python run_solver.py AU-BG-98            # rozwiąż instancję (domyślne archiwum)
  uv run python run_solver.py AU-BG-98 --iterations 50000 --seed 1
  ```
* **Uruchomienie przepływu (workflow) za pomocą Snakemake**:
  Potok automatycznie wykrywa wszystkie pliki instancji `.xml` w katalogu `data/raw/` (z wyłączeniem pustych plików i szablonu `instance.xml`), uruchamia dla nich solver równolegle, a na koniec generuje zbiorcze raporty podsumowujące koszty rozwiązań (`summary.csv` oraz `summary.md` w `data/results/`).

  Uruchomienie z domyślnymi parametrami (30 000 iteracji, seed=0):
  ```bash
  uv run snakemake --cores all
  ```

  Konfiguracja parametrów obliczeń za pomocą zmiennych środowiskowych (np. szybki test na 500 iteracji):
  * **Windows (PowerShell)**:
    ```powershell
    $env:SOLVER_ITERATIONS="500"; uv run snakemake --cores all
    ```
  * **Linux / macOS**:
    ```bash
    SOLVER_ITERATIONS=500 uv run snakemake --cores all
    ```

### Testowanie i jakość kodu

Przed wykonaniem commita lub pusha warto sprawdzić kod za pomocą poniższych poleceń:

* **Uruchomienie testów jednostkowych (pytest) z pokryciem**:
  ```bash
  uv run python -m pytest                   # pełny zestaw
  uv run python -m pytest -m "not slow"     # pomiń długie testy (~9 min)
  ```
  > **Uwaga:** używaj `python -m pytest`, nie `uv run pytest`. Brak sekcji `[build-system]` w `pyproject.toml` sprawia, że skrót `uv run pytest` nie dodaje katalogu głównego repo do `sys.path`, przez co import `src` się wywala.
* **Sprawdzenie typowania (mypy)**:
  ```bash
  uv run mypy src/
  ```
* **Linter (ruff check)**:
  ```bash
  uv run ruff check src/
  ```
* **Automatyczne formatowanie kodu (ruff format)**:
  ```bash
  uv run ruff format src/
  ```

### Pre-commit hooks

W repozytorium skonfigurowane są pre-commit hooki, które automatycznie sprawdzają poprawność kodu (Ruff, Mypy) oraz nazwę brancha i format wiadomości commitów.

* **Instalacja hooków w Git (wymagane raz na start)**:
  ```bash
  uv run pre-commit install --hook-type pre-commit --hook-type commit-msg
  ```

* **Format wiadomości commitów**:
  Stosujemy [Conventional Commits](https://www.conventionalcommits.org/), np.:
  ```
  feat: add UCB selector
  fix: correct deviation formula for LimitIdleTimesConstraint
  docs: update flow diagram
  refactor: split evaluator_ref.py into package
  chore: remove dead config.yaml
  ```

* **Ręczne uruchomienie hooków na wszystkich plikach**:
  ```bash
  uv run pre-commit run --all-files
  ```

### Struktura projektu

```
run_solver.py          — punkt wejścia CLI (solver LAHC na jednej instancji z archiwum XHSTT)
src/                   — cała logika domenowa (flat package, import src.model itd.)
  model.py             — dataclassy XHSTT: Instance, Event, Constraint, Solution, ...
  parser.py            — parser XML archiwum XHSTT → list[Instance]
  construct.py         — zachłanny konstruktor rozwiązania początkowego
  evaluator_ref/       — ewaluator kosztu (referencyjny, pełna ewaluacja)
    _cache.py          — cache statycznych danych per instancja (grupy, czasy, valid starts)
    occurrences.py     — Occurrence, resolve_occurrences, occupancy index
    constraints.py     — _evaluate_*_constraint dla ~16 typów XHSTT + dispatch
    __init__.py        — fasada: evaluate_cost_components, total_cost, re-eksporty
  cost.py              — Cost(infeasibility, objective) z porównaniem leksykograficznym
  delta.py             — przyrostowa ewaluacja kosztu (Etap 3; gotowa, niezapodłączona do LAHC)
  moves.py             — ruchy lokalnego przeszukiwania (time_reassign, swap, kempe_chain, ...)
  heuristics.py        — MANUAL_HEURISTICS: 8 operatorów o kontrakcie apply(solution, instance, rng)
  lahc.py              — pętla LAHC (Late Acceptance Hill Climbing, Burke & Bykov)
  xml_writer.py        — eksport Solution → XML XHSTT (do HSEval)
  html_report.py       — raport HTML: siatka timetable + ocena ograniczeń
data/xhstt2014/        — archiwum XHSTT-2014 (25 instancji, Git LFS)
data/raw/              — pojedyncze pliki instancji wykrywane przez Snakefile
data/results/          — wyniki Snakemake (summary.csv / summary.md, commitowane jako przykład)
tests/                 — testy pytest + tests/fixtures/ (małe instancje XML)
scripts/               — skrypty pomocnicze (generate_summary.py, benchmarki, hooki pre-commit)
docs/superpowers/      — specyfikacje i plany implementacyjne poszczególnych etapów
archive/               — stary parser ITC-2019, zachowany wyłącznie jako odniesienie historyczne
```

### Zarządzanie danymi (Git LFS)

Duże pliki z danymi (np. w folderze `data/`) są wersjonowane za pomocą narzędzia **Git LFS** (Large File Storage).

* **Instalacja Git LFS**:
  Przed rozpoczęciem pracy upewnij się, że masz zainstalowany Git LFS na swoim systemie (np. na Windows: `winget install github.gitlfs` lub pobierz instalator ze strony https://git-lfs.github.com/). Następnie zainicjalizuj go jednorazowo w swoim systemie komendą:
  ```bash
  git lfs install
  ```

* **Pobranie i wysyłanie danych**:
  Pliki śledzone przez Git LFS są pobierane i wysyłane automatycznie przy standardowych poleceniach Gita:
  ```bash
  git pull
  git push
  ```

* **Dodanie nowych dużych plików**:
  Wszelkie nowe pliki umieszczane w folderze `data/` zostaną automatycznie objęte wersjonowaniem LFS dzięki regułom zdefiniowanym w pliku `.gitattributes`. Wystarczy dodać je do commita w zwykły sposób:
  ```bash
  git add data/
  git commit -m "chore: add new data files"
  git push
  ```
