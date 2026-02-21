---
applyTo: 'src/**/*.cc,src/**/*.hh,src/**/*.hpp,src/**/*.h,src/**/*.cpp,tests/**/*.cc,tests/**/*.hh,tests/**/*.hpp,tests/**/*.h,lua/**/*.cc,lua/**/*.hh,lua/**/*.hpp,lua/**/*.h'
excludeAgent: 'coding-agent'
---

# Conky C/C++ Review Instructions

Prioritize behavioral correctness, regressions, and runtime safety over style-only feedback.

Focus review comments on:

- Changes to text object registration (`CREATE_NODE_CALLBACK`) and update logic (`update_cb` + `conky::callback_handle`) that could break refresh cadence, caching, or responsiveness.
- Backend behavior across output modes (`display_output_base` descendants), especially graceful degradation for ncurses and HTTP outputs.
- Platform-guarded logic under `src/data/os/` and tests that should mirror implementation `#ifdef` boundaries.
- Lifetime/ownership bugs, null dereferences, unchecked conversions, out-of-bounds access, and unintended copies in hot paths.

Testing expectations:

- New behavior should have or update tests in `tests/test-*.cc`.
- Encourage focused test runs with `./build/tests/test-conky "<test-or-tag>"` and full verification with `ctest --test-dir build --output-on-failure`.
- If behavior changed but no test changed, ask for justification.

Do not request purely stylistic changes already enforced by clang-format unless readability or correctness is impacted.
