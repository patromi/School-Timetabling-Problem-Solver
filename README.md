# LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem

## Development

Nad zmianami pracujemy na swoich branchach. Branch nazywamy według schematu:

```
issue#<issue>
```

> [!TIP]
> **Zadania w VS Code (Tasks)**:
> Jeśli korzystasz z VS Code, wszystkie opisane poniżej komendy (instalacja środowiska, uruchamianie solvera, testy, lintery, DVC) możesz uruchamiać wygodnie z poziomu edytora.
> Aby to zrobić, otwórz menu `Terminal` -> `Run Task...` (lub użyj skrótu klawiszowego `Ctrl+Shift+B` / `Cmd+Shift+B`) i wybierz odpowiednie zadanie z listy.

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

W repozytorium skonfigurowane są pre-commit hooki, które automatycznie sprawdzają poprawność kodu (Ruff, Mypy) oraz nazwę brancha i format wiadomości commitów.

* **Instalacja hooków w Git (wymagane raz na start)**:
  ```bash
  uv run pre-commit install --hook-type pre-commit --hook-type commit-msg
  ```

* **Format wiadomości commitów**:
  Każda wiadomość commita musi zaczynać się od identyfikatora issue w nawiasach kwadratowych, np.:
  ```
  [issue#12]: Dodanie walidatora commitow
  ```

* **Ręczne uruchomienie hooków na wszystkich plikach**:
  ```bash
  uv run pre-commit run --all-files
  ```

### Zarządzanie danymi (DVC)

Duże pliki z danymi (np. w folderze `data/raw/`) są wersjonowane za pomocą narzędzia DVC i przechowywane na Google Drive.

* **Pobranie aktualnych danych na start**:
  1. Skonfiguruj klucze uwierzytelniające lokalnie (uzyskaj `gdrive_client_id` oraz `gdrive_client_secret` od autora projektu i wklej poniżej):
     ```bash
     uv run dvc remote modify franekremote --local gdrive_client_id "TWÓJ_CLIENT_ID"
     uv run dvc remote modify franekremote --local gdrive_client_secret "TWÓJ_CLIENT_SECRET"
     ```
  2. Pobierz pliki poleceniem:
     ```bash
     uv run dvc pull
     ```
     *(Uwaga: Zostaniesz poproszony o jednorazowe zalogowanie się w przeglądarce do konta Google).*

* **Aktualizacja / dodanie nowych danych**:
  1. Dodaj lub zmień pliki w folderze danych (np. `data/raw/`).
  2. Zaktualizuj śledzenie w DVC:
     ```bash
     uv run dvc add data/raw
     ```
  3. Wyślij dane na Google Drive:
     ```bash
     uv run dvc push
     ```
  4. Skomituj plik `data/raw.dvc` oraz pliki `.gitignore` wygenerowane przez DVC do repozytorium Git.
