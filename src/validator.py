"""KHE / HSEval validator module for XHSTT timetable solutions.

Wraps Kingston's HSEval (KHE suite) to evaluate XHSTT solutions against
official competition rules (Ground Truth reference).
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Literal

from src.model import Solution, SolutionGroup
from src.xml_writer import render_archive_with_solution_groups


@dataclass(frozen=True)
class HSEvalResult:
    """Outcome of HSEval validation for an XHSTT instance solution."""

    is_valid: bool
    infeasibility: int
    objective: int
    instance_id: str = ""
    solution_group_id: str = ""
    raw_output: str = ""
    error_message: str | None = None

    @property
    def is_feasible(self) -> bool:
        """True if the solution satisfies all hard (Required) constraints."""
        return self.is_valid and self.infeasibility == 0

    @property
    def total_cost_lex(self) -> tuple[int, int]:
        """Lexicographic pair (infeasibility, objective)."""
        return (self.infeasibility, self.objective)


def _to_wsl_path(path: Path) -> str:
    """Converts a Windows Path to a WSL /mnt/<drive>/... path."""
    abs_path = path.resolve()
    drive = abs_path.drive.rstrip(":").lower()
    posix_tail = abs_path.as_posix().split(":", 1)[-1]
    return f"/mnt/{drive}{posix_tail}"


class KHEValidator:
    """Validator for XHSTT solutions using KHE's hseval."""

    def __init__(
        self,
        hseval_path: str | Path | None = None,
        backend: Literal["auto", "local", "wsl", "docker"] = "auto",
        docker_image: str = "khe-validator",
    ) -> None:
        self.backend = backend
        self.docker_image = docker_image
        self.repo_root = Path(__file__).resolve().parent.parent

        if hseval_path is not None:
            self.hseval_path = Path(hseval_path)
        else:
            self.hseval_path = self._discover_hseval_binary()

    def _discover_hseval_binary(self) -> Path:
        """Searches for compiled hseval binary in standard repo locations."""
        env_path = os.environ.get("KHE_HSEVAL_PATH")
        if env_path:
            return Path(env_path)

        candidates = [
            self.repo_root / "tools" / "khe" / "bin" / "hseval",
            self.repo_root / "tools" / "khe" / "bin" / "hseval.exe",
            self.repo_root
            / "tools"
            / "khe"
            / "khe-2025_12_04"
            / "src_hseval"
            / "hseval.cgi",
            self.repo_root
            / "downloads"
            / "khe"
            / "khe-2025_12_04"
            / "src_hseval"
            / "hseval.cgi",
        ]
        for candidate in candidates:
            if candidate.is_file():
                return candidate

        which_hseval = shutil.which("hseval")
        if which_hseval:
            return Path(which_hseval)

        # Default fallback path
        return self.repo_root / "tools" / "khe" / "bin" / "hseval"

    def _determine_effective_backend(self) -> str:
        if self.backend != "auto":
            return self.backend

        is_windows = sys.platform.startswith("win")

        if not is_windows:
            if self.hseval_path.is_file() and os.access(self.hseval_path, os.X_OK):
                return "local"
            return "docker" if shutil.which("docker") else "local"

        # On Windows:
        if self.hseval_path.suffix.lower() == ".exe" and self.hseval_path.is_file():
            return "local"

        if shutil.which("wsl") and self.hseval_path.is_file():
            return "wsl"

        if shutil.which("docker"):
            return "docker"

        return "wsl"

    def validate_file(self, xml_path: str | Path) -> HSEvalResult:
        """Runs HSEval on an XHSTT archive file with solutions."""
        target_path = Path(xml_path).resolve()
        if not target_path.exists():
            return HSEvalResult(
                is_valid=False,
                infeasibility=-1,
                objective=-1,
                error_message=f"File not found: {target_path}",
            )

        backend = self._determine_effective_backend()
        try:
            if backend == "local":
                cmd = [str(self.hseval_path), "-tc", str(target_path)]
                res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            elif backend == "wsl":
                wsl_bin = _to_wsl_path(self.hseval_path)
                wsl_xml = _to_wsl_path(target_path)
                cmd = ["wsl", wsl_bin, "-tc", wsl_xml]
                res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            elif backend == "docker":
                # Mount directory of file into container
                folder = target_path.parent
                file_name = target_path.name
                cmd = [
                    "docker",
                    "run",
                    "--rm",
                    "-v",
                    f"{folder}:/workspace",
                    self.docker_image,
                    "-tc",
                    f"/workspace/{file_name}",
                ]
                res = subprocess.run(cmd, capture_output=True, text=True, check=False)
            else:
                return HSEvalResult(
                    is_valid=False,
                    infeasibility=-1,
                    objective=-1,
                    error_message=f"Unsupported backend: {backend}",
                )

            return self._parse_hseval_output(res.stdout, res.stderr, res.returncode)

        except Exception as exc:
            return HSEvalResult(
                is_valid=False,
                infeasibility=-1,
                objective=-1,
                error_message=f"Execution error ({backend}): {exc}",
            )

    def validate_xml_string(self, xml_text: str) -> HSEvalResult:
        """Validates in-memory XML string by writing it to a temporary file."""
        tmp_dir = self.repo_root / ".tmp_val"
        tmp_dir.mkdir(exist_ok=True)
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            suffix="_soln.xml",
            dir=tmp_dir,
            delete=False,
        ) as tmp_file:
            tmp_file.write(xml_text)
            tmp_path = Path(tmp_file.name)

        try:
            return self.validate_file(tmp_path)
        finally:
            if tmp_path.exists():
                try:
                    tmp_path.unlink()
                except OSError:
                    pass

    def validate_solution(
        self,
        archive_xml: str,
        solution: Solution,
        group_id: str = "HSEvalValidationGroup",
    ) -> HSEvalResult:
        """Wraps a Solution into an XHSTT archive and validates it."""
        group = SolutionGroup(id=group_id, solutions=[solution])
        full_xml = render_archive_with_solution_groups(archive_xml, [group])
        return self.validate_xml_string(full_xml)

    def _parse_hseval_output(
        self, stdout: str, stderr: str, returncode: int
    ) -> HSEvalResult:
        combined = (stdout + "\n" + stderr).strip()

        # Look for pattern: <instance_id> & {\bf <infeas>.<obj>} \\ or similar
        # e.g.: \multicolumn{1}{l}{BR-SA-00} & {\bf 8.00135} \\
        score_match = re.search(
            r"(?:\\multicolumn\{1\}\{l\}\{)?([A-Za-z0-9_\-\.]+)\}?\s*&\s*(?:\{\\bf\s*)?([0-9]+)\.([0-9]{5})",
            stdout,
        )
        if score_match:
            instance_id = score_match.group(1)
            infeasibility = int(score_match.group(2))
            objective = int(score_match.group(3))

            # Extract solution group name if available from header
            group_match = re.search(
                r"\\multicolumn\{1\}\{c\}\{\{\\bf\s*([^}]+)\}\}", stdout
            )
            sol_group = group_match.group(1) if group_match else ""

            return HSEvalResult(
                is_valid=True,
                infeasibility=infeasibility,
                objective=objective,
                instance_id=instance_id,
                solution_group_id=sol_group,
                raw_output=combined,
            )

        # Check for actual invalid table cell: & Invalid or & {\bf Invalid}
        if re.search(r"&\s*(?:\{\\bf\s*)?Invalid\b", stdout):
            inst_match = re.search(
                r"(?:\\multicolumn\{1\}\{l\}\{)?([A-Za-z0-9_\-\.]+)\}?\s*&\s*(?:\{\\bf\s*)?Invalid",
                stdout,
            )
            inst_id = inst_match.group(1) if inst_match else ""
            return HSEvalResult(
                is_valid=False,
                infeasibility=-1,
                objective=-1,
                instance_id=inst_id,
                raw_output=combined,
                error_message="HSEval reported solution is structurally Invalid.",
            )

        # Fallback: check if return code was non-zero or error occurred
        if returncode != 0 or stderr:
            return HSEvalResult(
                is_valid=False,
                infeasibility=-1,
                objective=-1,
                raw_output=combined,
                error_message=stderr.strip()
                or f"Process exited with code {returncode}",
            )

        return HSEvalResult(
            is_valid=False,
            infeasibility=-1,
            objective=-1,
            raw_output=combined,
            error_message="Could not parse HSEval score output.",
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Validate XHSTT solution using official KHE HSEval"
    )
    parser.add_argument("xml_file", help="Path to solution XML file")
    parser.add_argument(
        "output_report",
        nargs="?",
        default=None,
        help="Optional path to save report output",
    )
    parser.add_argument(
        "--hseval-path",
        "--khe-path",
        dest="hseval_path",
        default=None,
        help="Custom path to hseval binary (default: tools/khe/bin/hseval)",
    )
    parser.add_argument(
        "--backend",
        choices=["auto", "local", "wsl", "docker"],
        default="auto",
        help="Execution backend to run hseval",
    )

    args = parser.parse_args()

    validator = KHEValidator(hseval_path=args.hseval_path, backend=args.backend)
    result = validator.validate_file(args.xml_file)

    if args.output_report:
        Path(args.output_report).parent.mkdir(parents=True, exist_ok=True)
        Path(args.output_report).write_text(result.raw_output, encoding="utf-8")

    print("=" * 60)
    print(" HSEval Validation Report")
    print("=" * 60)
    print(f"File:          {args.xml_file}")
    print(f"Valid:         {'YES' if result.is_valid else 'NO'}")
    if result.is_valid:
        print(f"Instance:      {result.instance_id}")
        print(f"Infeasibility: {result.infeasibility} (hard constraints)")
        print(f"Objective:     {result.objective} (soft constraints)")
        feasible_str = "YES (0 hard violations)" if result.is_feasible else "NO"
        print(f"Feasible:      {feasible_str}")
    else:
        print(f"Error:         {result.error_message}")
    if args.output_report:
        print(f"Report saved:  {args.output_report}")
    print("=" * 60)

    if not result.is_valid:
        sys.exit(1)


if __name__ == "__main__":
    main()
