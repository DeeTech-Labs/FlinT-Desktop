# FlinT Desktop architecture

Companion for **FlinT** devices running **FlinT OS**, by **DeeTech Labs**.

## Layers

```text
Qt Quick / QML interface
  → DeviceSession (QObject properties and actions)
    → USB Serial / BLE transport / isolated simulator
      → FlinT OS protocol v1 commands and JSON replies
```

`src/main.cpp` selects the graphical application or command-line probe.
`qml/Main.qml` only instantiates the application window. The UI is split into:

- `qml/Theme.qml`: singleton with color roles, fonts, breakpoints and shared tokens.
- `qml/shell/`: window, responsive navigation, history and export dialog. Theme/language controls live in Desktop settings.
- `qml/components/`: reusable controls, cards, page scrolling and metrics. These
  components do not depend on a global USB device or application window.
- `qml/features/device/`: overview, connection, telemetry and device details.
- `qml/features/diagnostics/`: diagnostic page and log card.
- `qml/models/DevicePresentation.qml`: formatting device data for display.

Feature components receive `session` and presentation objects explicitly;
export requests travel through signals to the shell. No QML component parses
serial frames. Grid columns follow the available card width; narrow windows
use top navigation and stacked connection controls. Pages scroll independently.
All Controls use Qt's Basic style for consistent custom styling on Linux,
macOS and Windows; fonts follow the platform, and Qt handles display scaling.

`src/localization/Localization.*` installs a JSON-backed QTranslator. Catalogs
are split by page under `translations/ru/` and `translations/en/`, following
PixelStudio's contexts and Rebit's domain-pack layout. See [LOCALIZATION.md](LOCALIZATION.md).
Custom translations use QStandardPaths and language choice uses QSettings.

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
existing model logic. Settings, profiles, backups, face selection and Studio installation are capability-driven services. Firmware flashing is not offered in the application UI.

## Session data

The newest 600 accepted status snapshots are held in memory. Disconnect keeps
the last session available for inspection/export. Starting a new connection
clears the previous session. The USB log retains 150 bounded lines.

Each snapshot has a PC reception timestamp in UTC, distinct from the device's
UTC epoch and uptime. Export uses QSaveFile; partial writes do not replace an
existing capture. Captures are saved only on request and never uploaded. Preferences, profiles, Studio designs and a bounded Companion timeline persist locally.

Full command, field and capture definitions: [USB-DATA.md](USB-DATA.md).
Tests: [BUILD.md](BUILD.md#tests). Firmware:
[DeeTech-Labs/FlinT-OS](https://github.com/DeeTech-Labs/FlinT-OS).

### Automatic USB connection

The GUI enables discovery at startup; `--probe` only opens its explicit port.
Every second the session refreshes USB ports. Spark candidates use Espressif
native USB Serial/JTAG (VID `303a`, PID `1001`). A candidate must complete the
existing HELLO/INFO protocol validation before it becomes connected; a USB ID
alone does not identify a FlinT OS device. Other adapters can be selected manually.
Failed or occupied ports have a 15-second retry delay, allowing other candidates
to be checked. Unplugging removes that delay and the next scan detects reattachment,
including a changed port name. The GUI never switches away from an active device.
Disconnect also pauses automatic connection until the checkbox is enabled again.
Discovery runs only while the application is open, without a background service.
Failed handshakes preserve the previous capture; a verified new session starts a
new capture. The discovery port provider is injectable for deterministic tests.

## Companion transport and appearance (0.2)

`BleTransport` owns discovery, controller/service lifecycle, indication subscription
and ATT writes. `DeviceSession` feeds USB bytes and BLE fragments into the same
bounded line parser, handshake, capability validation and capture model. Connection
attempts are cancellable before HELLO; selecting BLE retains that endpoint for
session reconnect. Explicit disconnect disables automatic retries. `transport`
reports the selected channel, while STATUS `usb_connected` remains a device field.

`Theme` stores the system/light/dark preference in application settings. The overview
contains the clock preview, connection controls and quick actions. Technical metrics
remain on Clock data. Notifications use an overlay popup, outside page layout.
The preview is a local digital representation extrapolated between status samples,
not a framebuffer mirror or a renderer for arbitrary watch faces.

## Device services

`WatchSettings` owns the settings draft, device acknowledgements, profiles and
settings backup files. Invalid patches are rejected before firmware mutation;
failed persistence is an error, and restore prepares a draft for review.
`FaceStudio` validates local templates and maintains their library/favorites;
installation and selection each require a correlated device response.
`SimulatedDevice` is a local protocol peer behind the same session parser. It is
explicitly labelled and cannot be selected by automatic USB discovery.

The request extension is part of **protocol v1**. All version/schema identifiers
remain 1. Capabilities gate settings and Studio features on older firmware.

`CompanionTools` serializes notes, focus and metrics with other device requests,
and keeps a bounded local event timeline. `BackgroundAgent` provides optional
system-tray lifetime management; it does not install system autostart.

## Desktop presentation

`DeviceSession.viewState` exposes Disconnected, Connecting, Connected, Simulated,
StaleData and Error. Retained telemetry is labelled offline; time on illustrations
is the last reported device value and never advances in a local timer.

The blue theme uses shared surface roles, standalone SVG assets (`assets/icons`), segmented tabs and
responsive navigation. Overview quick actions route to the actual task/tab.
SwipeView supports touch navigation; horizontal wheel gestures, mouse Back/Forward
and keyboard history shortcuts are handled in the shell. Vertical scrolling stays
with the page. Page changes have no forced crossfade.

Profiles, face lists, Studio library, mappings and timeline have dedicated change
signals. Background status/event polling must not reset UI models. The regression
suite checks model stability across polling cycles.

BLE ownership uses numeric comparison with USB serial emulating absent hardware
buttons. Bonded/authenticated sessions enable Companion and Actions; trust changes
and emulated watch input remain serial-only. See COMPANION-PROTOCOL.md.

Android/mobile is explicitly deferred by the user; a future client is planned in Qt.


### Watch screen truth

`WatchScreen` downloads committed scenes into a bounded buffer, validates the
renderer identity, reconstructs RGB565 with the same portable firmware rasterizer,
and exposes raw QImages through a QML image provider. `WatchPreview` only displays
those frames and their age. It never reconstructs a face from STATUS timestamps.
Pixel capture is a separate USB diagnostic transaction. The editor also uses the
shared Studio builder for its explicitly labeled sample-value draft.

Settings rebase local edits against fresh device values, keeping edits made while
an apply request is in flight. Drafts belong to the device ID and are not applied
to another device. The global pending strip remains visible across page changes.
