"""Exercise the real Qt serial client against a USB protocol emulator (POSIX)."""

import json
import os
from pathlib import Path
import select
import subprocess
import tempfile
import threading
import time
import unittest

if os.name == "posix":
    import pty
    import tty

ROOT = Path(__file__).resolve().parents[1]
APP = Path(os.environ.get("FLINT_DESKTOP_BIN", ROOT / "build/flint_desktop"))
if not APP.exists() and (ROOT / "build/flint_desktop.app").exists():
    APP = ROOT / "build/flint_desktop.app/Contents/MacOS/flint_desktop"


class ClockEmulator:
    def __init__(self, fault=None):
        self.master, self.slave = pty.openpty()
        tty.setraw(self.slave)
        self.path = os.ttyname(self.slave)
        self.fault = fault
        self.commands = []
        self.epoch = 1792250000
        self.synced = False
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self.run, daemon=True)

    def __enter__(self):
        self.thread.start()
        return self

    def __exit__(self, *_):
        self.stop.set()
        self.thread.join(2)
        os.close(self.master)
        os.close(self.slave)

    def write(self, line):
        data = line.encode() + b"\r\n"
        # Fragmented packets must not be mistaken for complete replies.
        for i in range(0, len(data), 13):
            os.write(self.master, data[i:i + 13])
            time.sleep(.001)

    def run(self):
        pending = b""
        while not self.stop.is_set():
            if not select.select([self.master], [], [], .1)[0]:
                continue
            pending += os.read(self.master, 4096)
            while b"\n" in pending:
                line, pending = pending.split(b"\n", 1)
                command = line.decode().strip()
                self.commands.append(command)
                self.write("[000123][hb] diagnostic message")
                if command == "@HELLO":
                    self.write("FLINT HELLO 2 future" if self.fault == "version" else "FLINT HELLO 1 flint_spark")
                elif command == "@INFO":
                    if self.fault == "legacy":
                        self.write("FLINT ERROR invalid command")
                        continue
                    self.write("FLINT INFO " + json.dumps({
                        "protocol": 1, "id": "test-device", "model": "Future Clock",
                        "board": "future_board", "chip": "esp32c3", "firmware": "0.2.0",
                        "capabilities": ["info", "status", "time_sync", "future_feature"]}))
                elif command == "@STATUS":
                    if self.fault == "timeout":
                        continue
                    if self.fault == "malformed":
                        self.write("FLINT STATUS {broken")
                        continue
                    if self.fault == "noise":
                        os.write(self.master, b"x" * 9000 + b"\n")
                    status = {"uptime_ms": 5000000000, "free_heap_bytes": 160000,
                              "usb_connected": True, "time_valid": True,
                              "pc_synced": self.synced, "epoch": self.epoch,
                              "timezone": "Europe/Moscow", "active_face": "digital",
                              "settings": {"show_seconds": 255, "language": 0}}
                    if self.fault == "incomplete":
                        del status["epoch"]
                    self.write("FLINT STATUS " + json.dumps(status))
                elif command.startswith("@TIME "):
                    self.epoch = int(command.split()[1])
                    self.synced = True
                    self.write("FLINT TIME OK " + str(self.epoch + (1 if self.fault == "ack" else 0)))


@unittest.skipUnless(os.name == "posix", "PTY transport tests require POSIX")
class SessionTests(unittest.TestCase):
    def probe(self, clock, *args):
        return subprocess.run([str(APP), "--probe", clock.path, *args], text=True,
                              capture_output=True, timeout=12,
                              env={**os.environ, "QT_QPA_PLATFORM": "offscreen", "QT_QUICK_BACKEND": "software"})

    def test_collect_future_model_with_fragmented_replies_and_logs(self):
        with ClockEmulator() as clock:
            result = self.probe(clock)
            self.assertEqual(result.returncode, 0, result.stderr)
            capture = json.loads(result.stdout)
            self.assertEqual(capture["device"]["model"], "Future Clock")
            self.assertEqual(capture["latest_status"]["uptime_ms"], 5000000000)
            self.assertEqual(len(capture["samples"]), 1)
            self.assertEqual(clock.commands, ["@HELLO", "@INFO", "@STATUS"])

    def test_sync_and_export_real_capture(self):
        with tempfile.TemporaryDirectory() as directory, ClockEmulator() as clock:
            destination = Path(directory) / "capture.json"
            result = self.probe(clock, "--sync-time", "--output", str(destination))
            self.assertEqual(result.returncode, 0, result.stderr)
            capture = json.loads(destination.read_text())
            self.assertTrue(capture["latest_status"]["pc_synced"])
            self.assertLess(abs(capture["latest_status"]["epoch"] - time.time()), 3)
            self.assertEqual(len(capture["samples"]), 2)

    def test_failures_do_not_report_success(self):
        for fault in ("version", "legacy", "malformed", "incomplete", "timeout", "ack"):
            with self.subTest(fault=fault), ClockEmulator(fault) as clock:
                result = self.probe(clock, "--sync-time")
                self.assertEqual(result.returncode, 2, (fault, result.stdout, result.stderr))
                self.assertEqual(result.stdout, "")
                self.assertTrue(result.stderr)

    def test_recover_after_oversized_diagnostic_line(self):
        with ClockEmulator("noise") as clock:
            result = self.probe(clock)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)["latest_status"]["active_face"], "digital")

    def test_failed_export_is_reported(self):
        with ClockEmulator() as clock:
            result = self.probe(clock, "--output", "/nonexistent/flint/capture.json")
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, "")

    def test_periodic_collection(self):
        with ClockEmulator() as clock:
            result = self.probe(clock, "--samples", "3")
            self.assertEqual(result.returncode, 0, result.stderr)
            capture = json.loads(result.stdout)
            self.assertEqual(len(capture["samples"]), 3)
            self.assertEqual(clock.commands.count("@STATUS"), 3)


if __name__ == "__main__":
    unittest.main()
