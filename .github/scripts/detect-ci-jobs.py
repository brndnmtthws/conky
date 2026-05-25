#!/usr/bin/env python3
"""Classify changed files into the CI jobs that should run."""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import sys
from collections.abc import Iterable
from pathlib import PurePosixPath

CI_HYGIENE_FILES = {
    "lefthook.yml",
    "mise.lock",
    "mise.toml",
}
CI_HYGIENE_DIRS = {
    ".github",
}
CI_WORKFLOW = ".github/workflows/ci.yaml"
DOCKER_PATTERNS = {
    ".dockerignore",
    "Dockerfile",
    "Dockerfile.*",
    ".github/scripts/docker-build.bash",
    "docker/**",
}
SCCACHE_SCRIPT = ".github/scripts/setup-sccache.sh"
APPIMAGE_PATTERNS = {
    "appimage/**",
    "requirements-dev.txt",
}
NIX_PATTERNS = {
    "flake.lock",
    "flake.nix",
    "default.nix",
    "nix/**",
    "*.nix",
}
WEB_DOC_DIRS = {
    "doc",
    "web",
}
SHELL_SCRIPT_PATTERNS = {
    "*.bash",
    "*.sh",
    "**/*.bash",
    "**/*.sh",
}


def parse_changed_files(raw: str) -> list[str]:
    try:
        value = json.loads(raw)
    except json.JSONDecodeError as error:
        msg = f"changed files must be a JSON array: {error}"
        raise argparse.ArgumentTypeError(msg) from error

    if not isinstance(value, list) or not all(isinstance(path, str) for path in value):
        msg = "changed files must be a JSON array of strings"
        raise argparse.ArgumentTypeError(msg)

    return sorted(path for path in value if path)


def first_part(path: str) -> str:
    parts = PurePosixPath(path).parts
    return parts[0] if parts else ""


def starts_with_dir(path: str, directories: Iterable[str]) -> bool:
    return any(path == directory or path.startswith(f"{directory}/") for directory in directories)


def matches_any(path: str, patterns: Iterable[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def is_ci_hygiene(path: str) -> bool:
    return path in CI_HYGIENE_FILES or starts_with_dir(path, CI_HYGIENE_DIRS)


def is_web_doc(path: str) -> bool:
    return first_part(path) in WEB_DOC_DIRS


def is_native_or_unknown(path: str) -> bool:
    if is_web_doc(path) or is_ci_hygiene(path):
        return False

    # Preserve the old paths-ignore behavior: anything that is not web/docs
    # or CI metadata should take the conservative native/package path.
    return True


def classify(paths: list[str], release: bool) -> dict[str, bool | list[str]]:
    shell_scripts = [path for path in paths if matches_any(path, SHELL_SCRIPT_PATTERNS)]

    if release:
        return {
            "linux": False,
            "macos": False,
            "docker": True,
            "web": False,
            "nix": False,
            "appimage": True,
            "release": True,
            "ci_hygiene": False,
            "shell_scripts": shell_scripts,
        }

    ci_workflow = CI_WORKFLOW in paths
    native = ci_workflow or any(is_native_or_unknown(path) for path in paths)
    sccache = SCCACHE_SCRIPT in paths

    return {
        "linux": native or sccache,
        "macos": native or sccache,
        "docker": native or any(matches_any(path, DOCKER_PATTERNS) for path in paths),
        "web": ci_workflow or any(is_web_doc(path) for path in paths),
        "nix": native or any(matches_any(path, NIX_PATTERNS) for path in paths),
        "appimage": native or sccache or any(matches_any(path, APPIMAGE_PATTERNS) for path in paths),
        "release": False,
        "ci_hygiene": any(is_ci_hygiene(path) for path in paths),
        "shell_scripts": shell_scripts,
    }


def write_output(name: str, value: str) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    if output_path is None:
        print(f"{name}={value}")
        return

    with open(output_path, "a", encoding="utf-8") as output:
        print(f"{name}={value}", file=output)


def write_outputs(classification: dict[str, bool | list[str]]) -> None:
    for job in ("linux", "macos", "docker", "web", "nix", "appimage", "release", "ci_hygiene"):
        write_output(job, str(classification[job]).lower())

    shell_scripts = classification["shell_scripts"]
    if not isinstance(shell_scripts, list):
        raise TypeError("shell_scripts classification must be a list")
    write_output("shell_scripts_json", json.dumps(shell_scripts))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--changed-files-json", type=parse_changed_files, required=True)
    parser.add_argument("--release", action="store_true")
    args = parser.parse_args()

    write_outputs(classify(args.changed_files_json, args.release))
    return 0


if __name__ == "__main__":
    sys.exit(main())
