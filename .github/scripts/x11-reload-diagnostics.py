#!/usr/bin/env python3
"""Temporary CI investigation of the intermittent X11 reload hang."""

import os
import json
from pathlib import Path
import runpy
import subprocess
import sys
import time
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

    def probe(self):
        if mode == "grabbed":
            return super().probe()
        # Diagnostic only: skip inconsistent snapshots, retaining the original
        # deadline and all application/log assertions. Never use this control
        # as a replacement regression test without classifying X errors.
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            result = subprocess.run(
                [str(probe)], env=namespace["ENVIRONMENT"], text=True,
                capture_output=True, timeout=5, check=False,
            )
            if result.returncode == 0:
                return json.loads(result.stdout)
            print(f"Discarding raced control snapshot: {result.stdout}", flush=True)
            time.sleep(0.025)
        self.fail("No valid ungrabbed snapshot within five seconds")

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
                commands = ["set pagination off", "thread apply all bt"]
                if self.process is not None and pid == str(self.process.pid):
                    commands += ["p *display", "p display->request",
                                 "p display->last_request_read"]
                else:
                    commands += ["p GrabInProgress", "p grabState",
                                 "p currentMaxClients", "p ready_clients",
                                 "p saved_ready_clients",
                                 "p output_pending_clients", "p NewOutputPending",
                                 "p CriticalOutputPending", "p *server_poll",
                                 "set $i=1",
                                 "while $i<currentMaxClients\n"
                                 "if clients[$i]\n"
                                 "p *clients[$i]\n"
                                 "p *(OsCommPtr)clients[$i]->osPrivate\n"
                                 "if ((OsCommPtr)clients[$i]->osPrivate)->output\n"
                                 "p *((OsCommPtr)clients[$i]->osPrivate)->output\n"
                                 "end\n"
                                 "end\nset $i=$i+1\nend"]
                try:
                    subprocess.run(
                        ["sudo", "-n", "gdb", "-batch",
                         "-iex", "set debuginfod enabled on",
                         "-iex", "set debuginfod urls https://debuginfod.ubuntu.com",
                         *[arg for cmd in commands for arg in ("-ex", cmd)],
                         "-p", pid], timeout=90, check=False,
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
