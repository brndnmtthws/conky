#!/bin/bash
set -euxo pipefail

DIR="$( dirname "${BASH_SOURCE[0]}" )"
# Run clang-tidy only for the lines of code which have changed.
git diff -U0 $TRAVIS_COMMIT_RANGE | \
  $DIR/clang-tidy-diff.py -p1 \
    -checks=*,-clang-analyzer-alpha.* \
    -quiet \
    -- \
    -warnings-as-errors=*,-clang-analyzer-alpha.* \
    -format-style='{BasedOnStyle: google, IndentWidth: 2}'
