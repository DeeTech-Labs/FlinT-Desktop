# FlinT implementation ledger

All application protocol/schema versions remain **1** during active development.
This ledger distinguishes implemented code from the broader requested product.

## Current Desktop / firmware development (2026-10-20)

- Blue interface based on the supplied reference: sidebar icons, overview actions,
  segmented task pages, separated gallery/editor and settings inspector.
- Thirty standalone SVG icons in `assets/icons`, packaged as Qt resources.
- Touch swipes, horizontal wheel navigation, mouse Back/Forward and keyboard history.
- Dedicated UI-model signals prevent resets during background status/event polling.
- Explicit device presentation states and a confirmed watch-screen preview, using
  the identical C++ scene rasterizer and font masks on firmware/Desktop.
- Small committed scene snapshots over USB/authenticated BLE, visible-page polling,
  stale-frame labels and strict renderer identity checks; no continuous image stream.
- USB-only actual-pixel PNG capture with byte-for-byte comparison and a 30-second
  pin watchdog. All six real-device face/menu checks passed before the final UI pass.
- Blue boot reveal with a small Lato brand subset, light sweep and first-face fade.
- Blue firmware Digital/Companion/menu layouts, dirty-region transfers and matching
  Studio draft rendering in the editor.
- Persistent settings tabs, per-tab scroll positions, draft preservation across
  refresh/apply/reconnect, a global apply/discard summary and USB watch keyboard input.
- Authenticated BLE numeric comparison with USB serial emulating missing buttons,
  persistent bonds, a bounded pairing window and serial-only peer revocation.
- Companion operations on authenticated/bonded BLE; serial-only input emulation.
- Session-armed Actions: Linux MPRIS media, watch focus and X11 presentation keys.
- Studio icon/time/date layers, six stacking orders, icon placement/scale/visibility
  and two-frame 500 ms animation. Original face packages remain supported.
- Mobile is explicitly out of current scope; a future implementation is planned in Qt.

## Previously implemented foundation

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

- Physical button board support; Windows/macOS Actions; microphone, application,
  IDE/OBS integrations and user-defined commands.
- PC Companion notifications, music, temperature, Windows/macOS metrics and autostart.
- Smart profiles and application/context triggers.
- Rich notes, focus history, device-to-PC clipboard and find phone.
- Custom boot image/name and brand illustrations/glyphs/onboarding.
- Arbitrary additional Studio layers, color pixel art and arbitrary widgets.
- File transfer, backup of device assets, integrity/recovery, local/public gallery.
- Firmware updates, signed verification, USB/BLE OTA, recovery and rollback.
- Developer console, performance profiling, widget playground, developer SDK,
  Desktop plugin API (firmware already has a native face SDK).
- Android companion is deferred by explicit user request (future Qt client).
- Multi-device Hub, simultaneous mobile/desktop connections and ownership policy.
- Real ESP32-S3 board profile/hardware validation; Windows/macOS device tests.
- Full accessibility audit, installers, migrations and release validation. Reduced motion is implemented.

The current firmware has no battery sensor, controllable backlight or button
mapping in its Spark board profile. Those capabilities must follow actual board
support; the app must never invent battery/brightness/button data.

The connected Spark (ESP32-C3) was updated on 2026-10-18 with an application-only flash,
after backing up its previous application and checking the partition table.
No public release or remote publishing has been performed.

## Earlier validation (2026-10-18)

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

## Current validation (2026-10-20)

- Qt 6.10.2 Linux Desktop build passes.
- UI suite: 13 passed, no failures; opt-in hardware and visual-review tests are run
  separately. Both languages, three themes, four sizes, seven main pages pass.
- Regression checks cover stable models during background polling, Actions arming,
  no replay, Studio extended packages, settings/profiles and reconnect.
- Visual review passes on the real USB watch; screenshots verify selected settings.
  Keyboard shortcuts, mouse Back/Forward and synthesized touch swipes pass.
- Desktop Python suite: 7 passed. Firmware host suite: all 11 executables pass.
- ESP-IDF 6.0.2 ESP32-C3 build passes. Updated application-only after verifying the
  existing partition table and saving a complete 4 MB flash backup locally.
- Real Spark: numeric codes matched across USB and Desktop, private Companion read,
  time sync and two BLE connections pass. Bond survives application update/reset.
  USB revocation removes the bond; fresh authenticated pairing/reconnect passes.
- Physical display appearance, long-duration power tests, Windows/macOS hardware,
  additional boards and arbitrary third-party integrations are not certified by
  these checks. No mobile application or public release was produced.
