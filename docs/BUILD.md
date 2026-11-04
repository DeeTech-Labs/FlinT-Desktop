# Build

See [README](../README.md) for Qt dependencies and the normal CMake commands. CTest builds and checks the portable runtime as well as the actual QML application using a disposable offline simulator.

The `platform/` directory is a synchronized export of the canonical OS implementation. From the sibling OS checkout, run `python3 tools/sync_platform.py` after editing contracts, resources, compiler tools or tests. `--check` must report no differences.

Compile a local Ignite with `python3 platform/tools/compile_ignite.py project.json output.ignit`, or use Ignite Studio. Source font/image assets require Pillow. The compiler validates resource paths against the selected project directory.

Use four-space indentation. Project formatting lives in `.editorconfig` and `.clang-format`. Icons, generic QML components, fonts and shaders are retained as the visual resource library.
