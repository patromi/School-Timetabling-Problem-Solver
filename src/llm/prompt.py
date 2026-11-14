"""Diagnostic prompt builder for LLM heuristic generation.

Three variants:
  "a"   — aggregates only (constraint violation counts, heuristic ranking,
           stagnation state, existing signatures)
  "b"   — aggregates + concrete violation examples (specific resource/time
           clashes visible in the current solution)
  "rag" — aggregates + knowledge-base instructions retrieved from kb/

All variants share the same system prompt and function contract.  The user
message differs in how much problem-specific context is included.
"""

from dataclasses import dataclass

from src.heuristics import Heuristic
from src.llm.kb import retrieve
from src.model import Instance, Solution
from src.selectors import UCBSelector

VARIANT_A = "a"
VARIANT_B = "b"
VARIANT_RAG = "rag"

_SYSTEM_PROMPT = """\
You are an expert in combinatorial optimisation and Python.
Your task: write ONE new heuristic operator for a school timetabling solver.

## Problem context
The solver assigns a start time (time_ref) to each scheduled event.
A solution is a list of SolutionEvent objects:
  - event_ref: str          — which instance event
  - time_ref: str | None    — assigned start time id (None = unassigned)
  - duration: int | None    — duration in time slots
  - resources: list[SolutionEventResource]

The Instance exposes:
  - instance.times          — list[Time], each with .id and .name
  - instance.events         — list[Event], each with .id, .duration, .resources
  - instance.constraints    — list[Constraint] with .type, .required, .weight

## CONTRACT — you MUST follow this exactly
```python
def apply(solution: Solution, instance: Instance, rng: random.Random) -> Solution:
    ...
```
Rules:
- Never modify solution in-place. Use dataclasses.replace() to build new objects.
- Return a Solution with the EXACT same set of event_ref values as the input.
- Raise ValueError if the move cannot be applied (e.g. no eligible events).
- Allowed imports: math, random, collections, itertools, dataclasses, typing
- Forbidden: os, sys, subprocess, socket, open, eval, exec, __import__
- You may call valid_start_time_ids(instance, duration) → list[str] to get
  the list of legal start time ids for an event of the given duration.
- You may call dataclasses.replace(obj, field=value) to copy with changes.

## Output format
Return ONLY the raw Python function — no markdown, no explanation, no imports.\
"""


@dataclass(frozen=True)
class Diagnostic:
    """All data the prompt builder needs, pre-computed by the caller."""

    instance: Instance
    solution: Solution
    infeasibility: int
    objective: int
    constraint_costs: tuple[int, ...]  # one int per instance.constraints entry
    stagnant_iterations: int
    pool: list[Heuristic]
    selector: UCBSelector | None  # None when using a non-UCB selector


def _constraint_summary(diagnostic: Diagnostic) -> str:
    lines: list[str] = []
    for constraint, cost in zip(
        diagnostic.instance.constraints,
        diagnostic.constraint_costs,
        strict=True,
    ):
        if cost == 0:
            continue
        kind = "HARD" if constraint.required else "soft"
        lines.append(f"  {kind} {constraint.type}: cost={cost}")
    if not lines:
        return "  (no constraint violations)"
    return "\n".join(lines)


def _heuristic_ranking(diagnostic: Diagnostic) -> str:
    sel = diagnostic.selector
    if sel is None or not diagnostic.pool:
        return "  (no selector statistics available)"

    ranked = sorted(
        diagnostic.pool,
        key=lambda h: sel.mean_q(h),
        reverse=True,
    )
    lines: list[str] = []
    top = ranked[:3]
    bottom = ranked[-3:] if len(ranked) > 3 else []
    for h in top:
        q = sel.mean_q(h)
        n = int(sel.total_pulls(h))
        lines.append(f"  [top]    {h.id}  Q={q:.3f}  pulls={n}")
    if bottom:
        lines.append("  ...")
        for h in bottom:
            q = sel.mean_q(h)
            n = int(sel.total_pulls(h))
            lines.append(f"  [bottom] {h.id}  Q={q:.3f}  pulls={n}")
    return "\n".join(lines)


def _existing_signatures(pool: list[Heuristic]) -> str:
    return "\n".join(f"  - {h.id}: {h.name}" for h in pool)


def _violation_examples(diagnostic: Diagnostic, max_examples: int = 5) -> str:
    """Variant B: extract concrete resource-clash examples from the solution."""
    from collections import defaultdict

    from src.evaluator_ref import resolve_occurrences

    occurrences = resolve_occurrences(diagnostic.instance, diagnostic.solution)

    # Build time_ref → list of (resource_ref, event_ref) for clash detection
    resource_at_time: dict[str, dict[str, list[str]]] = defaultdict(
        lambda: defaultdict(list)
    )
    time_ids = [t.id for t in diagnostic.instance.times]
    pos = {t: i for i, t in enumerate(time_ids)}

    for o in occurrences:
        if o.time_ref is None:
            continue
        start = pos[o.time_ref]
        for slot_idx in range(start, min(start + o.duration, len(time_ids))):
            slot = time_ids[slot_idx]
            for _, resource_ref in o.resource_assignments:
                if resource_ref is not None:
                    resource_at_time[resource_ref][slot].append(o.event_ref)

    examples: list[str] = []
    for resource_ref, slots in resource_at_time.items():
        for slot, events in slots.items():
            if len(events) > 1 and len(examples) < max_examples:
                examples.append(
                    f"  resource {resource_ref!r} has {len(events)} events "
                    f"at time {slot!r}: {events}"
                )
    if not examples:
        return "  (no resource clashes detected)"
    return "\n".join(examples)


def build(diagnostic: Diagnostic, variant: str = VARIANT_A) -> list[dict[str, str]]:
    """Build an Ollama-style messages list for the given variant.

    Returns:
        [{"role": "system", "content": ...}, {"role": "user", "content": ...}]
    """
    feasibility_label = (
        "INFEASIBLE" if diagnostic.infeasibility > 0 else "feasible"
    )

    user_parts: list[str] = [
        "## Current solver state",
        f"Feasibility: {feasibility_label}",
        f"Cost: infeasibility={diagnostic.infeasibility}, "
        f"objective={diagnostic.objective}",
        f"Stagnant iterations: {diagnostic.stagnant_iterations}",
        "",
        "## Constraint violation breakdown",
        _constraint_summary(diagnostic),
        "",
        "## Heuristic effectiveness ranking (UCB Q-values)",
        _heuristic_ranking(diagnostic),
        "",
        "## Already existing heuristics (do NOT duplicate these)",
        _existing_signatures(diagnostic.pool),
    ]

    if variant == VARIANT_B:
        user_parts += [
            "",
            "## Concrete violation examples",
            _violation_examples(diagnostic),
        ]
    elif variant == VARIANT_RAG:
        kb_text = retrieve(
            instance=diagnostic.instance,
            infeasibility=diagnostic.infeasibility,
            constraint_costs=diagnostic.constraint_costs,
            stagnant_iterations=diagnostic.stagnant_iterations,
            prompt_variant=variant,
        )
        if kb_text:
            user_parts += [
                "",
                "## Domain knowledge — suggested repair strategies",
                kb_text,
            ]

    user_parts += [
        "",
        "## Your task",
        "Write a new `apply` function that addresses the dominant issue above.",
        "Return ONLY the raw Python function code.",
    ]

    return [
        {"role": "system", "content": _SYSTEM_PROMPT},
        {"role": "user", "content": "\n".join(user_parts)},
    ]
