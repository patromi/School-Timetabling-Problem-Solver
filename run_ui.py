"""Uruchamia lokalny serwer aplikacji UI i otwiera przeglądarkę."""

import argparse
import sys
import threading
import time
import webbrowser
from pathlib import Path

import uvicorn

REPO_ROOT = Path(__file__).resolve().parent


def open_browser(url: str, delay: float = 1.0) -> None:
    time.sleep(delay)
    print(f"\n[UI] Otwieram przeglądarkę pod adresem: {url}")
    webbrowser.open_new(url)


def main() -> None:
    parser = argparse.ArgumentParser(description="Uruchomienie interfejsu graficznego (UI) solvera planów lekcji.")
    parser.add_argument("--host", default="127.0.0.1", help="Host serwera (domyślnie 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8000, help="Port serwera (domyślnie 8000)")
    parser.add_argument("--no-browser", action="store_true", help="Nie otwieraj automatycznie przeglądarki")
    args = parser.parse_args()

    url = f"http://{args.host}:{args.port}"
    print("=" * 60)
    print(f"  School Timetabling Problem Solver - Web UI")
    print(f"  Serwer uruchomiony pod adresem: {url}")
    print("=" * 60)

    if not args.no_browser:
        threading.Thread(target=open_browser, args=(url, 1.2), daemon=True).start()

    uvicorn.run("src.api:app", host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()
