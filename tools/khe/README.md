# KHE & HSEval Tooling

Ten folder zawiera pakiet **KHE (Kingston's High School Timetabling Engine)** oraz oficjalny ewaluator zawodów XHSTT — **HSEval** autorstwa Jeffreya Kingstona.

Służy on w projekcie jako referencyjny walidator (Ground Truth) do weryfikacji poprawności ewaluatora w Pythonie (`xhstt_core/evaluator_ref.py`).

## 1. Kompilacja pod Linuksem / WSL

Aby skompilować KHE i HSEval na maszynie z Linuksem (lub w WSL2):

```bash
cd tools/khe
make build
```

Po zakończeniu kompilacji pliki binarne znajdą się w katalogu `tools/khe/bin/`:
- `tools/khe/bin/hseval` — oficjalny ewaluator XHSTT
- `tools/khe/bin/khe` — solver KHE

Czyszczenie plików pośrednich:
```bash
make clean
```

## 2. Budowanie i uruchamianie w Dockerze

Jeśli chcesz zbudować i uruchomić ewaluator w odizolowanym kontenerze Docker:

```bash
# Budowa obrazu Docker
docker build -t khe-validator tools/khe

# Walidacja pliku rozwiązania przez strumień stdin (nie wymaga montowania wolumenów!)
docker run -i --rm khe-validator -tc /dev/stdin < output/BR-SA-00_solution.xml
```

## 3. Użycie z poziomu Pythona

W projekcie dostępny jest moduł `xhstt_core/validator.py`:

```python
from xhstt_core.validator import KHEValidator

validator = KHEValidator()

# 1. Walidacja pliku XML
result = validator.validate_file("output/BR-SA-00_solution.xml")
print(f"Valid: {result.is_valid}, Infeasibility: {result.infeasibility}, Objective: {result.objective}")

# 2. Walidacja z poziomu kodu (obiekty Instance i Solution)
result = validator.validate_solution(instance, solution)
```

Można go także wywołać z wiersza poleceń:
```bash
uv run python -m xhstt_core.validator output/BR-SA-00_solution.xml
```
