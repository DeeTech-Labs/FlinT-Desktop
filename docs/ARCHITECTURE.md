# FlinT Desktop architecture

Companion for **FlinT** devices running **FlinT OS**, by **DeeTech Labs**.

## Layers

```text
Qt Quick / QML interface
  → DeviceSession (QObject properties and actions)
    → Qt Serial Port (asynchronous reads, writes and errors)
      → FlinT OS USB commands and JSON replies
```

`src/main.cpp` selects the graphical application or command-line probe.
`qml/Main.qml` presents ports, connection state, device information, telemetry,
time synchronization, file export and diagnostics. It does not parse serial frames.

`src/device/DeviceSession.*` owns the port, protocol state, timers, reply
validation, bounded logs, snapshots and atomic JSON export. The same service is
used by the graphical interface and `--probe`, so transport tests exercise it directly.

## Connection and requests

Connect → HELLO → INFO → first STATUS → poll every two seconds.
The client keeps one request in flight, with a four-second response timeout.
Diagnostic lines may arrive between fragmented replies. Input lines are bounded
to 8192 bytes; oversized lines are discarded through LF. Failures close the port
and show an error; the user controls reconnection.

Features depend on INFO capabilities, not model-name switches. `info` and
`status` are required for collection; `time_sync` enables the time-sync action.
Additional capabilities and metadata can support future models without changing
existing model logic. Current functionality does not edit device settings or flash firmware.

## Session data

The newest 600 accepted status snapshots are held in memory. Disconnect keeps
the last session available for inspection/export. Starting a new connection
clears the previous session. The USB log retains 150 bounded lines.

Each snapshot has a PC reception timestamp in UTC, distinct from the device's
UTC epoch and uptime. Export uses QSaveFile; partial writes do not replace an
existing capture. Nothing is uploaded or saved automatically.

Full command, field and capture definitions: [USB-DATA.md](USB-DATA.md).
Tests: [BUILD.md](BUILD.md#tests). Firmware:
[DeeTech-Labs/FlinT-OS](https://github.com/DeeTech-Labs/FlinT-OS).
