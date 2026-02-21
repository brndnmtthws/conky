---
applyTo: '.github/workflows/**/*.yml,.github/workflows/**/*.yaml,.github/scripts/**/*.sh,.github/scripts/**/*.bash,Dockerfile,tests/dockerfiles/**'
excludeAgent: 'coding-agent'
---

# Conky CI/Release Review Instructions

Prioritize signal quality and release safety.

Focus review comments on:

- Steps that could mask failures (missing `set -e`, ignored exit codes, or removed test stages).
- Drift from project CI expectations: configure, build, and run tests (`ctest --output-on-failure`) on supported matrices.
- Cache key changes that risk stale artifacts or ineffective caching.
- Changes affecting release artifacts (AppImage, Docker, checksums, signing) and versioning correctness.
- Security concerns in actions usage (unpinned third-party actions, unsafe token exposure, or over-broad permissions).

Require explicit rationale when reducing matrix coverage or disabling optional feature combinations that currently run in CI.
