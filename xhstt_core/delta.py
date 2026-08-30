"""One-shot incremental cost evaluation.

Stateless entry point kept for callers that just want "the cost of this one
candidate, given that one"; it wraps `xhstt_core.incremental`, which is the
actual engine and the one the solver uses.

This used to be a self-contained "filtered full re-evaluation" -- decide
which constraints a move could have affected, then re-run each of them over
its whole scope, twice. That was correct but measured *slower* than plain
full evaluation on real instances, because the constraints with
instance-spanning scopes dominate evaluation time and are touched by almost
any move. `IncrementalEvaluator` narrows the unit of re-evaluation from a
constraint's whole scope to the spec's own point of application, so only the
handful of resources/events/groups that actually saw different data get
re-scored.

Note this wrapper builds state from scratch on every call, so it suits
one-off comparisons only; anything scoring a sequence of candidates should
hold an `IncrementalEvaluator` instead.
"""

from xhstt_core.cost import Cost
from xhstt_core.incremental import IncrementalEvaluator, StructuralChangeError
from xhstt_core.model import Instance, Solution


def delta_cost(
    instance: Instance,
    old_solution: Solution,
    old_cost: Cost,
    new_solution: Solution,
) -> Cost:
    """Cost of new_solution, computed incrementally from old_solution --
    numerically identical to evaluate_cost(instance, new_solution).

    Assumes new_solution was produced from old_solution by a move that
    preserves the event at every position and the number of solution events
    (every heuristic in xhstt_core.heuristics.MANUAL_HEURISTICS does), and
    raises ValueError if it wasn't.

    `old_cost` is accepted for backwards compatibility and deliberately
    ignored: the engine derives the old cost from the state it builds
    anyway, and taking the caller's word for it would silently return a
    wrong answer whenever the two disagreed.
    """
    evaluator = IncrementalEvaluator(instance, old_solution)
    try:
        transaction = evaluator.evaluate(new_solution)
    except StructuralChangeError as error:
        raise ValueError(
            "old_solution and new_solution resolve to a different number of "
            "occurrences -- delta_cost only supports moves that preserve "
            "event/split structure (see xhstt_core.heuristics.MANUAL_HEURISTICS)"
        ) from error
    return transaction.cost
