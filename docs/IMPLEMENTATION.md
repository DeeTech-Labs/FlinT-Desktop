# FlinT implementation ledger

All application protocol/schema versions remain **1** during active development.
This ledger distinguishes implemented code from the broader requested product.

## Implemented; hardware checks still required

- USB and BLE discovery, HELLO/INFO/STATUS/TIME, shared command dispatcher.
- GATT indications, request fragmentation, bounded buffers and reconnect.
- Correlated `@REQ` / `FLINT RESULT` extension in protocol v1.
- Atomic settings patches, validation and storage acknowledgement in FlinT OS.
- My watch settings, profiles, settings backup/restore with review before apply.
- System/light/dark themes, adaptive navigation, overlay notifications.
- Device simulator, diagnostics, capture export, optional automatic time sync.
- Built-in face selection, Face Studio static layout and 16x16 monochrome icon,
  local library, favorites, import/export, file live reload, persistent Studio face in firmware.
- USB/simulator Companion: notes and explicit clipboard send, autonomous focus timer,
  Linux CPU/RAM metrics, find-watch LED, local event timeline and JSON export.
- Optional system tray mode keeps the desktop connection alive after window close.

## Remaining requested work

- Authenticated BLE pairing with physical confirmation, bonding, peer revocation.
- Button input hardware/event routing and FlinT Actions (media, microphone,
  applications, IDE/OBS integrations, local commands, presentations).
- PC Companion notifications, music, temperature, Windows/macOS metrics and autostart.
- Smart profiles and application/context triggers.
- Rich notes, focus history, device-to-PC clipboard and find phone.
- Custom boot image/name and brand illustrations/glyphs/onboarding.
- Pixel Lab layers, color pixel art, animation, widgets and framebuffer mirror.
- File transfer, backup of device assets, integrity/recovery, local/public gallery.
- Firmware updates, signed verification, USB/BLE OTA, recovery and rollback.
- Developer console, performance profiling, widget playground, developer SDK,
  Desktop plugin API (firmware already has a native face SDK).
- Android companion, foreground BLE connection, notifications/actions, media,
  permissions, face management and firmware updates.
- Multi-device Hub, simultaneous mobile/desktop connections and ownership policy.
- Real ESP32-S3 board profile/hardware validation; Windows/macOS device tests.
- Full accessibility audit, installers, migrations and release validation. Reduced motion is implemented.

The current firmware has no battery sensor, controllable backlight or button
mapping in its Spark board profile. Those capabilities must follow actual board
support; the app must never invent battery/brightness/button data.

The connected Spark (ESP32-C3) was updated on 2026-10-18 with an application-only flash,
after backing up its previous application and checking the partition table.
No public release or remote publishing has been performed.

## Local validation

- Qt 6.10.2 Linux application build and offscreen startup.
- UI suite: 11 checks passed (the optional hardware check is skipped by default),
  including simulator workflows and responsive layouts
  in Russian/English, all three themes and four window sizes.
- Desktop Python suite: 7 tests passed, including real serial-client integration.
- Firmware host suite: all 11 executables passed.
- Firmware tooling: 12 Python tests passed.
- ESP-IDF 6.0.2 ESP32-C3 firmware build passed; 49% of the app slot remains free.

Real Spark BLE discovery and two successive encrypted connections were verified
through the Desktop transport on Linux. HELLO/INFO/SETTINGS/TIME/STATUS passed,
as did a fragmented request and rejection of private Companion operations on BLE.
The clock was synchronized and its timezone/date preferences were preserved.
The opt-in `hardwareBleRoundTrip` test passed on the real board, including two
connections and explicit disconnect. BlueZ reported `Connected: no` afterward.
Long-running reconnect, independent power, display rendering, power use and tray
behavior still need dedicated checks. Android SDK is not installed locally.
