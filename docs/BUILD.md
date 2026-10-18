# Build and run — FlinT Desktop

Requirements: C++17, CMake 3.21+, Qt 6.5+ with Qt Quick, Quick Controls,
Quick Dialogs, Serial Port, Bluetooth and Widgets. Qt Test is required when `BUILD_TESTING=ON` (the default). Python 3 is required
for protocol and catalog tests. Disable tests for production-only builds with
`-DBUILD_TESTING=OFF`.

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
`cmake --install build --config Debug --prefix <staging-directory>` installs
the application and runs Qt's QML deployment script for its runtime dependencies.
Platform installers, signing and notarization remain separate release steps.

## Tests

Build first, then run the cross-platform UI tests:

```bash
ctest --test-dir build -C Debug --output-on-failure
python3 -m unittest discover -s tests -v
```

The CTest suite verifies JSON language switching, timezone formatting and all
three pages at widths 540, 760, 1024 and 1440 in both languages. It fails on QML
warnings or horizontally overflowing text. Set `FLINT_SCREENSHOT_DIR` to an
existing directory when running `flint_ui_tests` to save rendered screenshots.
The Python catalog checks also run on Windows; USB emulator tests require POSIX.

The suite uses POSIX pseudo-terminals with the real Qt executable and no device.
Set `FLINT_DESKTOP_BIN` to test a binary in a different build directory.
It covers fragmented replies, firmware logs, unknown models, invalid/incomplete
JSON, unsupported firmware, timeouts, time-sync acknowledgements, polling and export.

CI is configured with Qt 6.10.2 on Ubuntu, macOS and Windows for compilation
and offscreen UI tests. Linux/macOS also run the POSIX USB integration suite.
The test harness supplies offscreen software-rendering environment variables.
A local Linux run does not substitute for the macOS/Windows CI results.

## Hardware capture

Close serial monitors and the Python sync helper before connecting in the app.
Use a firmware build with INFO/STATUS support. Read-only probe:

```bash
./build/flint_desktop --probe /dev/ttyACM0 --samples 3 --output capture.json
```

Add `--sync-time` to explicitly set the device's UTC clock from the PC.
On a headless Linux machine, prefix the command with `QT_QPA_PLATFORM=offscreen`.
For the interactive workflow and retention limits, see [README](../README.md).

BLE builds also require Qt Bluetooth (`qtconnectivity` in the Qt installer). Linux needs a running BlueZ service and an enabled adapter. macOS bundles include a Bluetooth usage description. Windows/macOS BLE packaging and permissions still need device validation.

Bluetooth discovery is initialized on the first BLE scan, so USB-only startup
and the simulator do not initialize BlueZ. On Linux, Qt may print
`Missing CAP_NET_ADMIN permission` during BLE discovery: its privileged API for
detecting public/random addresses is unavailable. This diagnostic alone does not
mean the application has crashed; validate actual discovery and connection.
`QML debugging is enabled` is expected for a Qt Creator configuration with QML
debugging enabled. Use a build with QML debugging disabled for distribution.

Linux also links Qt DBus. BLE scanning owns a BlueZ discovery session, so it
works when another application is already scanning with a different transport
filter. Only the application's own discovery reference is released on stop.

### Optional Linux hardware check

With the selected Spark nearby and the main application disconnected, run:

```sh
FLINT_HARDWARE_BLE_ADDR=1C:DB:D4:C6:49:16 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/Desktop-Debug/flint_ui_tests hardwareBleRoundTrip
```

Replace the address with your device's BLE address. This explicit opt-in check
reads settings, synchronizes the real clock, verifies that private Companion
requests are rejected over BLE, and disconnects/reconnects twice. It does not
flash firmware. Normal test runs skip hardware access.
