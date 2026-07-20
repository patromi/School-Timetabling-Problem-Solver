# LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem

## Development

Nad zmianami pracujemy na swoich branchach. Branch nazywamy według schematu:

```
issue#<issue>
```

Do zarządzania zależnościami i środowiskiem w Pythonie używamy narzędzia `uv`.

### Uruchomienie i synchronizacja projektu

* **Synchronizacja środowiska (instalacja zależności)**:
  ```bash
  uv sync
  ```
* **Uruchomienie solvera (szablon)**:
  ```bash
  uv run python src/solver/main.py [ścieżka_do_pliku_xml]
  ```
* **Uruchomienie przepływu (workflow) za pomocą Snakemake**:
  ```bash
  uv run snakemake --cores all
  ```

### Testowanie i jakość kodu

Przed wykonaniem commita lub pusha warto sprawdzić kod za pomocą poniższych poleceń:

* **Uruchomienie testów jednostkowych (pytest) z pokryciem**:
  ```bash
  uv run pytest
  ```
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

W repozytorium skonfigurowane są pre-commit hooki, które automatycznie sprawdzają poprawność kodu przy każdym commicie.

* **Ręczne uruchomienie hooków na wszystkich plikach**:
  ```bash
  uv run pre-commit run --all-files
  ```
