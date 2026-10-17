# Contributing to FlinT Desktop

Thanks for helping improve **FlinT Desktop**, the companion for **FlinT** devices
running **FlinT OS**, by **DeeTech Labs**.

## Before you start

- Read [README](README.md), [architecture](docs/ARCHITECTURE.md) and the
  [USB protocol](docs/USB-DATA.md).
- Check [CHANGELOG.md](CHANGELOG.md) for development notes.
- Questions and ideas → [Discussions](https://github.com/DeeTech-Labs/FlinT-Desktop/discussions).
- Bugs and concrete features → [Issues](https://github.com/DeeTech-Labs/FlinT-Desktop/issues).
- Support routing → [SUPPORT.md](SUPPORT.md).

## Development setup

Install Qt 6.5+ (Quick, Quick Controls, Quick Dialogs, Serial Port), CMake 3.21+,
a C++17 compiler and Python 3 for tests. See [BUILD.md](docs/BUILD.md).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
python3 -m unittest discover -s tests -v
```

Tests use virtual serial ports on POSIX and do not need physical hardware.
The GitHub workflow builds with Qt 6.10.2 on Linux, macOS and Windows. It runs
UI and catalog checks on all three platforms, and serial-emulator tests on
Linux/macOS, for PRs and pushes to `main`. Run hardware checks separately when changing USB behavior.

## Code organization

- QML owns presentation; `src/device/` owns transport, protocol and session data.
- Keep serial I/O asynchronous and the UI thread responsive.
- Match complete protocol replies, handle fragmented input and preserve input bounds.
- Use capabilities from INFO; do not gate features on a hard-coded model name.
- Keep HELLO/TIME compatibility with FlinT OS's Python helper.
- Document protocol changes in both repositories and test failure behavior.
- Keep generated builds, device captures and local Qt Creator settings out of commits.

## Branching and PRs

- Do not push directly to `main`.
- Branch from current `main`: `feature/…`, `fix/…`, `chore/…`, `docs/…`.
- Open a small, reviewable PR. Explain behavior, compatibility and verification.
- Changes to firmware belong in [FlinT OS](https://github.com/DeeTech-Labs/FlinT-OS).

## Commit messages

Use English imperative [Conventional Commits](https://www.conventionalcommits.org/):

```text
feat(usb): Add device capability discovery
fix(session): Reject incomplete status snapshots
docs: Clarify Qt Creator setup
```

## Naming and license

**FlinT Desktop** (application), **FlinT OS** (firmware), **FlinT Spark** (first
device), **FlinT** (product line), **DeeTech Labs** (organization).

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
Participation follows the [Code of Conduct](CODE_OF_CONDUCT.md).
