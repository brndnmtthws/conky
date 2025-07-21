#!/usr/bin/env python3

import subprocess

def get_first_number(s):
    number = ""
    found_digit = False

    for char in s:
        if char.isdigit():
            number += char
            found_digit = True
        elif found_digit:
            break

    return int(number) if number else None


def git_log():
    entries = []
    try:
        result = subprocess.run(
            ["git", "log", "--no-merges", "--shortstat", "--pretty=format:%H %n %s"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True,
        )
        entries = result.stdout.split("\n\n")
    except subprocess.CalledProcessError as e:
        print(f"Error running git command: {e.stderr}")
        return []

    commits = []
    for line in entries:
        info = line.split("\n")
        changes = info[2].split(",")
        files_changed = get_first_number(changes[0])
        insertions = 0
        deletions = 0
        if len(changes) > 1:
            if "insertions" in changes[1]:
                insertions = get_first_number(changes[1])
            elif "deletions" in changes[1]:
                deletions = get_first_number(changes[1])
        if len(changes) > 2:
            if "insertions" in changes[2]:
                insertions = get_first_number(changes[2])
            elif "deletions" in changes[2]:
                deletions = get_first_number(changes[2])
        commit = {
            "hash": info[0].strip(),
            "message": info[1].strip(),
            "changes": {
                "files": files_changed,
                "insertions": insertions,
                "deletions": deletions,
            },
        }
        commits.append(commit)

    return commits

def main():
    log = git_log()
    if len(log) == 0:
        return

    large_changes = sorted(log, key=lambda it: it["changes"]["insertions"] + it["changes"]["deletions"], reverse=True)

    print(f"{'Hash':41} {'Insertions':11} {'Deletions':11} {'Message'}")
    print("-" * 90)
    for commit in large_changes:
        print(
            f"{commit['hash']:41} {commit["changes"]['insertions']:11} {commit["changes"]['deletions']:11} {commit['message']}"
        )


if __name__ == "__main__":
    main()
