# Repository Guidelines

## Project Structure & Module Organization

- `src/` hosts the core engine, text objects, and display backends; platform shims live under `src/data/os/`.
- `tests/` contains Catch2 suites (`test-*.cc`) plus fixtures; example assets reside in `data/`, `extras/`, and `web/`.
- `cmake/` holds build helpers; `3rdparty/` pins vendored libraries; `lua/` exposes scripting hooks reused in configuration samples.
- Build outputs stay in `build/`; clean it between toolchain switches to avoid stale configuration issues.

## Build, Test, and Development Commands

- Configure: `cmake -S . -B build -G Ninja -DMAINTAINER_MODE=ON` sets up a debug-friendly out-of-tree build aligned with CI.
- Compile: `cmake --build build` or `ninja -C build` builds all binaries and modules.
- Test: `ctest --test-dir build --output-on-failure` runs the suite; narrow focus with `./build/tests/test-conky "<test-or-tag>"` (for example `"[linux]"`) or `./build/tests/test-conky --section "<section>"`.
- Format: `cmake --build build --target clang-format` or `ninja -C build clang-format` rewrites sources; pair with `cmake --build build --target check-clang-format` in CI. If the target is missing, reconfigure with `-DCHECK_CODE_QUALITY=ON`.
- Coverage: `cmake -S . -B build -G Ninja -DMAINTAINER_MODE=ON -DCODE_COVERAGE=ON` then `cmake --build build --target test-conky-coverage-html`; HTML results are generated in the build tree under `test-conky-coverage-html/` (for example `build/tests/test-conky-coverage-html/index.html`).

## Coding Style & Naming Conventions

- Adopt Google C++ style: 2-space indent, 80-column target, left-aligned pointers (`char* ptr`), and braced initialization.
- Keep include blocks ordered: system, third-party, project. Run clang-format before committing, and avoid manual tabs.
- Mirror existing naming patterns such as `display_output_x11`, `text_object_xyz`, and the callback pattern from `update-cb.hh` (`callback<Result, Keys...>` plus `register_cb<YourCallback>`) for new components.

## Testing Guidelines

- Add tests beside peers in `tests/`, naming files `test-feature.cc` and Catch2 sections with snake_case tags.
- Wrap OS- or backend-specific assertions in the same `#ifdef` guards used by the implementation.
- Regenerate coverage after behavioral changes to confirm critical modules remain exercised.

## Commit & Pull Request Guidelines

- Prefer Conventional Commits (e.g., `fix: correct .dockerignore` or `build(deps): bump libfoo`), keeping imperative, present-tense subjects.
- Reference related issues or PRs with `(#1234)`, and keep each commit focused on one logical change.
- PRs should outline intent, note validation commands, and attach screenshots when user-visible output changes.
- Confirm `ctest`, formatting, and key runtime checks succeed before requesting review; reviewers expect merge-ready branches.

## Architecture Overview

- Register text objects via the `OBJ`, `OBJ_ARG`, `OBJ_IF`, and `OBJ_IF_ARG` macros in `src/core.cc` (`construct_text_object`) and reuse existing caching/update helpers instead of ad-hoc loops.
- Backends derive from `display_output_base`; ensure new drawing code degrades gracefully for ncurses and HTTP outputs.
- Long-running stats should use the shared `update_cb` + `conky::callback_handle` pattern to keep the UI responsive.
