# Changelog

All notable changes to **FlinT Desktop** are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Application versions in `CMakeLists.txt` aim to follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

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

- Keep the interface stable during background status polling.
- Apply the device timezone when displaying its clock time.
