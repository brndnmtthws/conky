# Repository Guidelines

## Project Structure & Module Organization

- `src/`: core engine, text objects, display backends; `src/data/os/`: platform shims.
- `tests/`: Catch2 suites (`test-*.cc`) and fixtures; `data/`, `extras/`, `web/`: example assets.
- `cmake/`: build helpers; `3rdparty/`: pinned vendored libraries; `lua/`: scripting hooks used in config samples.
- Keep build outputs in `build/`; clean when switching toolchains to avoid stale config.

## Build, Test, and Development Commands

- Setup: `mise install` installs pinned tools from `mise.toml` (`cmake`, `ninja`, `uv`, `bun`, `lefthook`); `mise activate` runs `mise i -q` on repo enter. `mise run doctor` checks native dependencies (`mise` provides build tools; install X11, Cairo, Lua, Imlib2, librsvg, ncurses, and libxml2 via the OS package manager).
- Tasks: `mise run configure`, `mise run build`, `mise run test`, `mise run format`, and `mise run check-format` wrap CMake/Ninja. `mise run python-deps` validates Python helpers via `uv` and `requirements-dev.txt` (do not install Python directly).
- Website: `mise run web-install`, `mise run web-dev`, `mise run web-build`, and `mise run web-lint` manage `web/`. Use Bun for website and JS hook tooling (`bunx` for one-off executables).
- Configure: `cmake -S . -B build -G Ninja -DMAINTAINER_MODE=ON` (debug-friendly, out-of-tree, CI-aligned).
- Compile: `cmake --build build` or `ninja -C build`.
- Test: `ctest --test-dir build --output-on-failure`; filter with `./build/tests/test-conky "<test-or-tag>"` (e.g., `"[linux]"`) or `./build/tests/test-conky --section "<section>"`.
- Format: `cmake --build build --target clang-format` or `ninja -C build clang-format`; verify with `cmake --build build --target check-clang-format`. If missing, reconfigure with `-DCHECK_CODE_QUALITY=ON`.
- Coverage: `cmake -S . -B build -G Ninja -DMAINTAINER_MODE=ON -DCODE_COVERAGE=ON`, then `cmake --build build --target test-conky-coverage-html` (outputs HTML to `test-conky-coverage-html/`, e.g., `build/tests/test-conky-coverage-html/index.html`).

## Coding Style & Naming Conventions

- Follow Google C++ style: 2-space indent, 80-column target, left-aligned pointers (`char* ptr`), braced initialization.
- Order includes: system, third-party, project. Run `clang-format` before committing; no manual tabs.
- For new components, mirror existing naming patterns such as `display_output_x11`, `text_object_xyz`, and the `update-cb.hh` callback pattern (`callback<Result, Keys...>` + `register_cb<YourCallback>`).

## Testing Guidelines

- Add tests in `tests/` as `test-feature.cc` with `snake_case` Catch2 section tags.
- Guard OS- or backend-specific assertions with the implementation's `#ifdef`s.
- Regenerate coverage after behavioral changes to verify critical modules remain covered.

## Commit & Pull Request Guidelines

- Use Conventional Commits (e.g., `fix: correct .dockerignore`, `build(deps): bump libfoo`) with imperative, present-tense subjects.
- Keep commits to one logical change and reference issues/PRs with `(#1234)`.
- In PRs, state intent, list validation commands, and attach screenshots for user-visible output changes.
- Pass `ctest`, formatting, and key runtime checks before requesting review; branches must be merge-ready.

## Architecture Overview

- Register text objects via `OBJ`, `OBJ_ARG`, `OBJ_IF`, and `OBJ_IF_ARG` in `src/core.cc` (`construct_text_object`); reuse caching/update helpers instead of ad hoc loops.
- Derive backends from `display_output_base`; degrade new drawing code gracefully for ncurses and HTTP outputs.
- Use `update_cb` + `conky::callback_handle` for long-running stats to keep the UI responsive.
