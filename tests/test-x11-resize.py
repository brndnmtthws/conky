#!/usr/bin/env python3
"""Check content sizing after obsolete notifications on an isolated X server.

Set CONKY_TEST_X11_DISPLAY to an isolated Xvfb DISPLAY, never a desktop.
Usage: test-x11-resize.py /path/to/conky /path/to/x11-test-probe
"""

import json
import os
import re
import select
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

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
        self.write_content("HEADER")
        self.log_path = self.path / "conky.log"
        self.log = self.log_path.open("w", encoding="utf-8")
        self.addCleanup(self.log.close)
        self.process = None
        self.wm = None
        self.wm_buffer = b""

    def write_content(self, content):
        # Conky reads this file concurrently: never expose the empty interval
        # between truncation and writing the replacement content.
        temporary = self.content.with_suffix(".tmp")
        temporary.write_text(content)
        temporary.replace(self.content)

    def test_content_updates_are_atomic(self):
        def write_while_reading(path, content):
            with path.open("w") as stream:
                self.assertEqual(self.content.read_text(), "HEADER")
                return stream.write(content)

        with mock.patch.object(Path, "write_text", write_while_reading):
            self.write_content("UPDATED")
        self.assertEqual(self.content.read_text(), "UPDATED")

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

    def start(self, window_type="desktop", alignment="top_left", wait=True,
              settings=""):
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
            "gap_x=30, gap_y=30, draw_shades=false, "
            f"{settings}}};\n"
            f"conky.text = [[${{cat {self.content}}}]];\n"
        )
        self.process = subprocess.Popen(
            [BINARY, "-DD", "-c", str(config)], env=ENVIRONMENT,
            stdout=self.log, stderr=subprocess.STDOUT,
        )
        self.addCleanup(self.stop)
        return self.wait_for(
            lambda window: not wait or window["width"] >= 200
            or alignment == "none"
        )

    def start_wm(self):
        self.wm = subprocess.Popen(
            [PROBE, "--wm"], env=ENVIRONMENT, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=self.log, bufsize=0,
        )
        self.addCleanup(self.stop_wm)
        self.assertEqual(self.wm_event(), {"event": "ready"})

    def stop_wm(self):
        if self.wm.poll() is None:
            self.wm.stdin.close()
            try:
                self.wm.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.wm.kill()
                self.wm.wait(timeout=5)
        self.wm.stdin.close()
        self.wm.stdout.close()

    def wm_event(self, timeout=5):
        deadline = time.monotonic() + timeout
        while b"\n" not in self.wm_buffer:
            self.assertIsNone(self.wm.poll(), self.log_path.read_text())
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            readable, _, _ = select.select([self.wm.stdout], [], [], remaining)
            if not readable:
                return None
            chunk = os.read(self.wm.stdout.fileno(), 4096)
            self.assertTrue(chunk, self.log_path.read_text())
            self.wm_buffer += chunk
        line, self.wm_buffer = self.wm_buffer.split(b"\n", 1)
        return json.loads(line)

    def wm_request(self):
        event = self.wm_event()
        self.assertIsNotNone(event, self.log_path.read_text())
        self.assertEqual(event["event"], "request", event)
        return event

    def wm_apply(self, command="apply"):
        self.wm.stdin.write((command + "\n").encode())
        self.wm.stdin.flush()
        self.assertEqual(self.wm_event(), {"event": "applied", "errors": 0})

    def check_no_wm_resize(self, message):
        # Fixed sizing still permits alignment moves. Honor those rather than
        # confusing a legitimate position adjustment with another resize.
        deadline = time.monotonic() + 0.3
        while time.monotonic() < deadline:
            event = self.wm_event(deadline - time.monotonic())
            if event is None:
                return
            self.assertEqual(event["event"], "request", event)
            self.assertFalse(event["resizes"], message)
            self.wm_apply()

    def wait_for_measured_growth(self, initial_height):
        # Do not mistake a slow or stalled process for successful coalescing.
        # Wait until normal content updates have measured the larger content.
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            self.assertIsNone(self.process.poll(), self.log_path.read_text())
            sizes = re.findall(r"computed text size: \(\d+, (\d+)\)",
                               self.log_path.read_text())
            if sizes and int(sizes[-1]) > initial_height:
                return
            time.sleep(0.025)
        self.fail("content was not measured while resize was pending\n"
                  + self.log_path.read_text())

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
        self.write_content("HEADER\nNETWORK\nDownload\nUpload\nTotal")
        grown = self.wait_for(lambda w: w["height"] > initial["height"])
        self.assertEqual(grown["width"], initial["width"])
        self.probe("--notify", grown["id"], initial["width"], initial["height"])
        self.write_content("HEADER")
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
        self.write_content("HEADER\n" + "CONTENT\n" * 10)
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

    def test_initial_noop_resize_does_not_block_growth(self):
        self.write_content("")
        initial = self.start(
            "utility", wait=False,
            settings="minimum_width=0, minimum_height=1, gap_x=0, gap_y=0, "
            "border_width=0, border_inner_margin=0, border_outer_margin=0",
        )
        self.assertEqual((initial["width"], initial["height"]), (1, 1))
        # With no WM and no geometry change, the server will not emit a
        # ConfigureNotify. Wait for the actual no-op path, not a fixed delay.
        deadline = time.monotonic() + 5
        marker = "[main_loop] computed text size: (1, 1)"
        while time.monotonic() < deadline:
            self.assertIsNone(self.process.poll(), self.log_path.read_text())
            if marker in self.log_path.read_text():
                break
            time.sleep(0.025)
        else:
            self.fail("initial no-op was not measured\n"
                      + self.log_path.read_text())
        self.write_content("HEADER\nNETWORK\nDownload\nUpload\nTotal")
        grown = self.wait_for(lambda w: w["width"] > 1 and w["height"] > 1)
        self.assertEqual(grown["id"], initial["id"])

    def check_redirected_resizes_are_coalesced(
            self, reload_pending=False, alignment="top_left"):
        self.start_wm()
        initial = self.start("normal", alignment, wait=False)
        first = self.wm_request()
        self.assertEqual(first["id"], initial["id"])
        self.write_content("HEADER\nNETWORK\nDownload\nUpload\nTotal")
        self.wait_for_measured_growth(first["height"])
        if reload_pending:
            marker = "initialized display output"
            initialized = self.log_path.read_text().count(marker)
            self.process.send_signal(signal.SIGUSR1)
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                self.assertIsNone(self.process.poll(), self.log_path.read_text())
                if self.log_path.read_text().count(marker) > initialized:
                    break
                time.sleep(0.025)
            else:
                self.fail("reload did not complete\n" + self.log_path.read_text())
        self.assertIsNone(self.wm_event(0.2), "a second resize overlapped the first")
        self.wm_apply()
        second = self.wm_request()
        self.assertEqual(second["id"], first["id"])
        self.assertEqual(second["width"], first["width"])
        self.assertGreater(second["height"], first["height"])
        first_geometry = self.wait_for(lambda w: w["height"] == first["height"])
        self.wm_apply()
        grown = self.wait_for(lambda w: w["height"] == second["height"])

        self.write_content("HEADER")
        shrinking = self.wm_request()
        self.assertEqual(shrinking["height"], first["height"])
        self.wm_apply()
        shrunk = self.wait_for(lambda w: w["height"] == first["height"])
        if alignment == "bottom_left":
            bottom = first_geometry["root_y"] + first_geometry["height"]
            self.assertEqual(grown["root_y"] + grown["height"], bottom)
            self.assertEqual(shrunk["root_y"] + shrunk["height"], bottom)

        # An actual user/WM resize is still authoritative after auto-sizing.
        self.wm_apply(f"resize {initial['id']} 170 50")
        self.wait_for(lambda w: (w["width"], w["height"]) == (170, 50))
        self.write_content("HEADER\n" + "CONTENT\n" * 10)
        self.check_no_wm_resize("external geometry was overridden")
        self.wait_for(lambda w: (w["width"], w["height"]) == (170, 50))

    def test_redirected_resizes_are_coalesced(self):
        self.check_redirected_resizes_are_coalesced()

    def test_bottom_aligned_redirected_resizes_are_coalesced(self):
        self.check_redirected_resizes_are_coalesced(alignment="bottom_left")

    def test_reload_preserves_pending_redirected_resize(self):
        self.check_redirected_resizes_are_coalesced(reload_pending=True)

    def test_redirected_move_finishes_before_next_resize(self):
        self.start_wm()
        initial = self.start("normal", wait=False)
        first = self.wm_request()
        self.assertEqual(first["id"], initial["id"])
        self.assertTrue(first["resizes"])
        self.wm_apply()
        original = self.wait_for(lambda w: w["height"] == first["height"])

        # Reload changes only position. Hold that move while a later content
        # update requests a larger size, so their replies cannot overlap.
        config = self.path / "conky.lua"
        config.write_text(config.read_text().replace("gap_y=30", "gap_y=60"))
        self.process.send_signal(signal.SIGUSR1)
        moving = self.wm_request()
        self.assertEqual(moving["id"], initial["id"])
        self.assertFalse(moving["resizes"])
        self.write_content("HEADER\nNETWORK\nDownload\nUpload\nTotal")
        self.wait_for_measured_growth(first["height"])
        self.assertIsNone(self.wm_event(0.2), "a resize overlapped the move")

        self.wm_apply()
        growing = self.wm_request()
        self.assertTrue(growing["resizes"])
        self.assertGreater(growing["height"], first["height"])
        moved = self.wait_for(lambda w: w["root_y"] > original["root_y"])
        self.assertEqual(moved["height"], first["height"])
        self.wm_apply()
        grown = self.wait_for(lambda w: w["height"] == growing["height"])
        self.assertEqual(grown["id"], initial["id"])
        self.assertEqual(grown["root_y"], moved["root_y"])

    def test_wm_constrained_resize_remains_fixed(self):
        self.start_wm()
        initial = self.start("normal", wait=False)
        request = self.wm_request()
        self.assertEqual(request["id"], initial["id"])
        self.wm_apply("constrain 170 50")
        self.wait_for(lambda w: (w["width"], w["height"]) == (170, 50))
        self.write_content("HEADER\n" + "CONTENT\n" * 10)
        self.check_no_wm_resize("WM constraint was overridden")
        self.wait_for(lambda w: (w["width"], w["height"]) == (170, 50))

    def test_wm_refused_resize_is_not_retried(self):
        self.start_wm()
        initial = self.start("normal", wait=False)
        request = self.wm_request()
        self.assertEqual(request["id"], initial["id"])
        self.wm_apply("reject")
        self.write_content("HEADER\n" + "CONTENT\n" * 10)
        self.check_no_wm_resize("WM refusal was overridden")
        self.wait_for(
            lambda w: (w["width"], w["height"]) ==
            (initial["width"], initial["height"])
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
