#!/usr/bin/env python3
"""Temporary CI investigation of the intermittent X11 reload hang."""

import os
from pathlib import Path
import runpy
import subprocess
import sys
import unittest


mode = sys.argv[1]
probe = Path("build/tests/x11-test-probe").resolve()
if mode == "ungrabbed":
    # A diagnostic control only: dropping the grab restores snapshot races,
    # so a BadWindow here is not evidence of a Conky regression.
    source = Path("tests/x11-test-probe.cc").read_text()
    source = source.replace("XGrabServer(display);", "")
    source = source.replace("XUngrabServer(display);", "")
    source_path = Path("build/ungrabbed-probe.cc")
    source_path.write_text(source)
    probe = Path("build/ungrabbed-probe").resolve()
    subprocess.run(
        ["clang++", str(source_path), "-o", str(probe), "-lX11"], check=True
    )
elif mode != "grabbed":
    raise SystemExit(f"Unknown observer: {mode}")

os.environ["CONKY_TEST_X11_XFT"] = "1"
sys.argv = ["tests/test-x11-reload.py", "build/src/conky", str(probe)]
namespace = runpy.run_path(sys.argv[0])


class DiagnosticReload(namespace["X11Reload"]):
    diagnosed = False

    def fail(self, msg=None):
        if not self.diagnosed:
            self.diagnosed = True
            print(f"\nDIAGNOSTIC FAILURE ({mode}): {msg}", flush=True)
            pids = []
            if self.process is not None and self.process.poll() is None:
                pids.append(str(self.process.pid))
            result = subprocess.run(
                ["pgrep", "-x", "Xvfb"], text=True, capture_output=True, check=False
            )
            pids.extend(result.stdout.split())
            for pid in pids:
                print(f"\nSTACKS FOR PID {pid}", flush=True)
                try:
                    subprocess.run(
                        ["sudo", "-n", "gdb", "-batch", "-ex", "set pagination off",
                         "-ex", "thread apply all bt", "-p", pid],
                        timeout=20, check=False,
                    )
                except subprocess.TimeoutExpired:
                    print("Debugger timed out", flush=True)
        super().fail(msg)


suite = unittest.TestSuite(
    DiagnosticReload("test_console_retires_window_and_x11_can_restart")
    for _ in range(50)
)
result = unittest.TextTestRunner(verbosity=2, failfast=True).run(suite)
print(f"DIAGNOSTIC RESULT {mode}: success={result.wasSuccessful()}", flush=True)
# Always run the other control; this workflow gathers evidence, not a fix gate.
