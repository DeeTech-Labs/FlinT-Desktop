# Contributing to Flint Flare

Thanks for helping improve **Flint Flare**, the companion for **FlinT** devices
running **FlinT OS**, by **DeeTech Labs**.

## Before you start

- Read [README](README.md), [architecture](docs/ARCHITECTURE.md) and the
  [Companion protocol](docs/COMPANION-PROTOCOL.md).
- Check [CHANGELOG.md](CHANGELOG.md) for development notes.
- Questions and ideas → [Discussions](https://github.com/DeeTech-Labs/Flint-Flare/discussions).
- Bugs and concrete features → [Issues](https://github.com/DeeTech-Labs/Flint-Flare/issues).
- Support routing → [SUPPORT.md](SUPPORT.md).

## Development setup

Install Qt 6.5+ (Quick, Quick Controls, Quick Dialogs, Serial Port), CMake 3.21+,
a C++20 compiler and Python 3/Pillow for compiler and integration tests. See [BUILD.md](docs/BUILD.md).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Tests use the shared Core and a disposable offline simulator; they do not need physical hardware.
The GitHub workflow builds with Qt 6.10.2 on Linux, macOS and Windows for PRs and pushes to `main`.
Run hardware checks separately when changing USB or Bluetooth behavior.

## Code organization

- QML owns presentation; retain its design system, icons and visual resources.
- `src/platform_app/` owns application services; `src/device/` owns the reusable Bluetooth lifecycle.
- `platform/` is exported from the canonical OS implementation.
- Keep serial I/O asynchronous and the UI thread responsive.
- Match complete protocol replies, handle fragmented input and preserve input bounds.
- Use capabilities from INFO; do not gate features on a hard-coded model name.
- Use the shared binary v1 handshake and development contract revision; do not add textual compatibility shims.
- Use four-space indentation without tabs.
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

**Flint Flare** (application), **FlinT OS** (firmware), **FlinT Spark** (first
device), **FlinT** (product line), **DeeTech Labs** (organization).

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
Participation follows the [Code of Conduct](CODE_OF_CONDUCT.md).
