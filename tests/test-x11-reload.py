#!/usr/bin/env python3
"""Exercise real SIGUSR1 reloads on an isolated, explicitly opted-in X server.

Set CONKY_TEST_X11_DISPLAY to an isolated Xvfb DISPLAY. Do not use a desktop:
these tests deliberately draw on the root window. For example:
  xvfb-run -a sh -c 'CONKY_TEST_X11_DISPLAY=$DISPLAY ctest --test-dir build -R x11_reload --output-on-failure'
Direct use: test-x11-reload.py /path/to/conky /path/to/x11-test-probe
"""

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest

if not os.environ.get("CONKY_TEST_X11_DISPLAY"):
    print("SKIP: set CONKY_TEST_X11_DISPLAY to an isolated test X server")
    raise SystemExit(77)
if len(sys.argv) != 3:
    raise SystemExit(__doc__)
BINARY, PROBE = [str(Path(arg).resolve()) for arg in sys.argv[1:]]
sys.argv = sys.argv[:1]
ENVIRONMENT = os.environ.copy()
ENVIRONMENT["DISPLAY"] = os.environ["CONKY_TEST_X11_DISPLAY"]
ENVIRONMENT["XDG_SESSION_TYPE"] = "x11"
ENVIRONMENT.pop("WAYLAND_DISPLAY", None)
MARKER = "CONKY RELOAD CONTENT"


class X11Reload(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="conky-x11-reload-")
        self.addCleanup(self.directory.cleanup)
        self.config = Path(self.directory.name) / "conky.lua"
        self.log_path = Path(self.directory.name) / "conky.log"
        self.log = self.log_path.open("w", encoding="utf-8")
        self.addCleanup(self.log.close)
        self.options = {
            "background": False,
            "output_backend": "x11",
            "own_window": True,
            "own_window_type": "normal",
            "own_window_title": "ConkyReloadTest",
            "own_window_color": "102030",
            "use_xft": os.environ.get("CONKY_TEST_X11_XFT") == "1",
            "font": (
                "DejaVu Sans Mono:size=20"
                if os.environ.get("CONKY_TEST_X11_XFT") == "1" else "fixed"
            ),
            "double_buffer": True,
            "update_interval": 0.05,
            "disable_auto_reload": True,
            "alignment": "top_left",
            "gap_x": 30,
            "gap_y": 30,
            "minimum_width": 360,
            "minimum_height": 100,
            "default_color": "ffffff",
            "draw_shades": False,
            "draw_outline": False,
            "draw_borders": False,
        }
        self.process = None
        self.root = self.probe()["root"]

    def probe(self):
        result = subprocess.run(
            [PROBE], env=ENVIRONMENT, text=True, capture_output=True, timeout=5
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        result = json.loads(result.stdout)
        self.assertEqual(result["errors"], 0)
        return result

    def write_config(self, **changes):
        self.options.update(changes)
        entries = []
        for key, value in self.options.items():
            literal = str(value).lower() if isinstance(value, bool) else repr(value)
            entries.append(f"{key}={literal}")
        self.config.write_text(
            "conky.config={" + ",".join(entries) + "};\n"
            + f"conky.text=[[{MARKER}\nVisible text 123456789]]\n",
            encoding="utf-8",
        )

    def start(self, **changes):
        self.write_config(**changes)
        self.process = subprocess.Popen(
            [BINARY, "-D", "-c", str(self.config)], env=ENVIRONMENT,
            stdin=subprocess.DEVNULL, stdout=self.log, stderr=subprocess.STDOUT,
        )
        self.addCleanup(self.stop)
        return self.wait_for(self.is_rendered)

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
        try:
            status = self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait()
            self.fail("Conky did not stop after SIGTERM")
        log = self.log_path.read_text(encoding="utf-8")
        self.assertEqual(status, 0, log)
        for error in ("BadWindow", "BadDrawable", "BadPixmap", "BadGC", "BadDamage"):
            self.assertNotIn(error, log)
        self.assertFalse(self.probe()["windows"], "owned XID leaked after exit")

    def is_rendered(self, sample):
        if self.options["output_backend"] == "console":
            return not sample["windows"] and MARKER in self.log_path.read_text()
        if not self.options["own_window"]:
            return not sample["windows"] and sample["root_text"]
        return (
            len(sample["windows"]) == 1
            and sample["windows"][0]["width"] >= 360
            and sample["windows"][0]["height"] >= 100
            and sample["windows"][0]["text"]
        )

    def wait_for(self, predicate, timeout=5):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.process is not None:
                self.assertIsNone(self.process.poll(), self.log_path.read_text())
            sample = self.probe()
            self.assertEqual(sample["root"], self.root, "root window was destroyed")
            if predicate(sample):
                return sample
            time.sleep(0.025)
        self.fail(f"timed out; last sample: {sample}\n{self.log_path.read_text()}")

    def reload(self, preserved_id=None, **changes):
        self.write_config(**changes)
        old_count = self.log_path.read_text().count("initialized display output")
        self.process.send_signal(signal.SIGUSR1)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            self.assertIsNone(self.process.poll(), self.log_path.read_text())
            sample = self.probe()
            self.assertEqual(sample["root"], self.root)
            if preserved_id is not None:
                self.assertEqual([w["id"] for w in sample["windows"]], [preserved_id])
                self.assertTrue(sample["windows"][0]["text"], "blank retained window")
            if self.log_path.read_text().count("initialized display output") > old_count:
                return self.wait_for(self.is_rendered)
            time.sleep(0.025)
        self.fail("reload did not finish\n" + self.log_path.read_text())

    def test_ordinary_reload_keeps_visible_xid(self):
        window = self.start()["windows"][0]["id"]
        for _ in range(5):
            sample = self.reload(preserved_id=window)
            self.assertEqual(sample["windows"][0]["id"], window)

    def test_root_to_own_renders_on_first_reload(self):
        self.start(own_window=False)
        sample = self.reload(own_window=True)
        self.assertNotEqual(sample["windows"][0]["id"], self.root)

    def test_own_to_root_removes_owned_window(self):
        self.start()
        self.reload(own_window=False)

    def test_repeated_own_root_transitions(self):
        self.start()
        for _ in range(3):
            self.reload(own_window=False)
            self.reload(own_window=True)

    def test_console_retires_window_and_x11_can_restart(self):
        self.start()
        for _ in range(2):
            self.reload(output_backend="console")
            self.reload(output_backend="x11")


if __name__ == "__main__":
    unittest.main(verbosity=2)
