# Changelog

All notable changes to **FlinT Desktop** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Application versions in `CMakeLists.txt` aim to follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Protocol v1 request IDs, atomic device settings, local profiles and settings backup/restore.
- Protocol simulator and Face Studio with pixel icon editing, library, favorites and device installation.
- BLE discovery, GATT connection, shared INFO/STATUS/TIME commands and session reconnect.
- System/light/dark themes, larger clock preview and overlay notifications.
- Companion protocol documentation and Bluetooth build dependencies.
- USB Companion notes, clipboard send, focus timer, Linux CPU/RAM metrics and find-watch indicator.
- Local event timeline with JSON export, file live reload and optional system tray mode.

- Qt6 / C++17 / QML application with CMake and Qt Creator support.
- USB device discovery, protocol handshake and capability-based device information.
- Periodic status collection, PC time synchronization and bounded USB diagnostics.
- Manual JSON session export and command-line capture.
- Serial-emulator integration tests for real Qt client behavior and error handling.
- Shared FlinT branding, contributor documentation, issue forms and CODEOWNERS.
- Linux Qt build and test workflow for pull requests and `main`.
- Dark dashboard with reusable QML controls, responsive pages and a shared theme.
- English and Russian JSON language packs split by page, with runtime switching.
- UI, catalog and timezone tests, plus Linux/macOS/Windows build checks.

### Fixed

- Handle delayed BLE service UUIDs, use a longer scan and select the public Spark address on Linux without CAP_NET_ADMIN.
- Own a Linux BlueZ discovery reference when another Bluetooth client is already scanning.
- Request Linux pairing explicitly before encrypted GATT access.

- Keep the interface stable during background status polling.
- Apply the device timezone when displaying its clock time.
