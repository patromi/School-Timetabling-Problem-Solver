"""Replay cache for LLM-generated heuristics.

Path: generated/replay/seed_{seed}_{instance_id}_{variant}_{model}.jsonl
Each line: {"trigger_idx", "candidate_idx", "h_id", "code"}

On a re-run with the same seed, instance, prompt variant and model,
pool.maybe_generate reads from this file instead of calling the LLM.  The
variant and model are part of the key so that runs of different prompt
variants (EXP-L2) or models never replay each other's heuristics.  Loaded
heuristics are marked source="llm_replay" in their Heuristic object so they
are distinguishable in logs and run summaries.
"""

import json
import re
from pathlib import Path
from typing import Any

from src.llm.filelock import append_line

_REPLAY_DIR = Path("generated/replay")


def _safe(text: str) -> str:
    return re.sub(r"[^a-zA-Z0-9_-]", "_", text)


def _replay_path(seed: int, instance_id: str, variant: str, model: str) -> Path:
    return (
        _REPLAY_DIR
        / f"seed_{seed}_{_safe(instance_id)}_{_safe(variant)}_{_safe(model)}.jsonl"
    )


def load(
    seed: int, instance_id: str, *, variant: str, model: str
) -> list[dict[str, Any]]:
    """Return all replay entries for the key, sorted by (trigger_idx, candidate_idx)."""
    path = _replay_path(seed, instance_id, variant, model)
    if not path.exists():
        return []
    entries: list[dict[str, Any]] = []
    with path.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            try:
                entries.append(json.loads(line))
            except json.JSONDecodeError:
                pass
    return sorted(entries, key=lambda e: (e["trigger_idx"], e["candidate_idx"]))


def save_entry(
    seed: int,
    instance_id: str,
    trigger_idx: int,
    candidate_idx: int,
    h_id: str,
    code: str,
    *,
    variant: str,
    model: str,
) -> None:
    """Append one validated heuristic to the replay file."""
    entry = {
        "trigger_idx": trigger_idx,
        "candidate_idx": candidate_idx,
        "h_id": h_id,
        "code": code,
    }
    append_line(
        _replay_path(seed, instance_id, variant, model),
        json.dumps(entry, ensure_ascii=False),
    )


def has_trigger(
    seed: int, instance_id: str, trigger_idx: int, *, variant: str, model: str
) -> bool:
    """Return True if the replay file contains at least one entry for trigger_idx."""
    entries = load(seed, instance_id, variant=variant, model=model)
    return any(e["trigger_idx"] == trigger_idx for e in entries)
