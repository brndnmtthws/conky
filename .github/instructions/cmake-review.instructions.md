---
applyTo: 'CMakeLists.txt,cmake/**/*.cmake,tests/CMakeLists.txt,src/CMakeLists.txt,lua/CMakeLists.txt,data/CMakeLists.txt,doc/CMakeLists.txt,extras/CMakeLists.txt,3rdparty/CMakeLists.txt'
excludeAgent: 'coding-agent'
---

# Conky Build-System Review Instructions

Prioritize build reproducibility and CI parity.

Focus review comments on:

- Option defaults and interactions (`MAINTAINER_MODE`, `BUILD_TESTING`, `CHECK_CODE_QUALITY`, `CODE_COVERAGE`) that may change expected developer or CI behavior.
- Platform/compiler conditionals that could break Linux/macOS matrix builds.
- Dependency/linking changes that risk optional feature regressions.
- New targets/commands that are not wired into existing workflows or documented usage.

Validation expectations:

- Build paths should remain compatible with `cmake . -B build -G Ninja -DMAINTAINER_MODE=ON`.
- Test execution should remain compatible with `ctest --output-on-failure` from `build/`.
- Format-related guidance should align with available targets (`clang-format`, `check-clang-format`) and their configuration preconditions.

Raise comments when CMake changes alter cache variable semantics without migration notes or clear rationale.
