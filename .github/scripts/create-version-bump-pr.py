#!/usr/bin/env python3
"""Create a version bump branch, commit, push it, and open a PR."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

VERSION_FILE = Path("cmake/Conky.cmake")
VERSION_PATTERN = re.compile(
    r'^set\(VERSION_(MAJOR|MINOR|PATCH) "([0-9]+)"\)$'
)


def run(
    command: list[str], *, check: bool = True
) -> subprocess.CompletedProcess[str]:
    print(f"+ {' '.join(command)}")
    return subprocess.run(command, check=check, text=True)


def capture(command: list[str], *, check: bool = True) -> str:
    result = subprocess.run(
        command,
        check=check,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return result.stdout.strip()


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    sys.exit(1)


def ensure_clean_worktree() -> None:
    status = capture(["git", "status", "--porcelain"])
    if status:
        fail("working tree is not clean; commit or stash local changes first")


def ensure_command(command: str) -> None:
    if shutil.which(command) is None:
        fail(f"required command not found: {command}")


def read_version(path: Path) -> tuple[int, int, int]:
    values: dict[str, int] = {}

    for line in path.read_text(encoding="utf-8").splitlines():
        match = VERSION_PATTERN.match(line)
        if match:
            values[match.group(1)] = int(match.group(2))

    missing = {"MAJOR", "MINOR", "PATCH"} - values.keys()
    if missing:
        names = ", ".join(sorted(missing))
        fail(f"could not find version field(s) in {path}: {names}")

    return values["MAJOR"], values["MINOR"], values["PATCH"]


def bump_version(current: tuple[int, int, int], kind: str) -> tuple[int, int, int]:
    major, minor, patch = current

    if kind == "major":
        return major + 1, 0, 0
    if kind == "minor":
        return major, minor + 1, 0
    if kind == "patch":
        return major, minor, patch + 1

    raise ValueError(f"unsupported bump kind: {kind}")


def write_version(path: Path, version: tuple[int, int, int]) -> None:
    major, minor, patch = version
    replacements = {
        "MAJOR": f'set(VERSION_MAJOR "{major}")',
        "MINOR": f'set(VERSION_MINOR "{minor}")',
        "PATCH": f'set(VERSION_PATCH "{patch}")',
    }

    lines = []
    replaced: set[str] = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        match = VERSION_PATTERN.match(line)
        if match:
            field = match.group(1)
            lines.append(replacements[field])
            replaced.add(field)
        else:
            lines.append(line)

    missing = set(replacements) - replaced
    if missing:
        names = ", ".join(sorted(missing))
        fail(f"could not update version field(s) in {path}: {names}")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def ensure_branch_available(branch: str, remote: str) -> None:
    local = run(
        ["git", "show-ref", "--verify", "--quiet", f"refs/heads/{branch}"],
        check=False,
    )
    if local.returncode == 0:
        fail(f"local branch already exists: {branch}")

    remote_branch = run(
        ["git", "ls-remote", "--exit-code", "--heads", remote, branch],
        check=False,
    )
    if remote_branch.returncode == 0:
        fail(f"remote branch already exists on {remote}: {branch}")
    if remote_branch.returncode not in {0, 2}:
        fail(f"could not check remote branch availability on {remote}")


def create_pr(branch: str, version: str, base: str, dry_run: bool) -> None:
    title = f"chore: bump version to v{version}"
    body = "\n".join(
        [
            f"Bump Conky version to v{version}.",
            "",
            "Validation:",
            "- Not run; version-only change.",
        ]
    )

    if dry_run:
        print(f"dry-run: would create PR with title: {title}")
        return

    run(
        [
            "gh",
            "pr",
            "create",
            "--base",
            base,
            "--head",
            branch,
            "--title",
            title,
            "--body",
            body,
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a Conky version bump branch, commit, push, and PR."
    )
    parser.add_argument("kind", choices=("patch", "minor", "major"))
    parser.add_argument("--base", default="main")
    parser.add_argument("--remote", default="origin")
    parser.add_argument(
        "--branch-prefix",
        default="chore/bump-version",
        help="branch prefix; the version suffix is appended automatically",
    )
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    repo_root = Path(capture(["git", "rev-parse", "--show-toplevel"]))
    if Path.cwd() != repo_root:
        fail(f"run this from the repository root: {repo_root}")

    if args.dry_run:
        current = read_version(VERSION_FILE)
        next_version = bump_version(current, args.kind)
        version = ".".join(str(part) for part in next_version)
        branch = f"{args.branch_prefix}-v{version}"
        print(f"current version: {'.'.join(str(part) for part in current)}")
        print(f"next version:    {version}")
        print(f"branch:          {branch}")
        print("dry-run: no branch, commit, push, or PR was created")
        return 0

    ensure_command("git")
    ensure_command("gh")
    ensure_clean_worktree()

    run(["git", "fetch", "--tags", args.remote, args.base])
    run(["git", "checkout", args.base])
    run(["git", "pull", "--ff-only", args.remote, args.base])
    ensure_clean_worktree()

    current = read_version(VERSION_FILE)
    next_version = bump_version(current, args.kind)
    version = ".".join(str(part) for part in next_version)
    branch = f"{args.branch_prefix}-v{version}"
    print(f"current version: {'.'.join(str(part) for part in current)}")
    print(f"next version:    {version}")
    print(f"branch:          {branch}")

    ensure_branch_available(branch, args.remote)
    run(["git", "checkout", "-b", branch])

    write_version(VERSION_FILE, next_version)
    run(["git", "add", str(VERSION_FILE)])
    run(["git", "commit", "-m", f"chore: bump version to v{version}"])
    run(["git", "push", "-u", args.remote, branch])
    create_pr(branch, version, args.base, args.dry_run)

    return 0


if __name__ == "__main__":
    sys.exit(main())
