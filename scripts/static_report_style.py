"""Wspólny styl wykresów raportu eksperymentów statycznych (report_static.py).

Kolory biorą się z palety referencyjnej (sloty kategoryczne w ustalonej
kolejności, kolor przypisany do selektora, nigdy do jego miejsca w rankingu)
i są te same co w plot_convergence.py, żeby wykresy w pracy były spójne.
"""

import csv
from collections.abc import Iterable, Sequence
from pathlib import Path
from typing import Any

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
from matplotlib.figure import Figure

TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
TEXT_MUTED = "#8a8984"
GRID = "#e4e3df"
SURFACE = "#ffffff"
NEUTRAL = "#b5b3ad"  # remis, konfiguracja odniesienia

SELECTOR_ORDER = ["random", "epsilon-greedy", "ucb", "ucb-preprocessing"]
SELECTOR_COLORS = {
    "random": "#2a78d6",
    "epsilon-greedy": "#eb6834",
    "ucb": "#1baf7a",
    "ucb-preprocessing": "#eda100",
}
CATEGORICAL = [
    "#2a78d6",
    "#eb6834",
    "#1baf7a",
    "#eda100",
    "#e87ba4",
    "#008300",
    "#6250d6",
    "#e34948",
]
# rampa niebieska (sekwencyjna) od jasnego do ciemnego
BLUE_RAMP = [
    "#cde2fb",
    "#9ec5f4",
    "#6da7ec",
    "#3987e5",
    "#256abf",
    "#184f95",
    "#0d366b",
]
# rampa pomarańczowa do drugiej rodziny kolorów (stany niewykonalne itp.)
ORANGE_RAMP = ["#fbd5c4", "#f4a582", "#eb6834", "#b8461c"]

# wygrana A / remis / wygrana B w porównaniach par
WIN_A = "#2a78d6"
WIN_B = "#eb6834"


def selector_color(selector: str) -> str:
    return SELECTOR_COLORS.get(selector, TEXT_SECONDARY)


def order_selectors(selectors: Iterable[str]) -> list[str]:
    present = set(selectors)
    first = [s for s in SELECTOR_ORDER if s in present]
    return first + sorted(present - set(first))


def style_axes(ax: Axes, grid_axis: str = "y") -> None:
    """Recesywna siatka i osie: bez górnej/prawej ramki, szare podpisy."""
    if grid_axis:
        ax.grid(True, axis=grid_axis, color=GRID, linewidth=0.6, zorder=0)
    ax.set_axisbelow(True)
    ax.tick_params(colors=TEXT_SECONDARY, labelsize=8, length=2)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)


def panel_title(ax: Axes, text: str) -> None:
    ax.set_title(text, color=TEXT_PRIMARY, fontsize=10, loc="left")


def axis_label(ax: Axes, x: str | None = None, y: str | None = None) -> None:
    if x is not None:
        ax.set_xlabel(x, color=TEXT_SECONDARY, fontsize=8)
    if y is not None:
        ax.set_ylabel(y, color=TEXT_SECONDARY, fontsize=8)


def footnote(fig: Figure, text: str) -> None:
    fig.text(
        0.5, 0.005, text, ha="center", va="bottom", fontsize=7, color=TEXT_SECONDARY
    )


def legend_top(
    fig: Figure, handles: Sequence[Any], labels: Sequence[str], ncol: int = 6
) -> None:
    fig.legend(
        handles,
        labels,
        loc="upper center",
        ncol=min(len(labels), ncol),
        frameon=False,
        fontsize=8,
        labelcolor=TEXT_PRIMARY,
    )


def small_multiples(
    n: int, ncols: int, panel_w: float, panel_h: float, **kw: Any
) -> tuple[Figure, list[Axes]]:
    """Siatka paneli na n małych wykresów; nadmiarowe panele są wyłączone."""
    nrows = -(-n // ncols)
    fig, axes = plt.subplots(
        nrows,
        ncols,
        figsize=(panel_w * ncols, panel_h * nrows + 0.8),
        squeeze=False,
        **kw,
    )
    flat = [ax for row in axes for ax in row]
    for ax in flat[n:]:
        ax.set_axis_off()
    return fig, flat[:n]


def figure_paths(path: Path) -> tuple[Path, Path]:
    """Ścieżki PNG i PDF wykresu `path` (<raport>/wykresy/<nazwa>):
    <raport>/wykresy/png/<nazwa>.png i <raport>/wykresy/pdf/<nazwa>.pdf."""
    png = path.parent / "png" / path.with_suffix(".png").name
    pdf = path.parent / "pdf" / path.with_suffix(".pdf").name
    png.parent.mkdir(parents=True, exist_ok=True)
    pdf.parent.mkdir(parents=True, exist_ok=True)
    return png, pdf


def save(fig: Figure, path: Path, top: float = 0.93, bottom: float = 0.03) -> None:
    """Zapisuje PNG (200 dpi) i PDF pod tą samą nazwą (zob. figure_paths)."""
    png, pdf = figure_paths(path)
    fig.tight_layout(rect=(0, bottom, 1, top))
    fig.savefig(png, dpi=200, facecolor=SURFACE)
    fig.savefig(pdf, facecolor=SURFACE)
    plt.close(fig)


def data_path(path: Path) -> Path:
    """Gdzie trafia CSV z danymi wykresu: wykres leży w <raport>/wykresy/,
    jego dane w <raport>/dane/ pod tą samą nazwą."""
    return path.parent.parent / "dane" / path.with_suffix(".csv").name


def write_csv(path: Path, header: Sequence[str], rows: Iterable[Sequence[Any]]) -> None:
    """Widok tabelaryczny danych wykresu (zob. data_path)."""
    target = data_path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)
