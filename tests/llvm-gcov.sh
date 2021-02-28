#!/usr/bin/env bash

if hash llvm-cov-11 2>/dev/null; then
    llvm_cov="llvm-cov-11"
elif hash llvm-cov110 2>/dev/null; then
    llvm_cov="llvm-cov110"
elif hash llvm-cov100 2>/dev/null; then
    llvm_cov="llvm-cov100"
elif hash llvm-cov-10 2>/dev/null; then
    llvm_cov="llvm-cov-10"
elif hash llvm-cov90 2>/dev/null; then
    llvm_cov="llvm-cov90"
elif hash llvm-cov-9 2>/dev/null; then
    llvm_cov="llvm-cov-9"
elif hash llvm-cov80 2>/dev/null; then
    llvm_cov="llvm-cov80"
elif hash llvm-cov-8 2>/dev/null; then
    llvm_cov="llvm-cov-8"
elif hash llvm-cov70 2>/dev/null; then
    llvm_cov="llvm-cov70"
elif hash llvm-cov-7 2>/dev/null; then
    llvm_cov="llvm-cov-7"
elif hash llvm-cov 2>/dev/null; then
    llvm_cov="llvm-cov"
fi

exec $llvm_cov gcov "$@"
