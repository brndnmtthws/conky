# Repository Guidelines

## Project Structure & Module Organization

- `src/` hosts the core engine, text objects, and display backends; platform shims live under `src/data/os/`.
- `tests/` contains Catch2 suites (`test-*.cc`) plus fixtures; example assets reside in `data/`, `extras/`, and `web/`.
- `cmake/` holds build helpers; `3rdparty/` pins vendored libraries; `lua/` exposes scripting hooks reused in configuration samples.
- Build outputs stay in `build/`; clean it when switching toolchains to avoid stale configuration.

## Build, Test, and Development Commands

- Preferred setup: run `mise install` to set up the pinned developer tools from `mise.toml` (`cmake`, `ninja`, `uv`, `bun`, and `lefthook`). When `mise activate` is enabled in your shell, the repo's enter hook runs `mise i -q` automatically. Use `mise run doctor` to check system-native dependencies; `mise` provides build tools, while libraries such as X11, Cairo, Lua, Imlib2, librsvg, ncurses, and libxml2 come from the OS package manager.
- Preferred tasks: `mise run configure`, `mise run build`, `mise run test`, `mise run format`, and `mise run check-format` wrap the standard CMake/Ninja workflow. Use `mise run python-deps` to validate Python helpers via `uv` and `requirements-dev.txt` instead of installing Python directly.
- Website tasks: use `mise run web-install`, `mise run web-dev`, `mise run web-build`, and `mise run web-lint` for the `web/` project. Use Bun commands for website and JavaScript hook tooling, including `bunx` for one-off package executables.
- Configure: `cmake -S . -B build -G Ninja -DMAINTAINER_MODE=ON` sets up a debug-friendly, out-of-tree build aligned with CI.
- Compile: `cmake --build build` or `ninja -C build` builds all binaries and modules.
- Test: `ctest --test-dir build --output-on-failure` runs the suite; narrow focus with `./build/tests/test-conky "<test-or-tag>"` (e.g., `"[linux]"`) or `./build/tests/test-conky --section "<section>"`.
- Format: `cmake --build build --target clang-format` or `ninja -C build clang-format` rewrites sources; pair with `cmake --build build --target check-clang-format` in CI. If the target is missing, reconfigure with `-DCHECK_CODE_QUALITY=ON`.
- Coverage: run `cmake -S . -B build -G Ninja -DMAINTAINER_MODE=ON -DCODE_COVERAGE=ON`, then `cmake --build build --target test-conky-coverage-html`; HTML reports are generated in the build tree under `test-conky-coverage-html/` (e.g., `build/tests/test-conky-coverage-html/index.html`).

## Coding Style & Naming Conventions

- Adopt Google C++ style: 2-space indent, 80-column target, left-aligned pointers (`char* ptr`), and braced initialization.
- Keep include blocks ordered: system, third-party, project. Run `clang-format` before committing and avoid manual tabs.
- For new components, mirror existing naming patterns such as `display_output_x11`, `text_object_xyz`, and the callback pattern in `update-cb.hh` (`callback<Result, Keys...>` plus `register_cb<YourCallback>`).

## Testing Guidelines

- Add tests beside peers in `tests/`, naming files `test-feature.cc` and using `snake_case` tags for Catch2 sections.
- Wrap OS- or backend-specific assertions in the same `#ifdef` guards used by the implementation.
- Regenerate coverage after behavioral changes to confirm critical modules remain exercised.

## Commit & Pull Request Guidelines

- Prefer Conventional Commits (e.g., `fix: correct .dockerignore` or `build(deps): bump libfoo`) with imperative, present-tense subjects.
- Reference related issues or PRs with `(#1234)`, and keep each commit focused on one logical change.
- PRs should outline intent, list validation commands, and attach screenshots when user-visible output changes.
- Ensure `ctest`, formatting, and key runtime checks pass before requesting review; reviewers expect merge-ready branches.

## Architecture Overview

- Register text objects via the `OBJ`, `OBJ_ARG`, `OBJ_IF`, and `OBJ_IF_ARG` macros in `src/core.cc` (`construct_text_object`), and reuse existing caching/update helpers instead of ad hoc loops.
- Backends derive from `display_output_base`; ensure new drawing code degrades gracefully for ncurses and HTTP outputs.
- Long-running stats should use the shared `update_cb` + `conky::callback_handle` pattern to keep the UI responsive.
