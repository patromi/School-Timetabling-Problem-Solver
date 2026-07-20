#!/usr/bin/env python
"""Validate git branch name against naming convention."""

import re
import subprocess
import sys


def get_branch_name() -> str:
    """Get the current git branch name."""
    try:
        branch = (
            subprocess.check_output(["git", "symbolic-ref", "--short", "HEAD"])
            .decode("utf-8")
            .strip()
        )
        return branch
    except subprocess.CalledProcessError:
        return ""


def main() -> None:
    """Validate current branch name."""
    branch = get_branch_name()
    # Bypass validation for default/common environment branches
    if not branch or branch in ["main", "master", "develop", "HEAD", "release"]:
        sys.exit(0)

    # Expected pattern: issue#<issue_number>
    # Allow optional trailing description like issue#12-some-description
    pattern = r"^issue#\d+(?:[-_a-zA-Z0-9]+)?$"
    if not re.match(pattern, branch):
        print(
            f"Error: Git branch name '{branch}' does not match "
            "the required pattern 'issue#<issue>'.",
            file=sys.stderr,
        )
        print(
            "Examples of valid branch names: 'issue#12', 'issue#45-add-solver'",
            file=sys.stderr,
        )
        sys.exit(1)


if __name__ == "__main__":
    main()
