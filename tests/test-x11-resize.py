#!/usr/bin/env python3
"""Check content sizing after obsolete notifications on an isolated X server.

Set CONKY_TEST_X11_DISPLAY to an isolated Xvfb DISPLAY, never a desktop.
Usage: test-x11-resize.py /path/to/conky /path/to/x11-test-probe
"""

import json
import os
from pathlib import Path
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


class X11Resize(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="conky-x11-resize-")
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name)
        self.content = self.path / "content.txt"
        self.content.write_text("HEADER")
        self.log_path = self.path / "conky.log"
        self.log = self.log_path.open("w", encoding="utf-8")
        self.addCleanup(self.log.close)
        self.process = None

    def probe(self, *args):
        result = subprocess.run(
            [PROBE, *map(str, args)], env=ENVIRONMENT, text=True,
            capture_output=True, timeout=5,
        )
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        if not args:
            sample = json.loads(result.stdout)
            self.assertEqual(sample["errors"], 0)
            return sample["windows"]

    def start(self, window_type="desktop", alignment="top_left"):
        use_xft = os.environ.get("CONKY_TEST_X11_XFT") == "1"
        font = "DejaVu Sans Mono:size=8" if use_xft else "fixed"
        config = self.path / "conky.lua"
        config.write_text(
            "conky.config = {background=false, output_backend='x11', "
            "own_window=true, own_window_title='ConkyReloadTest', "
            f"own_window_type='{window_type}', alignment='{alignment}', "
            f"use_xft={str(use_xft).lower()}, font='{font}', "
            "double_buffer=true, update_interval=0.05, "
            "disable_auto_reload=true, minimum_width=200, maximum_width=200, "
            "gap_x=30, gap_y=30, draw_shades=false};\n"
            f"conky.text = [[${{cat {self.content}}}]];\n"
        )
        self.process = subprocess.Popen(
            [BINARY, "-DD", "-c", str(config)], env=ENVIRONMENT,
            stdout=self.log, stderr=subprocess.STDOUT,
        )
        self.addCleanup(self.stop)
        return self.wait_for(
            lambda window: window["width"] >= 200 or alignment == "none"
        )

    def stop(self):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)

    def wait_for(self, predicate, timeout=5):
        deadline = time.monotonic() + timeout
        windows = []
        while time.monotonic() < deadline:
            self.assertIsNone(self.process.poll(), self.log_path.read_text())
            windows = self.probe()
            if len(windows) == 1 and predicate(windows[0]):
                return windows[0]
            time.sleep(0.025)
        self.fail(f"timed out; windows={windows}\n{self.log_path.read_text()}")

    def grow_and_shrink_after_notification(self, window_type):
        initial = self.start(window_type)
        self.probe("--notify", initial["id"], 5, 5)
        # File contents change, not the config: exercise normal text updates,
        # not a reload that might reset the fixed-size latch.
        self.content.write_text("HEADER\nNETWORK\nDownload\nUpload\nTotal")
        grown = self.wait_for(lambda w: w["height"] > initial["height"])
        self.assertEqual(grown["width"], initial["width"])
        self.probe("--notify", grown["id"], initial["width"], initial["height"])
        self.content.write_text("HEADER")
        shrunk = self.wait_for(lambda w: w["height"] == initial["height"])
        self.assertEqual(shrunk["id"], initial["id"])

    def test_desktop_grows_and_shrinks_after_obsolete_notification(self):
        self.grow_and_shrink_after_notification("desktop")

    def test_normal_grows_and_shrinks_after_obsolete_notification(self):
        self.grow_and_shrink_after_notification("normal")

    def check_external_size_stays_fixed(self, alignment):
        initial = self.start("normal", alignment)
        self.probe("--resize", initial["id"], 170, 50)
        self.wait_for(lambda w: (w["width"], w["height"]) == (170, 50))
        self.content.write_text("HEADER\n" + "CONTENT\n" * 10)
        # Observe several normal update intervals; a wrongly cleared latch
        # would return to the content width and grow vertically.
        deadline = time.monotonic() + 0.5
        while time.monotonic() < deadline:
            self.assertIsNone(self.process.poll(), self.log_path.read_text())
            windows = self.probe()
            self.assertEqual(len(windows), 1)
            self.assertEqual((windows[0]["width"], windows[0]["height"]), (170, 50))
            time.sleep(0.05)

    def test_external_resize_remains_fixed(self):
        self.check_external_size_stays_fixed("top_left")

    def test_alignment_none_remains_fixed(self):
        self.check_external_size_stays_fixed("none")


if __name__ == "__main__":
    unittest.main(verbosity=2)
