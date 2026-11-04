[Русский оригинал](../ru/BUILD.md)

# Build

See [README](../../README.md) for Qt dependencies and the normal CMake commands. CTest builds and checks the portable runtime as well as the actual QML application using a disposable offline simulator.

The `platform/` directory is a Git submodule of
[Flint IgniteSDK](https://github.com/DeeTech-Labs/Flint-IgniteSDK), pinned to a commit.
Clone with `--recurse-submodules`, or run `git submodule update --init --recursive`
before configuring. Shared changes are made in the SDK repository and consumed by
updating the submodule pointer; no source export or copy step exists.

Compile a local Ignite with `python3 platform/tools/compile_ignite.py project.json output.ignit`, or use Ignite Studio. Source font/image assets require Pillow. The compiler validates resource paths against the selected project directory.

Use four-space indentation. Project formatting lives in `.editorconfig` and `.clang-format`. Icons, generic QML components, fonts and shaders are retained as the visual resource library.
