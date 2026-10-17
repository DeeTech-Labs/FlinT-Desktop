# Build and run — FlinT Desktop

Requirements: C++17, CMake 3.21+, Qt 6.5+ with Qt Quick, Quick Controls,
Quick Dialogs and Serial Port. Python 3 is required for integration tests.

## Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/flint_desktop
```

For a non-system Qt, pass `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64` at configure
time, adjusted for the Qt kit on your platform. The commands above use Linux
and a single-configuration generator.

Qt Creator: open root `CMakeLists.txt`, choose a Qt6 kit, configure, build and run.
Keep user kit settings and build directories untracked.

For multi-configuration generators, use `cmake --build build --config Debug`.
Windows executable: `build/Debug/flint_desktop.exe`; macOS bundle:
`build/flint_desktop.app` (the configuration subdirectory depends on the generator).
This repository does not yet produce standalone deployment packages.

## Tests

Build first, then run on Linux/macOS:

```bash
python3 -m unittest discover -s tests -v
```

The suite uses POSIX pseudo-terminals with the real Qt executable and no device.
Set `FLINT_DESKTOP_BIN` to test a binary in a different build directory.
It covers fragmented replies, firmware logs, unknown models, invalid/incomplete
JSON, unsupported firmware, timeouts, time-sync acknowledgements, polling and export.

CI uses Qt 6.10.2 and Ubuntu with an offscreen software-rendered Qt platform.
The test harness supplies these environment variables for its child processes.
Windows/macOS are target platforms; their builds are not currently CI-validated.

## Hardware capture

Close serial monitors and the Python sync helper before connecting in the app.
Use a firmware build with INFO/STATUS support. Read-only probe:

```bash
./build/flint_desktop --probe /dev/ttyACM0 --samples 3 --output capture.json
```

Add `--sync-time` to explicitly set the device's UTC clock from the PC.
On a headless Linux machine, prefix the command with `QT_QPA_PLATFORM=offscreen`.
For the interactive workflow and retention limits, see [README](../README.md).
