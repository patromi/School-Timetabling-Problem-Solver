# Sekcja "Ocena rozwiązania" w raporcie HTML — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a per-constraint cost breakdown ("Ocena rozwiązania") to the
end of `xhstt_core/html_report.py`'s `render_timetable_page`, so a reader
can see which XHSTT constraints are violated and by how much, without
reading raw XML or running a separate CLI report.

**Architecture:** A new pure function `build_constraint_scores(instance,
occurrences)` (calls the already-implemented `evaluate_constraint` from
`xhstt_core/evaluator_ref.py` once per instance constraint) feeds a new
rendering function `render_evaluation_section(scores, infeasibility,
objective)`. `render_timetable_page` calls both internally — its public
signature does not change. The section is two grouped tables (required /
preferred), each sorted by cost descending, with zero-cost rows hidden by
default and a client-side toggle (same vanilla-JS pattern already used for
the resource-type tabs) to reveal them.

**Tech Stack:** Python 3 (dataclasses, stdlib `html.escape`), inline
vanilla JS/CSS (no build step, no external assets — matches the rest of
`html_report.py`), pytest.

## Global Constraints

- `render_timetable_page(instance, occurrences, infeasibility, objective)`
  keeps its exact current signature — no caller (`run_solver.py`,
  existing tests) changes.
- No new third-party dependencies.
- All new UI-facing strings are Polish with full diacritics (matches
  existing strings in the file, e.g. "plan zajęć", "wykonalny").
- Reuse existing CSS custom properties (`--ok`, `--bad`, `--surface`,
  `--line`, `--ink-muted`, `--accent`, `--ink`) and fonts (`IBM Plex
  Sans`/`IBM Plex Mono`) — no new palette or font.
- TDD: for every code step, write the failing test first, watch it fail,
  then implement.
- Reference spec: `docs/superpowers/specs/2026-07-19-html-report-evaluation-section-design.md`.

---

### Task 1: `ConstraintScore` + `build_constraint_scores`

**Files:**
- Modify: `xhstt_core/html_report.py` (imports at top, new dataclass and
  function after the existing `TimetableCell` dataclass / before
  `build_days`)
- Test: `tests/test_html_report.py`

**Interfaces:**
- Produces: `ConstraintScore` dataclass (`id: str, name: str, type: str,
  required: bool, weight: int, cost_function: str, cost: int`) and
  `build_constraint_scores(instance: Instance, occurrences:
  list[Occurrence]) -> list[ConstraintScore]`, both importable from
  `xhstt_core.html_report`. Order of the returned list matches
  `instance.constraints` (file order) — sorting for display is the
  renderer's job (Task 2), not this function's.

- [ ] **Step 1: Write the failing test**

Add near the top of `tests/test_html_report.py`, after the existing
`ARCHIVE` constant, a second fixture archive with constraints, plus a
loader and the new test:

```python
CONSTRAINTS_ARCHIVE = """<HighSchoolTimetableArchive>
  <Instances>
    <Instance Id="I1">
      <MetaData><Name>Test School</Name></MetaData>
      <Times>
        <TimeGroups>
          <Day Id="gr_Mon"><Name>Poniedzialek</Name></Day>
          <Day Id="gr_Tue"><Name>Wtorek</Name></Day>
        </TimeGroups>
        <Time Id="Mon_1"><Name>Mon_1</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Mon_2"><Name>Mon_2</Name><Day Reference="gr_Mon"/></Time>
        <Time Id="Tue_1"><Name>Tue_1</Name><Day Reference="gr_Tue"/></Time>
        <Time Id="Tue_2"><Name>Tue_2</Name><Day Reference="gr_Tue"/></Time>
      </Times>
      <Resources>
        <ResourceTypes>
          <ResourceType Id="Teacher"><Name>Teacher</Name></ResourceType>
          <ResourceType Id="Class"><Name>Class</Name></ResourceType>
        </ResourceTypes>
        <ResourceGroups></ResourceGroups>
        <Resource Id="C1"><Name>7A</Name><ResourceType Reference="Class"/><ResourceGroups></ResourceGroups></Resource>
        <Resource Id="T1"><Name>Kowalski</Name><ResourceType Reference="Teacher"/><ResourceGroups></ResourceGroups></Resource>
      </Resources>
      <Events>
        <EventGroups></EventGroups>
        <Event Id="E1">
          <Name>Matematyka</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
        <Event Id="E2">
          <Name>Fizyka musi miec czas</Name>
          <Duration>2</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
        <Event Id="E3">
          <Name>Chemia musi miec czas</Name>
          <Duration>1</Duration>
          <Resources>
            <Resource Reference="C1"><Role>Class</Role></Resource>
            <Resource Reference="T1"><Role>Teacher</Role></Resource>
          </Resources>
        </Event>
      </Events>
      <Constraints>
        <AssignTimeConstraint Id="AT1">
          <Name>Fizyka musi miec czas</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Events><Event Reference="E2"/></Events></AppliesTo>
        </AssignTimeConstraint>
        <AssignTimeConstraint Id="AT2">
          <Name>Chemia musi miec czas</Name>
          <Required>true</Required>
          <Weight>1</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Events><Event Reference="E3"/></Events></AppliesTo>
        </AssignTimeConstraint>
        <PreferTimesConstraint Id="PT1">
          <Name>Matematyka wolimy we wtorek</Name>
          <Required>false</Required>
          <Weight>5</Weight>
          <CostFunction>Linear</CostFunction>
          <AppliesTo><Events><Event Reference="E1"/></Events></AppliesTo>
          <TimeGroups><TimeGroup Reference="gr_Tue"/></TimeGroups>
        </PreferTimesConstraint>
      </Constraints>
    </Instance>
  </Instances>
</HighSchoolTimetableArchive>"""


def _instance_with_constraints():
    return parse_archive(CONSTRAINTS_ARCHIVE)[0]


def test_build_constraint_scores_computes_cost_per_constraint():
    instance = _instance_with_constraints()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E3", time_ref="Tue_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    scores = build_constraint_scores(instance, occurrences)

    by_id = {s.id: s for s in scores}
    assert len(scores) == 3
    # E2 never gets a time in the solution -> AssignTimeConstraint
    # deviation is its full duration (2), weight 1 -> cost 2.
    assert by_id["AT1"].cost == 2
    assert by_id["AT1"].required is True
    assert by_id["AT1"].type == "AssignTimeConstraint"
    assert by_id["AT1"].weight == 1
    assert by_id["AT1"].cost_function == "Linear"
    # E3 gets Tue_2 -> fully assigned -> deviation 0.
    assert by_id["AT2"].cost == 0
    # E1 gets Mon_1, preferred group is gr_Tue -> deviation = duration 1,
    # weight 5 -> cost 5.
    assert by_id["PT1"].cost == 5
    assert by_id["PT1"].required is False
```

Update the two `from xhstt_core...` import lines at the top of the test
file to also pull in the new names:

```python
from xhstt_core.evaluator_ref import resolve_occurrences
from xhstt_core.html_report import (
    build_constraint_scores,
    build_days,
    build_resource_grid,
    render_timetable_page,
)
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_html_report.py::test_build_constraint_scores_computes_cost_per_constraint -v`
Expected: FAIL — `ImportError: cannot import name 'build_constraint_scores'`

- [ ] **Step 3: Write minimal implementation**

In `xhstt_core/html_report.py`:

1. Change the evaluator import line:

```python
from xhstt_core.evaluator_ref import Occurrence, evaluate_constraint
```

2. Add the dataclass and function right after `TimetableCell` (before
   `build_days`):

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


def build_constraint_scores(
    instance: Instance, occurrences: list[Occurrence]
) -> list[ConstraintScore]:
    """One score per instance constraint, in file order -- callers sort
    and group for display as needed."""
    return [
        ConstraintScore(
            id=c.id,
            name=c.name,
            type=c.type,
            required=c.required,
            weight=c.weight,
            cost_function=c.cost_function,
            cost=evaluate_constraint(instance, occurrences, c),
        )
        for c in instance.constraints
    ]
```

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_html_report.py::test_build_constraint_scores_computes_cost_per_constraint -v`
Expected: PASS

- [ ] **Step 5: Run the full test file to check nothing else broke**

Run: `python -m pytest tests/test_html_report.py -v`
Expected: all tests PASS (existing 6 + 1 new = 7)

- [ ] **Step 6: Commit**

```bash
git add xhstt_core/html_report.py tests/test_html_report.py
git commit -m "feat: add per-constraint cost scoring to html_report"
```

---

### Task 2: Render the "Ocena rozwiązania" section, wire it in, style it

**Files:**
- Modify: `xhstt_core/html_report.py` (`render_timetable_page`, `_PAGE_CSS`,
  `_PAGE_JS`; new private render helpers)
- Test: `tests/test_html_report.py`

**Interfaces:**
- Consumes: `ConstraintScore` and `build_constraint_scores` from Task 1.
- Produces: `render_evaluation_section(scores: list[ConstraintScore],
  infeasibility: int, objective: int) -> str` (returns a `<section
  class="evaluation">...</section>` HTML fragment). `render_timetable_page`
  calls it internally and appends its output inside `.page`, after
  `<main class="grid-wrap">`.

- [ ] **Step 1: Write the failing test**

Add to `tests/test_html_report.py`:

```python
def test_render_timetable_page_shows_evaluation_section():
    instance = _instance_with_constraints()
    solution = Solution(
        instance_ref=instance.id,
        events=[
            SolutionEvent(event_ref="E1", time_ref="Mon_1"),
            SolutionEvent(event_ref="E3", time_ref="Tue_2"),
        ],
    )
    occurrences = resolve_occurrences(instance, solution)

    html = render_timetable_page(instance, occurrences, infeasibility=2, objective=5)

    assert "Ocena rozwiązania" in html
    assert "Ograniczenia wymagane" in html
    assert "Ograniczenia preferowane" in html
    assert "suma = 2" in html
    assert "suma = 5" in html

    # AT1 (Fizyka, cost 2) is violated -> its row is visible by default.
    at1_pos = html.index("Fizyka musi miec czas", html.index("Ocena rozwiązania"))
    row1_start = html.rindex("<tr", 0, at1_pos)
    row1_tag = html[row1_start : html.index(">", row1_start) + 1]
    assert "row--bad" in row1_tag
    assert "hidden" not in row1_tag

    # AT2 (Chemia, cost 0) is satisfied -> its row starts hidden.
    at2_pos = html.index("Chemia musi miec czas", html.index("Ocena rozwiązania"))
    row2_start = html.rindex("<tr", 0, at2_pos)
    row2_tag = html[row2_start : html.index(">", row2_start) + 1]
    assert "row--ok" in row2_tag
    assert "hidden" in row2_tag


def test_render_timetable_page_evaluation_section_handles_no_constraints():
    # The base fixture instance (_instance(), from the top of this file)
    # has an empty <Constraints/> block -- both groups must fall back to
    # their "brak ograniczen" message instead of rendering empty/broken
    # tables.
    instance = _instance()
    solution = Solution(
        instance_ref=instance.id,
        events=[SolutionEvent(event_ref="E1", time_ref="Mon_1")],
    )
    occurrences = resolve_occurrences(instance, solution)

    html = render_timetable_page(instance, occurrences, infeasibility=0, objective=0)

    assert "Brak ograniczeń wymaganych w tej instancji." in html
    assert "Brak ograniczeń preferowanych w tej instancji." in html
```

Also extend the existing tag-balance test to include the new `section`
tag:

```python
    for tag in ("table", "div", "ul", "li", "dd", "dl", "html", "body", "section"):
```

(single-line change to the `for tag in (...)` line inside
`test_render_timetable_page_includes_lesson_and_resource_names`)

- [ ] **Step 2: Run tests to verify they fail**

Run: `python -m pytest tests/test_html_report.py -k evaluation_section -v`
Expected: FAIL — `AssertionError` (section text not found in HTML yet)

- [ ] **Step 3: Write minimal implementation**

In `xhstt_core/html_report.py`, add these three functions after
`_render_resource_table` and before `render_timetable_page`:

```python
def _render_eval_row(score: ConstraintScore) -> str:
    row_class = "row--bad" if score.cost > 0 else "row--ok"
    hidden_attr = "" if score.cost > 0 else " hidden"
    return (
        f'<tr class="{row_class}"{hidden_attr}>'
        f"<td>{escape(score.name)}</td>"
        f'<td class="mono">{escape(score.type)}</td>'
        f'<td class="mono num">{score.weight}</td>'
        f'<td class="mono">{escape(score.cost_function)}</td>'
        f'<td class="mono num">{score.cost}</td>'
        f"</tr>"
    )


def _render_eval_group(
    kind: str, title: str, scores: list[ConstraintScore], total: int
) -> str:
    if not scores:
        label = "wymaganych" if kind == "required" else "preferowanych"
        return (
            f'<div class="eval-group" data-kind="{kind}">'
            f"<h3>{escape(title)}</h3>"
            f'<p class="eval-empty">Brak ograniczeń {label} w tej instancji.</p>'
            f"</div>"
        )
    ordered = sorted(scores, key=lambda s: (-s.cost, s.name))
    violated = sum(1 for s in ordered if s.cost > 0)
    rows = "".join(_render_eval_row(s) for s in ordered)
    return f"""<div class="eval-group" data-kind="{kind}">
  <div class="eval-group-head">
    <h3>{escape(title)}</h3>
    <p class="eval-stat">{violated}/{len(ordered)} naruszonych &middot; suma = {total}</p>
    <button class="eval-toggle" type="button" data-kind="{kind}" data-total="{len(ordered)}" data-violated="{violated}" data-expanded="false">Pokaż wszystkie ({len(ordered)})</button>
  </div>
  <table class="eval-table">
    <thead><tr><th>Nazwa</th><th>Typ</th><th>Waga</th><th>Funkcja kosztu</th><th>Koszt</th></tr></thead>
    <tbody>{rows}</tbody>
  </table>
</div>"""


def render_evaluation_section(
    scores: list[ConstraintScore], infeasibility: int, objective: int
) -> str:
    required = [s for s in scores if s.required]
    preferred = [s for s in scores if not s.required]
    return f"""<section class="evaluation">
  <h2>Ocena rozwiązania</h2>
  {_render_eval_group("required", "Ograniczenia wymagane", required, infeasibility)}
  {_render_eval_group("preferred", "Ograniczenia preferowane", preferred, objective)}
</section>"""
```

Wire it into `render_timetable_page`: add this line before the `return
f"""..."""` (near where `feasible`/`status_class` are computed):

```python
    scores = build_constraint_scores(instance, occurrences)
    eval_section = render_evaluation_section(scores, infeasibility, objective)
```

And change the body of the returned template from:

```python
  <main class="grid-wrap" id="grid-wrap">{tables}</main>
</div>
```

to:

```python
  <main class="grid-wrap" id="grid-wrap">{tables}</main>

  {eval_section}
</div>
```

Append to `_PAGE_CSS` (before the closing `"""`):

```css

.evaluation { display: flex; flex-direction: column; gap: 1.25rem; }
.evaluation h2 {
  margin: 0;
  font-size: 1.3rem;
  font-weight: 700;
  border-bottom: 2px solid var(--ink);
  padding-bottom: 0.6rem;
}
.eval-group {
  background: var(--surface);
  border: 1px solid var(--line);
  padding: 1rem 1.25rem;
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}
.eval-group-head {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 0.75rem 1.25rem;
}
.eval-group-head h3 { margin: 0; font-size: 1.05rem; font-weight: 700; flex: 1 1 auto; }
.eval-stat {
  margin: 0;
  font-family: 'IBM Plex Mono', monospace;
  font-size: 0.82rem;
  color: var(--ink-muted);
}
.eval-empty { margin: 0; color: var(--ink-muted); font-size: 0.9rem; }
.eval-toggle {
  font-family: 'IBM Plex Sans', sans-serif;
  font-size: 0.8rem;
  font-weight: 600;
  border: 1px solid var(--line);
  background: var(--surface);
  color: var(--ink);
  padding: 0.35rem 0.8rem;
  border-radius: 999px;
  cursor: pointer;
  transition: border-color 0.15s ease;
}
.eval-toggle:hover { border-color: var(--accent); }
table.eval-table { width: 100%; border-collapse: collapse; }
table.eval-table th, table.eval-table td {
  border-bottom: 1px solid var(--line);
  padding: 0.5rem 0.6rem;
  text-align: left;
  font-size: 0.85rem;
}
table.eval-table thead th {
  font-size: 0.72rem;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--ink-muted);
}
table.eval-table td.mono { font-family: 'IBM Plex Mono', monospace; }
table.eval-table td.num { text-align: right; font-variant-numeric: tabular-nums; }
tr.row--bad td:first-child { border-left: 3px solid var(--bad); padding-left: calc(0.6rem - 3px); }
tr.row--ok td:first-child {
  border-left: 3px solid var(--ok);
  padding-left: calc(0.6rem - 3px);
  color: var(--ink-muted);
}
```

Append to `_PAGE_JS`, inside the existing IIFE, right before the closing
`})();` line:

```js

  var evalToggles = Array.prototype.slice.call(document.querySelectorAll('.eval-toggle'));
  evalToggles.forEach(function (btn) {
    btn.addEventListener('click', function () {
      var kind = btn.dataset.kind;
      var group = document.querySelector('.eval-group[data-kind="' + kind + '"]');
      var okRows = Array.prototype.slice.call(group.querySelectorAll('tr.row--ok'));
      var expanded = btn.dataset.expanded === 'true';
      okRows.forEach(function (row) { row.hidden = expanded; });
      btn.dataset.expanded = expanded ? 'false' : 'true';
      btn.textContent = expanded
        ? 'Pokaż wszystkie (' + btn.dataset.total + ')'
        : 'Pokaż tylko naruszone (' + btn.dataset.violated + ')';
    });
  });
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `python -m pytest tests/test_html_report.py -v`
Expected: all tests PASS (7 from Task 1 + 2 new = 9)

- [ ] **Step 5: Commit**

```bash
git add xhstt_core/html_report.py tests/test_html_report.py
git commit -m "feat: render evaluation section with required/preferred constraint tables"
```

---

### Task 3: Manual verification against a real instance

**Files:** none (verification only — run the existing CLI, inspect output)

**Interfaces:** none (uses `run_solver.py`, already wired to
`render_timetable_page` in Task 2's changes; no code changes in this task)

- [ ] **Step 1: Generate a report for a small real instance**

Run: `python run_solver.py --list`

Pick any small instance id from the printed table (few dozen events) —
this just needs to run fast. Then:

Run: `python run_solver.py <SMALL_ID> --iterations 2000 --seed 0`

Expected: command finishes, prints `Plan zajec (HTML) zapisany do:
output/<SMALL_ID>_timetable.html`.

- [ ] **Step 2: Open it and check the section by eye**

Run (Windows): `start output/<SMALL_ID>_timetable.html`

In the browser, scroll to the bottom and confirm:
- "Ocena rozwiązania" heading is present, with two sub-groups.
- The numbers next to "suma =" in each group match the `infeasibility`/
  `objective` values shown in the page header.
- Clicking "Pokaż wszystkie (N)" reveals the previously-hidden satisfied
  rows and the button relabels to "Pokaż tylko naruszone (M)"; clicking
  again re-hides them and restores the original label.

- [ ] **Step 3: Repeat on a large instance to confirm the default filter matters**

Run: `python run_solver.py AU-BG-98 --iterations 2000 --seed 0`
Run (Windows): `start output/AU-BG-98_timetable.html`

Confirm: with 172 constraints on this instance, the page does **not**
dump ~170 "OK" rows on load — only the violated ones are visible until
"Pokaż wszystkie" is clicked. This is the acceptance criterion from the
spec ("nie zalewa użytkownika 172 wierszami OK bez akcji z jego strony").

- [ ] **Step 4: Run the full test suite one more time**

Run: `python -m pytest -q`
Expected: all tests PASS, no regressions elsewhere in the repo.

- [ ] **Step 5: Report back**

Summarize in the conversation: which instance(s) were checked, whether
the three checks in Step 2/3 held, and the final `pytest -q` result. No
commit in this task (nothing changed).
