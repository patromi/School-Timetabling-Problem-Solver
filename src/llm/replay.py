"""Seed-keyed replay cache for LLM-generated heuristics.

Path: generated/replay/seed_{seed}_{instance_id}.jsonl
Each line: {"trigger_idx", "candidate_idx", "h_id", "code"}

On re-run with the same seed + instance, pool.maybe_generate reads from this
file instead of calling the LLM.  Loaded heuristics are marked
source="llm_replay" in their Heuristic object so they are distinguishable in
logs and run summaries.
"""

import json
import re
from pathlib import Path
from typing import Any

_REPLAY_DIR = Path("generated/replay")


def _replay_path(seed: int, instance_id: str) -> Path:
    safe_id = re.sub(r"[^a-zA-Z0-9_-]", "_", instance_id)
    return _REPLAY_DIR / f"seed_{seed}_{safe_id}.jsonl"


def load(seed: int, instance_id: str) -> list[dict[str, Any]]:
    """Return all replay entries for (seed, instance_id), sorted by (trigger_idx, candidate_idx)."""
    path = _replay_path(seed, instance_id)
    if not path.exists():
        return []
    entries: list[dict[str, object]] = []
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
) -> None:
    """Append one validated heuristic to the replay file."""
    _REPLAY_DIR.mkdir(parents=True, exist_ok=True)
    path = _replay_path(seed, instance_id)
    entry = {
        "trigger_idx": trigger_idx,
        "candidate_idx": candidate_idx,
        "h_id": h_id,
        "code": code,
    }
    with path.open("a", encoding="utf-8") as fh:
        fh.write(json.dumps(entry, ensure_ascii=False) + "\n")


def has_trigger(seed: int, instance_id: str, trigger_idx: int) -> bool:
    """Return True if the replay file contains at least one entry for trigger_idx."""
    entries = load(seed, instance_id)
    return any(e["trigger_idx"] == trigger_idx for e in entries)
