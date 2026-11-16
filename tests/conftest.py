"""Shared pytest fixtures."""

from collections.abc import Iterator
from pathlib import Path
from unittest.mock import patch

import pytest


@pytest.fixture(autouse=True)
def _generated_dir_in_tmp(tmp_path: Path) -> Iterator[None]:
    """Keep heuristics that tests "generate" (mock LLM responses) out of the
    repo's generated/ directory: its .py files are tracked by git as thesis
    material, and test runs used to leave mock heuristics there."""
    with patch("src.llm.pool._GENERATED_DIR", tmp_path / "generated"):
        yield
