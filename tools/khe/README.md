# KHE & HSEval Tooling

Ten folder zawiera konfigurację pakietu **KHE (Kingston's High School Timetabling Engine)** oraz oficjalnego ewaluatora zawodów XHSTT — **HSEval** autorstwa Jeffreya Kingstona.

Służy on w projekcie jako referencyjny walidator (Ground Truth) do weryfikacji poprawności ewaluatora w Pythonie (`src/evaluator_ref/`).

Źródła biblioteki C są **pobierane na żądanie (On-Demand)** i ignorowane w Gicie, dzięki czemu repozytorium pozostaje lekkie.

## 1. Kompilacja pod Linuksem / WSL

Aby automatycznie pobrać i skompilować KHE i HSEval na maszynie z Linuksem (lub w WSL2):

```bash
cd tools/khe
make build
```

Nadrzędny `Makefile`:
1. Automatycznie sprawdza, czy archiwum lub źródła istnieją (korzysta także z lokalnego cache w `downloads/`, jeśli istnieje).
2. W razie potrzeby pobiera oficjalną paczkę `khe-2025_12_04.tar.gz` z serwera autora.
3. Kompiluje biblioteki i umieszcza gotowe binaria w `tools/khe/bin/`:
   - `tools/khe/bin/hseval` — oficjalny ewaluator XHSTT
   - `tools/khe/bin/khe` — solver KHE

Dodatkowe polecenia:
- `make fetch` — samo pobranie i rozpakowanie źródeł C (bez kompilacji).
- `make clean` — usunięcie plików pośrednich `.o` oraz katalogu `bin/`.
- `make distclean` — usunięcie binariów oraz całego pobranego katalogu ze źródłami C.

## 2. Budowanie i uruchamianie w Dockerze

Kontener Docker pobiera źródła i kompiluje KHE w sposób całkowicie samowystarczalny:

```bash
# Budowa obrazu Docker
docker build -t khe-validator tools/khe

# Walidacja pliku rozwiązania przez strumień stdin (nie wymaga montowania wolumenów!)
docker run -i --rm khe-validator -tc /dev/stdin < output/BR-SA-00_solution.xml
```

## 3. Użycie z poziomu Pythona

W projekcie dostępny jest moduł `src/validator.py`:

```python
from src.validator import KHEValidator

validator = KHEValidator()

# 1. Walidacja pliku XML
result = validator.validate_file("output/BR-SA-00_solution.xml")
print(f"Valid: {result.is_valid}, Infeasibility: {result.infeasibility}, Objective: {result.objective}")

# 2. Walidacja z poziomu kodu (obiekty Instance i Solution)
result = validator.validate_solution(instance_xml, solution)
```

Z wiersza poleceń:
```bash
uv run python -m src.validator output/BR-SA-00_solution.xml
```
