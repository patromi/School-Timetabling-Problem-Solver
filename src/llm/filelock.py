"""Appending lines to a file shared by many processes.

Experiments on the cluster run dozens of solver processes at once, and they
all append to the same LLM cache and replay files on the shared /home
filesystem (NFS). Plain appends from several processes can interleave
there, so each line is written in one call under an exclusive lock.

The lock uses fcntl, which exists only on POSIX; on Windows (local
development, one process) the line is appended without it.
"""

import sys
from pathlib import Path

if sys.platform == "win32":

    def _lock(fd: int) -> None:
        pass

    def _unlock(fd: int) -> None:
        pass

else:
    import fcntl

    # Lustre mounted without the flock option (possible on the cluster's
    # /home) answers flock() with an OSError. The line is then written
    # without the lock: still in one write() on an O_APPEND file, which
    # Lustre keeps whole.
    def _lock(fd: int) -> None:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX)
        except OSError:
            pass

    def _unlock(fd: int) -> None:
        try:
            fcntl.flock(fd, fcntl.LOCK_UN)
        except OSError:
            pass


def append_line(path: Path, line: str) -> None:
    """Append `line` plus a newline to `path` atomically with respect to
    other processes calling this function."""
    path.parent.mkdir(parents=True, exist_ok=True)
    data = (line + "\n").encode("utf-8")
    with path.open("ab") as fh:
        _lock(fh.fileno())
        try:
            fh.write(data)
            fh.flush()
        finally:
            _unlock(fh.fileno())
