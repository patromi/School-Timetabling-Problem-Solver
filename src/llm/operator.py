"""LLM as a direct move operator — three variants.

Unlike the code-generation pipeline (pool.py), these operators call the LLM
at solve time to decide which moves to make, rather than generating a reusable
Python function.  Each fits the standard heuristic contract:

    apply(solution, instance, rng) -> Solution

Variant 1 — JSON moves:
    LLM receives violated events + their valid slots, returns a JSON list of
    {"event_ref": ..., "new_time_ref": ...} moves.  Solver applies them directly.

Variant 2 — Event filter:
    LLM selects which violated events to prioritise.  Solver moves each to its
    locally best slot (using _best_time_for_event, same as move_best).

Variant 3 — Chain-of-thought:
    LLM reasons step by step and outputs a sequence of MOVE: {...} lines.
    Solver applies them in order.

All three raise ValueError when:
  - no hard violations exist (nothing to repair)
  - the LLM response cannot be parsed
  - the LLM suggests no valid moves

ValueError is the standard signal to the solver that this heuristic cannot
be applied in the current state; the selector handles it gracefully.
"""

import json
import random
import re
from dataclasses import replace
from typing import Any, cast

from src.evaluator_ref import valid_start_time_ids
from src.heuristics import Heuristic, _best_time_for_event, _movable_violating_indices
from src.incremental import IncrementalEvaluator
from src.model import Instance, Solution


# Maximum violated events sent to LLM — keeps prompts short on large instances.
_MAX_EVENTS_IN_PROMPT = 15
# Maximum valid slots listed per event — avoids flooding the context window.
_MAX_SLOTS_IN_PROMPT = 20


def _format_violations(instance: Instance, constraint_costs: tuple[int, ...]) -> str:
    lines = []
    for c, cost in zip(instance.constraints, constraint_costs, strict=True):
        if cost > 0:
            kind = "HARD" if c.required else "soft"
            lines.append(f"  {kind} {c.type}: cost={cost}")
    return "\n".join(lines) if lines else "  (none)"


def _violated_events_payload(
    instance: Instance,
    solution: Solution,
    evaluator: IncrementalEvaluator,
) -> list[dict[str, Any]]:
    """Return a JSON-serialisable list of violated events with their valid slots."""
    indices = _movable_violating_indices(instance, solution, evaluator)
    payload = []
    for idx in indices[:_MAX_EVENTS_IN_PROMPT]:
        se = solution.events[idx]
        assert se.duration is not None
        slots = list(valid_start_time_ids(instance, se.duration))[:_MAX_SLOTS_IN_PROMPT]
        payload.append(
            {
                "event_ref": se.event_ref,
                "duration": se.duration,
                "current_time": se.time_ref,
                "valid_slots": slots,
            }
        )
    return payload


def _parse_json_list(text: str) -> list[Any]:
    """Extract a JSON array from LLM output, tolerating markdown fences."""
    text = text.strip()
    # Strip ```json ... ``` fences if present
    text = re.sub(r"^```[a-z]*\n?", "", text)
    text = re.sub(r"\n?```$", "", text.strip())
    # Try direct parse first
    try:
        result = json.loads(text)
        if isinstance(result, list):
            return result
    except json.JSONDecodeError:
        pass
    # Fallback: find first [...] block
    match = re.search(r"\[.*?\]", text, re.DOTALL)
    if match:
        return cast(list[Any], json.loads(match.group()))
    raise ValueError(f"no JSON array found in LLM response: {text[:300]!r}")


# ---------------------------------------------------------------------------
# Variant 1 — JSON moves
# ---------------------------------------------------------------------------

def _apply_json_moves(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    from src.llm import client as llm_client

    evaluator = IncrementalEvaluator(instance, solution)
    violated = _violated_events_payload(instance, solution, evaluator)
    if not violated:
        raise ValueError("no violated events — nothing to repair")

    constraint_costs = evaluator.constraint_costs()
    violations_text = _format_violations(instance, constraint_costs)

    messages = [
        {
            "role": "system",
            "content": (
                "You are a school timetabling repair expert. "
                "Return only valid JSON — no explanation, no markdown."
            ),
        },
        {
            "role": "user",
            "content": (
                "## Constraint violations\n"
                f"{violations_text}\n\n"
                "## Events involved in violations (with valid alternative slots)\n"
                f"{json.dumps(violated, indent=2)}\n\n"
                "Return a JSON array of moves that reduce violations.\n"
                'Each move: {"event_ref": "...", "new_time_ref": "..."}\n'
                "Choose new_time_ref ONLY from the event's valid_slots list.\n"
                "Return ONLY the JSON array."
            ),
        },
    ]

    resp = llm_client.call(messages)
    moves = _parse_json_list(resp.content)

    valid_slots_by_ref = {e["event_ref"]: set(e["valid_slots"]) for e in violated}
    events = list(solution.events)
    idx_by_ref = {se.event_ref: i for i, se in enumerate(events)}
    applied = 0

    for move in moves:
        if not isinstance(move, dict):
            continue
        event_ref = move.get("event_ref")
        new_time = move.get("new_time_ref")
        if event_ref not in idx_by_ref or event_ref not in valid_slots_by_ref:
            continue
        if new_time not in valid_slots_by_ref[event_ref]:
            continue
        events[idx_by_ref[event_ref]] = replace(events[idx_by_ref[event_ref]], time_ref=new_time)
        applied += 1

    if applied == 0:
        raise ValueError("LLM suggested no valid moves")
    return replace(solution, events=events)


# ---------------------------------------------------------------------------
# Variant 2 — Event filter + best-slot search
# ---------------------------------------------------------------------------

def _apply_filter(
    solution: Solution, instance: Instance, rng: random.Random
) -> Solution:
    from src.llm import client as llm_client

    evaluator = IncrementalEvaluator(instance, solution)
    indices = _movable_violating_indices(instance, solution, evaluator)
    if not indices:
        raise ValueError("no violated events — nothing to repair")

    violated_refs = [solution.events[i].event_ref for i in indices[:_MAX_EVENTS_IN_PROMPT]]
    constraint_costs = evaluator.constraint_costs()
    violations_text = _format_violations(instance, constraint_costs)

    messages = [
        {
            "role": "system",
            "content": (
                "You are a school timetabling repair expert. "
                "Return only valid JSON — no explanation, no markdown."
            ),
        },
        {
            "role": "user",
            "content": (
                "## Constraint violations\n"
                f"{violations_text}\n\n"
                "## Events involved in violations\n"
                f"{json.dumps(violated_refs)}\n\n"
                "Select 1-3 events most likely to reduce violations when moved.\n"
                "Return a JSON array of event_ref strings.\n"
                "Return ONLY the JSON array."
            ),
        },
    ]

    resp = llm_client.call(messages)
    selected_refs = _parse_json_list(resp.content)

    valid_set = set(violated_refs)
    selected = [r for r in selected_refs if isinstance(r, str) and r in valid_set]
    if not selected:
        raise ValueError("LLM selected no valid events")

    current = solution
    evaluator = IncrementalEvaluator(instance, current)
    for ref in selected:
        idx_by_ref = {se.event_ref: i for i, se in enumerate(current.events)}
        if ref not in idx_by_ref:
            continue
        current = _best_time_for_event(
            instance, current, idx_by_ref[ref], evaluator, rng=rng
        )
        evaluator.commit(current)

    return current


# ---------------------------------------------------------------------------
# Variant 3 — Chain-of-thought MOVE: lines
# ---------------------------------------------------------------------------

_MOVE_PATTERN = re.compile(r"MOVE:\s*(\{[^}]+\})")

def _apply_chain(
    solution: Solution, instance: Instance, rng: random.Random, n_moves: int = 5
) -> Solution:
    from src.llm import client as llm_client

    evaluator = IncrementalEvaluator(instance, solution)
    violated = _violated_events_payload(instance, solution, evaluator)
    if not violated:
        raise ValueError("no violated events — nothing to repair")

    constraint_costs = evaluator.constraint_costs()
    violations_text = _format_violations(instance, constraint_costs)

    events_desc = "\n".join(
        f'  "{e["event_ref"]}" dur={e["duration"]} at "{e["current_time"]}"'
        for e in violated
    )
    # One slots line per unique duration
    seen_durations: set[Any] = set()
    slots_lines: list[str] = []
    for e in violated:
        d = e["duration"]
        if d not in seen_durations:
            seen_durations.add(d)
            slots_lines.append(f"  duration={d}: {e['valid_slots'][:15]}")
    slots_desc = "\n".join(slots_lines)

    messages = [
        {
            "role": "system",
            "content": "You are a school timetabling repair expert.",
        },
        {
            "role": "user",
            "content": (
                "## Constraint violations\n"
                f"{violations_text}\n\n"
                "## Violated events\n"
                f"{events_desc}\n\n"
                "## Available time slots\n"
                f"{slots_desc}\n\n"
                f"Output up to {n_moves} moves to reduce violations.\n"
                "For each move output exactly one line:\n"
                '  MOVE: {"event_ref": "...", "new_time_ref": "..."}\n'
                "Choose new_time_ref only from the slots listed above for that duration.\n"
                "Output ONLY the MOVE lines."
            ),
        },
    ]

    resp = llm_client.call(messages, max_tokens=512)

    moves: list[dict[str, str]] = []
    for match in _MOVE_PATTERN.finditer(resp.content):
        try:
            moves.append(json.loads(match.group(1)))
        except json.JSONDecodeError:
            continue

    if not moves:
        raise ValueError(f"LLM returned no parseable MOVE lines: {resp.content[:300]!r}")

    valid_slots_by_ref = {e["event_ref"]: set(e["valid_slots"]) for e in violated}
    events = list(solution.events)
    idx_by_ref = {se.event_ref: i for i, se in enumerate(events)}
    applied = 0

    for move in moves[:n_moves]:
        event_ref = move.get("event_ref")
        new_time = move.get("new_time_ref")
        if event_ref not in idx_by_ref or event_ref not in valid_slots_by_ref:
            continue
        if new_time not in valid_slots_by_ref[event_ref]:
            continue
        events[idx_by_ref[event_ref]] = replace(events[idx_by_ref[event_ref]], time_ref=new_time)
        applied += 1

    if applied == 0:
        raise ValueError("LLM suggested no valid moves")
    return replace(solution, events=events)


# ---------------------------------------------------------------------------
# Public factory
# ---------------------------------------------------------------------------

LLM_OPERATOR_HEURISTICS: list[Heuristic] = [
    Heuristic(
        id="llm_op_json_moves",
        name="LLM operator: JSON move list",
        protected=True,
        apply=_apply_json_moves,
        incremental_safe=False,
    ),
    Heuristic(
        id="llm_op_filter",
        name="LLM operator: event filter + best-slot search",
        protected=True,
        apply=_apply_filter,
        incremental_safe=False,
    ),
    Heuristic(
        id="llm_op_chain",
        name="LLM operator: chain-of-thought MOVE lines",
        protected=True,
        apply=lambda sol, inst, rng: _apply_chain(sol, inst, rng),
        incremental_safe=False,
    ),
]
