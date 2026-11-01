"""Two-layer sandbox for LLM-generated heuristic code.

Layer 1 — static AST analysis (cheap, runs in-process):
  Rejects code that contains forbidden constructs before any execution.
  Whitelist of allowed top-level imports; explicit ban on dangerous builtins.

Layer 2 — subprocess isolation (safe, runs out-of-process):
  The code is executed in a fresh child process with a hard wall-clock
  timeout.  The child receives a small, controlled set of globals (model
  types + safe stdlib) and runs apply() on a dummy solution.  Any crash,
  timeout, or unexpected output causes rejection.

Both layers communicate results via a simple dict: {"ok": bool, "error": str}.
"""

import ast
import multiprocessing
import traceback
from typing import Any

from src.model import Instance, Solution

# Imports the generated function is allowed to bring in at the top level.
_ALLOWED_IMPORTS: frozenset[str] = frozenset(
    {
        "math",
        "random",
        "collections",
        "itertools",
        "dataclasses",
        "typing",
        # src.model types are injected into globals — no import needed,
        # but allow it so LLM code that writes "from src.model import ..."
        # is not rejected by the static check.  The subprocess sandbox
        # handles actual isolation.
        "src",
        "src.model",
    }
)

# AST node types whose presence is unconditionally rejected.
_FORBIDDEN_NAMES: frozenset[str] = frozenset(
    {
        "eval",
        "exec",
        "compile",
        "open",
        "__import__",
        "breakpoint",
        "input",
    }
)

_FORBIDDEN_IMPORT_ROOTS: frozenset[str] = frozenset(
    {
        "os",
        "sys",
        "subprocess",
        "socket",
        "urllib",
        "http",
        "ftplib",
        "smtplib",
        "pickle",
        "shelve",
        "importlib",
        "ctypes",
        "cffi",
        "builtins",
        "inspect",
        "gc",
        "signal",
        "threading",
        "multiprocessing",
        "concurrent",
        "asyncio",
    }
)

SUBPROCESS_TIMEOUT_S: float = 10.0


class _ForbiddenNodeVisitor(ast.NodeVisitor):
    """Walks the AST and raises SyntaxError on forbidden constructs."""

    def visit_Import(self, node: ast.Import) -> None:
        for alias in node.names:
            root = alias.name.split(".")[0]
            if root in _FORBIDDEN_IMPORT_ROOTS:
                raise SyntaxError(f"forbidden import: {alias.name!r}")
        self.generic_visit(node)

    def visit_ImportFrom(self, node: ast.ImportFrom) -> None:
        module = node.module or ""
        root = module.split(".")[0]
        if root in _FORBIDDEN_IMPORT_ROOTS:
            raise SyntaxError(f"forbidden import: {module!r}")
        self.generic_visit(node)

    def visit_Name(self, node: ast.Name) -> None:
        if node.id in _FORBIDDEN_NAMES:
            raise SyntaxError(f"forbidden name: {node.id!r}")
        self.generic_visit(node)

    def visit_Attribute(self, node: ast.Attribute) -> None:
        # Block e.g. os.system via attribute access on a forbidden module name
        if isinstance(node.value, ast.Name):
            if node.value.id in _FORBIDDEN_IMPORT_ROOTS:
                raise SyntaxError(
                    f"forbidden attribute access: {node.value.id}.{node.attr}"
                )
        self.generic_visit(node)


def static_check(code: str) -> dict[str, Any]:
    """Layer 1: parse + AST analysis. Returns {"ok": bool, "error": str}."""
    try:
        tree = ast.parse(code)
    except SyntaxError as exc:
        return {"ok": False, "error": f"syntax error: {exc}"}

    # Must define exactly one callable named 'apply'
    top_level_funcs = [
        node.name
        for node in ast.walk(tree)
        if isinstance(node, ast.FunctionDef) and node.col_offset == 0
    ]
    if "apply" not in top_level_funcs:
        return {"ok": False, "error": "no top-level function named 'apply' found"}

    try:
        _ForbiddenNodeVisitor().visit(tree)
    except SyntaxError as exc:
        return {"ok": False, "error": str(exc)}

    return {"ok": True, "error": ""}


def _subprocess_worker(
    code: str,
    instance_pickle: bytes,
    solution_pickle: bytes,
    result_queue: "multiprocessing.Queue[dict[str, Any]]",
) -> None:
    """Runs inside the child process.  Communicates via result_queue."""
    try:
        import dataclasses
        import itertools
        import math
        import pickle
        import random as random_mod
        from collections import Counter, defaultdict

        from src.evaluator_ref import valid_start_time_ids
        from src.model import (
            Instance,
            Solution,
            SolutionEvent,
            SolutionEventResource,
        )

        instance: Instance = pickle.loads(instance_pickle)
        solution: Solution = pickle.loads(solution_pickle)

        safe_globals: dict[str, Any] = {
            "__builtins__": {
                "len": len,
                "range": range,
                "enumerate": enumerate,
                "zip": zip,
                "list": list,
                "tuple": tuple,
                "dict": dict,
                "set": set,
                "frozenset": frozenset,
                "max": max,
                "min": min,
                "sum": sum,
                "abs": abs,
                "sorted": sorted,
                "reversed": reversed,
                "filter": filter,
                "map": map,
                "any": any,
                "all": all,
                "isinstance": isinstance,
                "hasattr": hasattr,
                "getattr": getattr,
                "type": type,
                "bool": bool,
                "int": int,
                "float": float,
                "str": str,
                "repr": repr,
                "print": print,
                "ValueError": ValueError,
                "TypeError": TypeError,
                "IndexError": IndexError,
                "KeyError": KeyError,
                "StopIteration": StopIteration,
                "True": True,
                "False": False,
                "None": None,
            },
            "math": math,
            "random": random_mod,
            "dataclasses": dataclasses,
            "defaultdict": defaultdict,
            "Counter": Counter,
            "itertools": itertools,
            # model types — available without import
            "Solution": Solution,
            "SolutionEvent": SolutionEvent,
            "SolutionEventResource": SolutionEventResource,
            "Instance": Instance,
            # helper exposed to LLM-generated code
            "valid_start_time_ids": valid_start_time_ids,
        }

        exec(compile(code, "<llm_heuristic>", "exec"), safe_globals)

        apply_fn = safe_globals.get("apply")
        if not callable(apply_fn):
            result_queue.put({"ok": False, "error": "apply is not callable"})
            return

        rng = random_mod.Random(42)
        result = apply_fn(solution, instance, rng)

        if not isinstance(result, Solution):
            got = type(result).__name__
            result_queue.put(
                {"ok": False, "error": f"apply returned {got}, not Solution"}
            )
            return

        event_refs = [e.event_ref for e in result.events]
        result_queue.put({"ok": True, "error": "", "event_refs": event_refs})

    except Exception:
        result_queue.put({"ok": False, "error": traceback.format_exc()})


def subprocess_run(
    code: str,
    instance: Instance,
    solution: Solution,
    timeout_s: float = SUBPROCESS_TIMEOUT_S,
) -> dict[str, Any]:
    """Layer 2: execute code in isolated subprocess with hard timeout.

    Returns {"ok": bool, "error": str, "event_refs": list[str] | None}.
    """
    import pickle

    instance_pickle = pickle.dumps(instance)
    solution_pickle = pickle.dumps(solution)

    ctx = multiprocessing.get_context("forkserver")
    q: multiprocessing.Queue[dict[str, Any]] = ctx.Queue()
    proc = ctx.Process(
        target=_subprocess_worker,
        args=(code, instance_pickle, solution_pickle, q),
        daemon=True,
    )
    proc.start()
    proc.join(timeout=timeout_s)

    if proc.is_alive():
        proc.kill()
        proc.join()
        return {"ok": False, "error": f"timeout after {timeout_s}s", "event_refs": None}

    if proc.exitcode != 0:
        return {
            "ok": False,
            "error": f"subprocess exited with code {proc.exitcode}",
            "event_refs": None,
        }

    if q.empty():
        return {"ok": False, "error": "no result from subprocess", "event_refs": None}

    return q.get_nowait()
