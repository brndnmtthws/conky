#!/usr/bin/env python3
"""Temporary CI investigation of the intermittent X11 reload hang."""

import os
import shlex
import json
from pathlib import Path
import runpy
import subprocess
import sys
import time
import unittest


mode = sys.argv[1]
minimal = mode.startswith("minimal-")
mode = mode.removeprefix("minimal-")
if minimal:
    flags = subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "xft", "x11", "xext"], text=True
    ).split()
    subprocess.run(
        ["clang++", "-std=c++17", "-g", "-O0", "-Wall", "-Wextra", "-Werror",
         ".github/scripts/x11-reconnect-reproducer.cc", *flags,
         "-o", "build/x11-reconnect-reproducer"], check=True,
    )
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

    def test_minimal_reconnect(self):
        self.process = subprocess.Popen(
            ["build/x11-reconnect-reproducer", *shlex.split(os.environ.get("CONKY_DIAG_MINIMAL_ARGS", "--iterations 1000"))],
            env=namespace["ENVIRONMENT"], stdin=subprocess.DEVNULL,
            stdout=self.log, stderr=subprocess.STDOUT,
        )
        self.addCleanup(self.stop)
        deadline = time.monotonic() + 90
        count = 0
        while self.process.poll() is None and time.monotonic() < deadline:
            self.probe()
            count += 1
            time.sleep(0.025)
        print(f"Minimal reproducer: {count} observer snapshots", flush=True)
        if self.process.poll() is None:
            self.fail("Minimal reconnect hung: " + self.log_path.read_text()[-4000:])
        self.assertEqual(self.process.returncode, 0, self.log_path.read_text()[-4000:])
        print(self.log_path.read_text()[-1000:], flush=True)

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
            for server_pid in result.stdout.split():
                try:
                    argv = Path(f"/proc/{server_pid}/cmdline").read_bytes().split(b"\0")
                except FileNotFoundError:
                    continue
                if len(argv) > 1 and argv[1].decode() == os.environ["CONKY_TEST_X11_DISPLAY"]:
                    pids.append(server_pid)
            for pid in pids:
                print(f"\nSTACKS FOR PID {pid}", flush=True)
                subprocess.run(["sudo", "-n", "ls", "-l", f"/proc/{pid}/fd"], check=False)
                subprocess.run(["sudo", "-n", "cat", f"/proc/{pid}/net/unix"], check=False)
                for info in Path(f"/proc/{pid}/fdinfo").glob("*"):
                    subprocess.run(["sudo", "-n", "cat", str(info)], check=False)
                commands = ["set pagination off", "thread apply all bt"]
                if self.process is not None and pid == str(self.process.pid):
                    commands += ["p *display", "p display->request",
                                 "p display->last_request_read", "p *display->xcb",
                                 "p *display->xcb->connection", "bt full"]
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
                                 "if ((OsCommPtr)clients[$i]->osPrivate)->input\n"
                                 "p *((OsCommPtr)clients[$i]->osPrivate)->input\n"
                                 "end\n"
                                 "if ((OsCommPtr)clients[$i]->osPrivate)->output\n"
                                 "p *((OsCommPtr)clients[$i]->osPrivate)->output\n"
                                 "end\n"
                                 "end\nset $i=$i+1\nend"]
                try:
                    subprocess.run(
                        ["sudo", "-n", "gdb", "-batch",
                         "-iex", "set debuginfod enabled off",
                         *[arg for cmd in commands for arg in ("-ex", cmd)],
                         "-p", pid], timeout=90, check=False,
                    )
                except subprocess.TimeoutExpired:
                    print("Debugger timed out", flush=True)
        super().fail(msg)


suite = unittest.TestSuite(
    DiagnosticReload("test_minimal_reconnect" if minimal else
                     "test_console_retires_window_and_x11_can_restart")
    for _ in range(1 if minimal else 100)
)
result = unittest.TextTestRunner(verbosity=2, failfast=True).run(suite)
print(f"DIAGNOSTIC RESULT {mode}: success={result.wasSuccessful()}", flush=True)
# Always run the other control; this workflow gathers evidence, not a fix gate.
