"""Utility: synchronize Git LFS pointers in generated/ at program startup.

Called once at the start of run_solver.py and experiments.py.  Silently
does nothing when git-lfs is unavailable, the directory has no pointers,
or the network is unreachable -- startup must never block on this.
"""

import subprocess
import sys
from pathlib import Path

_REPO_ROOT = Path(__file__).parent.parent
_LFS_DIRS = ["generated/replay", "generated"]


def pull_generated(*, verbose: bool = False) -> bool:
    """Run `git lfs pull --include generated/` from the repo root.

    Returns True when LFS reported success, False on any failure.
    Pass verbose=True to print what happens.
    """
    if not _lfs_available():
        return False

    if not _has_lfs_pointers():
        return True  # nothing to fetch

    include = ",".join(_LFS_DIRS)
    cmd = ["git", "lfs", "pull", f"--include={include}"]
    if verbose:
        print(f"[lfs] git lfs pull --include={include} ...", flush=True)
    try:
        result = subprocess.run(
            cmd,
            cwd=_REPO_ROOT,
            capture_output=not verbose,
            timeout=30,
        )
        success = result.returncode == 0
        if verbose and not success:
            print(f"[lfs] git lfs pull zakończony kodem {result.returncode}", file=sys.stderr)
        return success
    except (OSError, subprocess.TimeoutExpired):
        return False


def _lfs_available() -> bool:
    try:
        subprocess.run(
            ["git", "lfs", "version"],
            capture_output=True,
            timeout=5,
            check=True,
        )
        return True
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return False


def _has_lfs_pointers() -> bool:
    """Return True if any tracked file in generated/ is an LFS pointer
    (i.e. the content starts with 'version https://git-lfs.github.com/')."""
    for pattern in ("generated/replay/*.jsonl", "generated/*.py"):
        for path in _REPO_ROOT.glob(pattern):
            try:
                with path.open("rb") as _f:
                    header = _f.read(64)
                if header.startswith(b"version https://git-lfs.github.com/"):
                    return True
            except OSError:
                pass
    return False
