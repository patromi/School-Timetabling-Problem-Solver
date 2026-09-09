# LLM-Driven RL Hyper-Heuristic for School Timetabling

System rozwiązujący problem układania planów lekcji (XHSTT benchmark) oparty na selekcyjnej hiper-heurystyce sterowanej przez uczenie ze wzmocnieniem (RL). LLM generuje nowe heurystyki niskiego poziomu rozszerzające pulę operatorów w czasie działania solvera.

Projekt inżynierski (praca dyplomowa).

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
run_solver.py              — punkt wejścia CLI (LAHC na jednej instancji)
Snakefile                  — potok: solver na data/raw/*.xml → data/results/summary.{csv,md}
pyproject.toml             — konfiguracja projektu: pytest, mypy, ruff, zależności (uv)

src/                       — pakiet domenowy (flat layout; import src.model itd.)
  model.py                 — dataclassy XHSTT: Instance, Event, Constraint, Solution, …
  parser.py                — parser XML archiwum XHSTT → list[Instance]
  construct.py             — zachłanny konstruktor rozwiązania początkowego
  evaluator_ref/           — ewaluator kosztu (referencyjny, pełna ewaluacja)
    _cache.py              — @cache dla statycznych danych per instancja (grupy, czasy, valid starts)
    occurrences.py         — Occurrence, resolve_occurrences, occupancy_index
    constraints.py         — _evaluate_*_constraint dla typów XHSTT + dispatch
    __init__.py            — fasada: evaluate_cost_components, total_cost, re-eksporty
  cost.py                  — Cost(infeasibility, objective) z porównaniem leksykograficznym
  delta.py                 — delta_cost: przyrostowe przeliczanie kosztu (gotowe, niezapodłączone do LAHC)
  moves.py                 — ruchy lokalnego przeszukiwania używane przez LAHC
  heuristics.py            — MANUAL_HEURISTICS: 8 operatorów, kontrakt apply(solution, instance, rng)
  lahc.py                  — pętla LAHC (Late Acceptance Hill Climbing, Burke & Bykov)
  xml_writer.py            — eksport Solution → XML XHSTT (format dla HSEval)
  html_report.py           — raport HTML: siatka timetable + breakdown kosztów per ograniczenie
  assets/                  — zasoby statyczne html_report (fonts.css)

data/
  xhstt2014/               — archiwum XHSTT-2014 (25 instancji, Git LFS)
  raw/                     — pojedyncze pliki instancji XML wykrywane przez Snakefile
  results/                 — wyniki Snakemake: summary.csv, summary.md

tests/
  fixtures/                — małe instancje XML: BrazilInstance1, ArtificialSudoku4x4
  test_construct.py        — testy konstruktora rozwiązania początkowego
  test_cost.py             — testy modelu kosztu (Cost, porównanie leksykograficzne)
  test_delta.py            — test delta: 10 000 ruchów, asercja delta == pełna ewaluacja (@slow)
  test_evaluator_ref.py    — testy ewaluatora kosztu (zgodność z HSEval)
  test_heuristics.py       — testy heurystyk (strukturalna poprawność, brak mutacji wejścia)
  test_html_report.py      — testy generowania raportu HTML
  test_lahc.py             — testy pętli LAHC (powtarzalność z seeda)
  test_moves.py            — testy ruchów lokalnego przeszukiwania
  test_parser.py           — testy parsera XHSTT
  test_xml_writer.py       — testy eksportu do XML

scripts/
  generate_summary.py      — generuje summary.csv/md z wynikami solvera (wywoływany przez Snakefile)
  benchmark_delta_evaluation.py — benchmark delta vs. pełna ewaluacja
  check_branch_name.py     — hook pre-commit: walidacja nazwy brancha (issue#N-opis)
  check_commit_msg.py      — hook commit-msg: walidacja Conventional Commits

docs/
  flow-ukladania-planu.md  — opis pipeline'u parsowanie→solver→eksport (dla pracy dyplomowej)
  superpowers/             — specyfikacje i plany implementacyjne poszczególnych etapów

archive/                   — stary parser ITC-2019, zachowany wyłącznie jako odniesienie historyczne
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
