#!/usr/bin/env python3
"""Validate that all upstream CI jobs succeeded or were intentionally skipped."""

from __future__ import annotations

import argparse
import sys

ALLOWED_RESULTS = {"success", "skipped"}


def parse_job_result(value: str) -> tuple[str, str]:
    job, separator, result = value.partition("=")
    if not separator or not job or not result:
        msg = f"expected JOB=RESULT, got {value!r}"
        raise argparse.ArgumentTypeError(msg)
    return job, result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("results", nargs="+", type=parse_job_result)
    args = parser.parse_args()

    failed = False
    for job, result in args.results:
        if result in ALLOWED_RESULTS:
            print(f"{job}: {result}")
            continue

        print(f"::error::{job} finished with status {result}")
        failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
