# USB protocol and data capture

Transport: USB Serial/JTAG, serial port 115200, 8N1, no flow control.
The client uses asynchronous Qt Serial Port I/O. Port opening must not put the
clock into the ROM downloader; release RTS before DTR.

## Session

ASCII commands start with `@` and end with LF. Replies start with `FLINT ` and
end with LF/CRLF. Diagnostic lines may appear between replies; they are logged
but are not treated as command responses.

Connection sequence:

```text
@HELLO
FLINT HELLO 1 flint_spark
@INFO
FLINT INFO {JSON}
@STATUS
FLINT STATUS {JSON}
```

One request is in flight at a time, with a four-second timeout. A timeout,
device error, malformed reply or port loss closes the session. Reconnection is
user-controlled. Input lines are limited to 8192 bytes; oversized lines are
discarded through LF. The log retains 150 bounded lines.

Extend support through new fields and `capabilities`, not model-name checks.
Collection requires `info` and `status`; `time_sync` enables the sync action.
Unknown additional capabilities do not block connection. The existing `@TIME`
command stays compatible with the Python helper in FlinT OS.

## Device information — INFO

```json
{
  "protocol": 1,
  "id": "1cdbd4c64914",
  "model": "Spark",
  "board": "flint_spark",
  "chip": "esp32c3",
  "firmware": "0.2.0",
  "api": 1,
  "screen": {"width": 280, "height": 240},
  "capabilities": ["info", "status", "time_sync"]
}
```

This is a format example, not a live discovery result. `id` is derived from the
factory eFuse MAC; reading it does not enable the radio. `screen` describes the
configured canvas, not detected physical display hardware.

## Runtime state — STATUS

```json
{
  "uptime_ms": 123456,
  "free_heap_bytes": 168000,
  "usb_connected": true,
  "time_valid": true,
  "pc_synced": true,
  "epoch": 1792250000,
  "timezone": "Europe/Moscow",
  "active_face": "digital",
  "settings": {
    "show_seconds": 255,
    "language": 0,
    "hour_format": 0,
    "date_format": 0,
    "date_separator": 0
  }
}
```

These values illustrate the format. Epoch and uptime are integers; uptime is
64-bit and does not reset when a 32-bit millis counter wraps. `epoch` is seconds
since the Unix epoch in UTC, or `null` when `time_valid=false`. `pc_synced`
refers to the current boot and is cleared by manual time editing. Settings
codes are defined by FlinT OS.

The firmware forms a snapshot when processing STATUS. Queries do not modify
settings or write flash. The application polls every two seconds.

## Time synchronization

```text
@TIME 1792250000
FLINT TIME OK 1792250000
```

The client checks that the acknowledged epoch matches the sent value, then
requests STATUS again. Accepted range: 1700000000..4102444799. The device's
timezone is unchanged. Firmware confirms successful sync with three LED flashes.

## Export

A JSON object with schema `flint.desktop.capture/1`:

- `exported_at`: PC export time, UTC ISO 8601.
- `device`: INFO reply.
- `latest_status`: last accepted STATUS reply.
- `samples`: up to 600 objects with `received_at` (PC reception time, UTC) and `status`.

Reception time is not device measurement time. QSaveFile handles output so a
failed write does not replace an existing capture with a partial file.

Disconnecting preserves the last session for export. A new connection clears
old snapshots. Continuous disk recording is not enabled; saving is explicit.
