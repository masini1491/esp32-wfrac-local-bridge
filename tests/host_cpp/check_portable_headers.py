#!/usr/bin/env python3
"""Compile every portable public C++ header as an isolated translation unit."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path


INCLUDE_ROOTS = (
    "components/wfrac_protocol/include",
    "components/wfrac_device_model/include",
    "components/wfrac_api/include",
    "components/wfrac_transport/include",
    "components/wfrac_client/include",
    "components/wfrac_persistence/include",
)
PER_HEADER_TIMEOUT_SECONDS = 20.0
TOTAL_TIMEOUT_SECONDS = 300.0


class HeaderCheckError(Exception):
    """An environment, discovery, or compiler failure that must fail CI."""


def enumerate_headers(root: Path) -> list[Path]:
    """Recursively enumerate regular .hpp files without hiding access errors."""
    found: list[Path] = []

    def visit(directory: Path) -> None:
        try:
            with os.scandir(directory) as scan:
                entries = sorted(scan, key=lambda entry: entry.name)
        except OSError as exc:
            raise HeaderCheckError(f"cannot read include directory {directory}: {exc}") from exc

        for entry in entries:
            entry_path = Path(entry.path)
            try:
                if entry.is_symlink() and entry.is_dir(follow_symlinks=True):
                    raise HeaderCheckError(f"symlinked include directory is unsupported: {entry_path}")
                if entry.is_dir(follow_symlinks=False):
                    visit(entry_path)
                elif entry.name.endswith(".hpp"):
                    if not entry.is_file(follow_symlinks=False):
                        raise HeaderCheckError(f"header is not a regular file: {entry_path}")
                    found.append(entry_path)
            except OSError as exc:
                raise HeaderCheckError(f"cannot inspect {entry_path}: {exc}") from exc

    visit(root)
    return found


def run_capture(command: list[str], *, timeout: float, cwd: Path) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            command,
            cwd=cwd,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
        )
    except FileNotFoundError as exc:
        raise HeaderCheckError(f"cannot start compiler command {command[0]!r}: {exc}") from exc
    except subprocess.TimeoutExpired as exc:
        raise HeaderCheckError(
            f"command timed out after {timeout:.1f}s: {command[0]}"
        ) from exc
    except OSError as exc:
        raise HeaderCheckError(f"cannot execute {command[0]!r}: {exc}") from exc


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    started = time.monotonic()

    revision_result = run_capture(
        ["git", "-C", str(repo_root), "rev-parse", "HEAD"],
        timeout=10.0,
        cwd=repo_root,
    )
    if revision_result.returncode != 0:
        raise HeaderCheckError(
            "cannot determine target revision: "
            + (revision_result.stderr.strip() or f"git exited {revision_result.returncode}")
        )
    revision = revision_result.stdout.strip()
    if len(revision) != 40 or any(char not in "0123456789abcdef" for char in revision):
        raise HeaderCheckError(f"unexpected target revision format: {revision!r}")
    print(f"TARGET_SHA={revision}", flush=True)

    compiler = shutil.which("g++")
    if compiler is None:
        raise HeaderCheckError("required compiler 'g++' was not found on PATH")
    version_result = run_capture([compiler, "--version"], timeout=10.0, cwd=repo_root)
    if version_result.returncode != 0:
        raise HeaderCheckError(
            f"compiler version query failed with exit {version_result.returncode}: "
            f"{version_result.stderr.strip()}"
        )
    version = next((line.strip() for line in version_result.stdout.splitlines() if line.strip()), "")
    if not version:
        raise HeaderCheckError("compiler returned an empty version string")
    print(f"COMPILER={compiler}", flush=True)
    print(f"COMPILER_VERSION={version}", flush=True)

    roots: list[Path] = []
    headers: list[tuple[Path, Path]] = []
    for relative_root in INCLUDE_ROOTS:
        root = repo_root / relative_root
        if not root.is_dir():
            raise HeaderCheckError(f"required include root is missing or not a directory: {relative_root}")
        try:
            with os.scandir(root) as scan:
                next(scan, None)
        except OSError as exc:
            raise HeaderCheckError(f"required include root is inaccessible: {relative_root}: {exc}") from exc
        root_headers = enumerate_headers(root)
        if not root_headers:
            raise HeaderCheckError(f"required include root contains no .hpp headers: {relative_root}")
        roots.append(root)
        headers.extend((root, header) for header in root_headers)

    if not headers:
        raise HeaderCheckError("header discovery returned no public headers")
    print(f"HEADER_COUNT={len(headers)}", flush=True)

    with tempfile.TemporaryDirectory(prefix="portable-header-check-") as temporary_directory:
        translation_unit = Path(temporary_directory) / "isolated_header.cpp"
        include_flags = [argument for root in roots for argument in ("-I", str(root))]
        for index, (owning_root, header) in enumerate(headers, start=1):
            relative_header = header.relative_to(owning_root).as_posix()
            try:
                translation_unit.write_text(
                    f"#include <{relative_header}>\n",
                    encoding="utf-8",
                    newline="\n",
                )
            except OSError as exc:
                raise HeaderCheckError(f"cannot create isolated translation unit: {exc}") from exc

            elapsed = time.monotonic() - started
            remaining = TOTAL_TIMEOUT_SECONDS - elapsed
            if remaining <= 0:
                raise HeaderCheckError(
                    f"overall {TOTAL_TIMEOUT_SECONDS:.0f}s watchdog expired before header {header}"
                )
            timeout = min(PER_HEADER_TIMEOUT_SECONDS, remaining)
            command = [
                compiler,
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsyntax-only",
                *include_flags,
                str(translation_unit),
            ]
            relative_path = header.relative_to(repo_root).as_posix()
            try:
                result = run_capture(command, timeout=timeout, cwd=repo_root)
            except HeaderCheckError as exc:
                raise HeaderCheckError(
                    f"header {index}/{len(headers)} {relative_path}: {exc}"
                ) from exc
            if result.returncode != 0:
                print(f"FAIL [{index}/{len(headers)}] {relative_path}", file=sys.stderr)
                if result.stdout:
                    print(result.stdout, file=sys.stderr, end="")
                if result.stderr:
                    print(result.stderr, file=sys.stderr, end="")
                raise HeaderCheckError(
                    f"compiler exited {result.returncode} for {relative_path}"
                )
            print(f"PASS [{index}/{len(headers)}] {relative_path}", flush=True)

    print(f"RESULT=PASS HEADERS={len(headers)}", flush=True)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HeaderCheckError as exc:
        print(f"RESULT=FAIL REASON={exc}", file=sys.stderr)
        raise SystemExit(1)
