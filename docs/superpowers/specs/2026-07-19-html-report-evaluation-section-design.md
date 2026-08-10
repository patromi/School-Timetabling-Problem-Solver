# Sekcja "Ocena rozwiązania" w raporcie HTML planu zajęć

## Kontekst

`xhstt_core/html_report.py` (`render_timetable_page`) generuje samodzielną
stronę HTML z wizualnym planem zajęć (siatka dzień×godzina per zasób).
Nagłówek strony pokazuje już zbiorcze liczby `infeasibility`/`objective`
(przekazywane przez wywołującego, np. `run_solver.py`), ale nie pokazuje
*skąd* się bierze ten koszt — nie widać, które z ograniczeń instancji są
naruszone ani ile każde z nich kosztuje.

Cel: dodać na końcu strony sekcję "Ocena rozwiązania" z pełną tabelą
per-ograniczenie, tak żeby użytkownik mógł zobaczyć rozkład kosztu bez
sięgania po `run_solver.py`'s `cost_breakdown` ani po surowy XML.

## Zakres

W zakresie:
- Nowa funkcja licząca koszt per ograniczenie instancji na podstawie już
  rozwiązanych `occurrences`.
- Nowa sekcja HTML na dole strony (dwie podtabele: ograniczenia wymagane /
  preferowane), z domyślnym filtrem "tylko naruszone" i przełącznikiem do
  pokazania wszystkich.
- Testy TDD rozszerzające `tests/test_html_report.py`.

Poza zakresem:
- Zmiana sygnatury `render_timetable_page` lub wywołań w `run_solver.py`
  (dane wejściowe — `instance`/`occurrences` — już wystarczają).
- Jakikolwiek nowy system kolorów/typografii — reużycie istniejących
  custom properties (`--ok`, `--bad`, `--surface`, `--line`, `--accent`)
  i czcionek (`IBM Plex Sans`/`IBM Plex Mono`) z `_PAGE_CSS`.
- Zmiana logiki `evaluate_constraint`/`total_cost` w `evaluator_ref.py` —
  ta sekcja tylko *woła* istniejącą funkcję, nie zmienia formuł.

## Warstwa danych

Nowy dataclass w `xhstt_core/html_report.py`:

```python
@dataclass
class ConstraintScore:
    id: str
    name: str
    type: str
    required: bool
    weight: int
    cost_function: str
    cost: int
```

Nowa funkcja:

```python
def build_constraint_scores(
    instance: Instance, occurrences: list[Occurrence]
) -> list[ConstraintScore]:
```

Dla każdego `c in instance.constraints` woła
`evaluate_constraint(instance, occurrences, c)` (import doda się do już
istniejącego `from xhstt_core.evaluator_ref import Occurrence` →
`Occurrence, evaluate_constraint`) i buduje `ConstraintScore`. Zwraca
listę w kolejności z pliku (sortowanie do wyświetlania robi się w
warstwie renderowania, nie tutaj — inny konsument tej funkcji może
chcieć inny porządek).

`render_timetable_page` woła `build_constraint_scores(instance, occurrences)`
wewnętrznie — sygnatura funkcji (`instance, occurrences, infeasibility,
objective`) się nie zmienia, bo oba argumenty już są dostępne. Suma
kosztów `required=True` matematycznie równa się parametrowi
`infeasibility`, a `required=False` — `objective` (te same wywołania
`evaluate_constraint` co w `run_solver.py`'s `cost_breakdown`) — nie
trzeba tego asercjować w runtime, to tożsamość wynikająca wprost z kodu.

## Struktura sekcji HTML

Umieszczona w `.page`, po `<main class="grid-wrap">`, przed `</div>`
zamykającym `.page` (czyli przed `<script>`).

```
<section class="evaluation">
  <h2>Ocena rozwiązania</h2>

  <div class="eval-group" data-kind="required">
    <div class="eval-group-head">
      <h3>Ograniczenia wymagane</h3>
      <p class="eval-stat">{naruszone}/{ogółem} naruszonych · suma = {infeasibility}</p>
      <button class="eval-toggle" data-kind="required">Pokaż wszystkie ({ogółem})</button>
    </div>
    <table class="eval-table">
      <thead><tr><th>Nazwa</th><th>Typ</th><th>Waga</th><th>Funkcja kosztu</th><th>Koszt</th></tr></thead>
      <tbody>...</tbody>
    </table>
  </div>

  <div class="eval-group" data-kind="preferred">
    ... analogicznie dla ograniczeń preferowanych (objective) ...
  </div>
</section>
```

Zasady:
- Wiersze w każdej tabeli posortowane malejąco wg `cost`, remisy wg
  `name` (stabilne, czytelne).
- Wiersz z `cost == 0` dostaje klasę `row--ok` i atrybut `hidden` domyślnie
  (ukryty, dopóki nie kliknie się toggle). Wiersz z `cost > 0` dostaje
  `row--bad`, zawsze widoczny.
- Toggle działa identycznie jak istniejący wzorzec `_PAGE_JS` dla
  `.type-tab`/`.chip` (nasłuch na `click`, przełącza `hidden` na wierszach
  z danym `data-kind`, zmienia tekst przycisku między "Pokaż wszystkie (N)"
  i "Pokaż tylko naruszone (M)") — bez przeładowania strony, bez zależności
  zewnętrznych.
- Pusta grupa (0 ograniczeń danego rodzaju w instancji) renderuje krótki
  tekst zamiast tabeli, np. `<p class="eval-empty">Brak ograniczeń
  wymaganych w tej instancji.</p>`.
- Grupa bez żadnego naruszenia (`naruszone == 0`, ale `ogółem > 0`) nadal
  pokazuje tabelę (pustą po domyślnym filtrze) + toggle, żeby użytkownik
  mógł potwierdzić "wszystko spełnione" i zobaczyć listę.

## Styl

Reużycie istniejących custom properties z `_PAGE_CSS` (`--ok`, `--bad`,
`--surface`, `--line`, `--ink-muted`, `--accent`). Kolumny liczbowe
(Waga, Koszt) w `IBM Plex Mono` z `font-variant-numeric: tabular-nums`,
zgodnie z konwencją reszty strony (siatka planu już tak robi dla numerów
okresów). Przycisk `.eval-toggle` stylistycznie jak `.chip`/`.type-tab`
(ten sam border-radius/padding/hover), żeby nie wprowadzać nowego wzorca
wizualnego.

## Testy

Rozszerzenie `tests/test_html_report.py` (TDD):
1. Fixture `ARCHIVE` dostaje 1-2 proste ograniczenia w `<Constraints>`
   (np. jedno `AssignTimeConstraint Required="true"`, jedno
   `PreferTimesConstraint Required="false"` albo podobne — na tyle
   proste, by dało się ręcznie policzyć oczekiwany koszt).
2. `test_build_constraint_scores_computes_cost_per_constraint` — na
   znanym rozwiązaniu (część zdarzeń bez czasu → niezerowy koszt)
   sprawdza, że zwrócona lista ma poprawne `cost`/`required`/`type` dla
   każdego ograniczenia.
3. `test_render_timetable_page_shows_evaluation_section` — sprawdza
   obecność nagłówka "Ocena rozwiązania", obecność nazwy naruszonego
   ograniczenia w widocznej (nie-`hidden`) części HTML, oraz że
   ograniczenie o koszcie 0 dostaje `hidden` i klasę `row--ok`.
4. Rozszerzenie istniejącego testu balansu tagów (`test_render_timetable_
   page_includes_lesson_and_resource_names`) o `section`/`table` z nowej
   sekcji — albo osobny analogiczny test dla samej sekcji ewaluacji, jeśli
   czytelniej.

## Kryteria akceptacji

- `render_timetable_page(instance, occurrences, infeasibility, objective)`
  nadal ma dokładnie taką samą sygnaturę i wszystkie dotychczasowe
  wywołania (w tym `run_solver.py`) działają bez zmian.
- Suma kosztów w tabeli "Ograniczenia wymagane" == `infeasibility` z
  nagłówka; suma w "Ograniczenia preferowane" == `objective` z nagłówka.
- Na dużej realnej instancji (np. `AU-BG-98`, 172 ograniczenia) strona
  domyślnie pokazuje tylko naruszone wiersze — nie zalewa użytkownika
  172 wierszami "OK" bez akcji z jego strony.
- Wszystkie istniejące i nowe testy w `tests/test_html_report.py`
  przechodzą.
