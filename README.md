# LLM-Driven-Reinforcement-Learning-Hyper-Heuristic-for-the-School-Timetabling-Problem

## Szybki start

```
pip install -r requirements.txt   # (na razie: pytest)
python run_solver.py --list       # lista instancji z data/xhstt2014/XHSTT-2014.xml
python run_solver.py AU-BG-98     # buduje rozwiazanie poczatkowe i uruchamia LAHC
```

Parametry: `--iterations N` (domyslnie 30000), `--seed N`, `--history N`
(dlugosc historii LAHC), `--output sciezka.xml` (gdzie zapisac wynik).
Kazdy przebieg zapisuje dwa pliki: `output/<ID>_solution.xml` (mozna
wyslac recznie do [HSEval](http://jeffreykingston.id.au/cgi-bin/hseval.cgi),
pole "file", `op=report`, w celu niezaleznej walidacji) oraz
`output/<ID>_timetable.html` — plan zajec jako siatka dzien x godzina,
z przelacznikiem klasa/nauczyciel/sala (otworzyc w przegladarce).

Testy: `pytest tests/`.
