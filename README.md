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
  git commit -m "[issue#...]: Dodanie nowych plików danych"
  git push
  ```
