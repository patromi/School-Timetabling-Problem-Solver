"""Rule-based knowledge base retrieval for RAG prompt augmentation.

Maps the current solver diagnostic state to one or more instruction files
from kb/.  Retrieval is deterministic and rule-based (no embeddings), so
experiments are fully reproducible and the mechanism is easy to describe
in the thesis.

Retrieval priority:
  1. If the solution is infeasible, pick the KB entry for the constraint
     type that contributes the most hard-violation cost.
  2. If feasible but highly stagnant (> STAGNATION_HIGH_THRESHOLD iterations),
     return the diversification entry.
  3. If feasible and moderately stagnant, return the soft-improvement entry.
"""

from pathlib import Path

from src.model import Constraint, Instance

KB_DIR = Path(__file__).parent.parent.parent / "kb"

STAGNATION_HIGH_THRESHOLD = 500

# Maps XHSTT constraint type tag → kb file name.
# Listed in descending "repair priority": AvoidClashes is almost always
# the hardest constraint to fix, so it gets the first match.
_CONSTRAINT_TO_KB: list[tuple[str, str]] = [
    ("AvoidClashesConstraint", "avoid_clashes_repair.md"),
    ("AssignTimeConstraint", "assign_time_repair.md"),
    ("LimitIdleTimesConstraint", "limit_idle_times.md"),
    ("LimitBusyTimesConstraint", "limit_busy_times.md"),
    ("ClusterBusyTimesConstraint", "cluster_busy_times.md"),
    ("SpreadEventsConstraint", "spread_events.md"),
    ("LinkEventsConstraint", "link_events.md"),
    ("OrderEventsConstraint", "order_events.md"),
]


def _dominant_hard_constraint(
    instance: Instance,
    constraint_costs: tuple[int, ...],
) -> Constraint | None:
    """Return the Required constraint with the highest cost, or None."""
    best: Constraint | None = None
    best_cost = 0
    for constraint, cost in zip(instance.constraints, constraint_costs, strict=True):
        if constraint.required and cost > best_cost:
            best_cost = cost
            best = constraint
    return best


def _load(filename: str) -> str | None:
    path = KB_DIR / filename
    return path.read_text(encoding="utf-8") if path.exists() else None


def retrieve(
    instance: Instance,
    infeasibility: int,
    constraint_costs: tuple[int, ...],
    stagnant_iterations: int,
    prompt_variant: str,
) -> str:
    """Return the KB section to inject into the prompt, or empty string.

    Only called when prompt_variant == 'rag'.  Other variants ignore this.
    """
    if prompt_variant != "rag":
        return ""

    if infeasibility > 0:
        dominant = _dominant_hard_constraint(instance, constraint_costs)
        if dominant is not None:
            for constraint_type, kb_file in _CONSTRAINT_TO_KB:
                if dominant.type == constraint_type:
                    text = _load(kb_file)
                    if text:
                        return text
        # Fallback: no specific KB entry found for the dominant constraint
        fallback = _load("avoid_clashes_repair.md")
        return fallback or ""

    if stagnant_iterations >= STAGNATION_HIGH_THRESHOLD:
        text = _load("diversification.md")
        return text or ""

    text = _load("soft_improvement.md")
    return text or ""
