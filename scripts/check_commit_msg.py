#!/usr/bin/env python
"""Validate git commit message against naming convention."""

import re
import sys


def main() -> None:
    """Validate commit message format."""
    if len(sys.argv) < 2:
        print("Error: Missing commit message file path.", file=sys.stderr)
        sys.exit(1)

    commit_msg_filepath = sys.argv[1]

    with open(commit_msg_filepath, encoding="utf-8") as f:
        commit_msg = f.read().strip()

    # Bypass validation for automatic merge or revert commits
    if commit_msg.startswith(("Merge ", "Revert ", "merge ")):
        sys.exit(0)

    # Expected pattern: [issue#<id>]: Opis zmian
    pattern = r"^\[issue#\d+\]:\s*.+$"
    if not re.match(pattern, commit_msg):
        print(
            "Error: Commit message does not match "
            "the required pattern '[issue#<id>]: Opis zmian'.",
            file=sys.stderr,
        )
        print(
            f"Your message was: '{commit_msg}'",
            file=sys.stderr,
        )
        print("Example: '[issue#12]: Dodanie walidatora commitow'", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
