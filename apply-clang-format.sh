#!/bin/bash

find . -iname "*.h" -o -iname "*.cc" -o -iname "*.hh" \
  | xargs clang-format \
    -style=file -i -fallback-style=google
