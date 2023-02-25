#!/usr/bin/env bash

llvm_version_suffixes=(
    ""
    -15
    150
    -14
    140
    -13
    130
    -12
    120
    -11
    110
    100
    -10
    90
    -9
)

for suffix in "${llvm_version_suffixes[@]}"; do
    llvm_cov_test="llvm-cov${suffix}"
    if hash $llvm_cov_test 2>/dev/null; then
        llvm_cov=$llvm_cov_test
        break
    fi
done

exec $llvm_cov gcov "$@"
