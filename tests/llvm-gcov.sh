#!/usr/bin/env bash

if hash llvm-cov 2>/dev/null; then
    llvm_cov="llvm-cov"
elif hash llvm-cov-7 2>/dev/null; then
    llvm_cov="llvm-cov-7"
elif hash llvm-cov-8 2>/dev/null; then
    llvm_cov="llvm-cov-8"
elif hash llvm-cov70 2>/dev/null; then
    llvm_cov="llvm-cov70"
elif hash llvm-cov80 2>/dev/null; then
    llvm_cov="llvm-cov80"
fi

exec $llvm_cov gcov "$@"
