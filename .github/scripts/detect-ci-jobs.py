#!/usr/bin/env python3
"""Classify changed files into the CI jobs that should run."""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import sys
from collections.abc import Iterable

# Patterns prefixed with "/" are anchored to the repository root.
# Unprefixed patterns (e.g. "*.nix") match at any depth.

CI_HYGIENE_FILES = {
    "/lefthook.yml",
    "/mise.lock",
    "/mise.toml",
    "/.github/**",
}
CI_WORKFLOW_FILES = {
    "/.github/workflows/ci.yaml",
}
DOCKER_FILES = {
    ".dockerignore",
    "Dockerfile",
    "Dockerfile.*",
    "/.github/scripts/docker-build.bash",
    "/docker/**",
}
CACHE_FILES = {
    "/.github/scripts/setup-sccache.sh",
}
APPIMAGE_FILES = {
    "/appimage/**",
    "/requirements-dev.txt",
}
NIX_FILES = {
    "*.nix",
    "/flake.lock",
    "/flake.nix",
    "/default.nix",
    "/nix/**",
}
WEB_DOC_FILES = {
    "/doc/**",
    "/web/**",
}
SHELL_SCRIPT_FILES = {
    "*.bash",
    "*.sh",
}
NON_NATIVE_FILES = CI_HYGIENE_FILES | WEB_DOC_FILES


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

def item_matches_any(path: str, patterns: Iterable[str]) -> bool:
    for pattern in patterns:
        if pattern.startswith("/"):
            if fnmatch.fnmatchcase(path, pattern[1:]):
                return True
        elif fnmatch.fnmatchcase(path, pattern) or fnmatch.fnmatchcase(path, "*/" + pattern):
            return True
    return False


def matches_any(paths: Iterable[str], patterns: Iterable[str], *, invert: bool = False) -> bool:
    return any(item_matches_any(path, patterns) ^ invert for path in paths)


def classify(paths: Iterable[str], release: bool) -> dict[str, bool | list[str]]:
    shell_scripts = [path for path in paths if item_matches_any(path, SHELL_SCRIPT_FILES)]

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

    ci_workflow = matches_any(paths, CI_WORKFLOW_FILES)
    native = ci_workflow or matches_any(paths, NON_NATIVE_FILES, invert=True)
    modifies_cache = matches_any(paths, CACHE_FILES)

    return {
        "linux": native or modifies_cache,
        "macos": native or modifies_cache,
        "docker": native or matches_any(paths, DOCKER_FILES),
        "web": ci_workflow or matches_any(paths, WEB_DOC_FILES),
        "nix": native or matches_any(paths, NIX_FILES),
        "appimage": native or modifies_cache or matches_any(paths, APPIMAGE_FILES),
        "release": False,
        "ci_hygiene": matches_any(paths, CI_HYGIENE_FILES),
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
    for job in (
        "linux",
        "macos",
        "docker",
        "web",
        "nix",
        "appimage",
        "release",
        "ci_hygiene",
    ):
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
