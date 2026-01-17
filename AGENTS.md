# Repository Guidelines

## Project Structure & Module Organization
- `src/` holds the core C++ engine and subsystems (for example `Main/`, `GUI/`, `Viewer/`).
- `CMakeLists.txt`, `CMakeModules/`, and `CMakePresets.json` define build configuration and dependency discovery.
- `test_suite/` contains CppUnit-based tests grouped by category (`unit_tests/`, `system_tests/`, `simgear_tests/`).
- `scripts/`, `utils/`, and `examples/` provide tooling, utilities, and sample assets.
- `docs-mini/`, `man/`, `icons/`, `package/`, and `3rdparty/` cover docs, packaging, assets, and vendored deps.

## Build, Test, and Development Commands
- Configure with presets (creates `../fgbuild` and `../dist`): `cmake --preset configure-base`
- Build with a preset: `cmake --build --preset build-debug` or `cmake --build --preset build-release`
- Run CTest from the build tree: `ctest --test-dir ../fgbuild -C Debug` (use `RelWithDebInfo` for release)
- Build or run the C++ test suite target: `cmake --build ../fgbuild --target test_suite`
- Run TerraSync Python tests: `python -m unittest discover scripts/python/TerraSync/tests`

## Coding Style & Naming Conventions
- C++ uses `.cxx`/`.hxx` with module include paths (e.g., `#include <Main/locale.hxx>`).
- Indentation is 4 spaces; braces commonly start on the next line in existing files.
- Match the surrounding style and keep formatting-only changes minimal.
- CMake follows local patterns in the nearest `CMakeLists.txt`.

## Testing Guidelines
- Add tests under `test_suite/<category>` and register them in the appropriate `CMakeLists.txt`.
- Use descriptive suite names consistent with existing `*Tests` entries.
- Prefer CTest-driven runs so results integrate with CI.

## Commit & Pull Request Guidelines
- Commit messages are short, imperative, and may use an `area:` prefix (e.g., `tests: Harden nasal tests`).
- PRs should describe the change, link related GitLab issues, and include verification steps.
- Add screenshots for UI/visual changes and note any new flags or data files.
