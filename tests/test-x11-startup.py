#!/usr/bin/env python3
"""Check X11 connection failures in a real Conky process without an X server.

Usage: python3 tests/test-x11-startup.py /path/to/conky
The executable must have X11 support. Wayland support may be on or off.
"""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


if len(sys.argv) != 2:
    raise SystemExit(__doc__)
BINARY = str(Path(sys.argv.pop()).resolve())
MARKER = "CONKY_X11_STARTUP_FALLBACK_OK"

# No colon means XOpenDisplay rejects the display syntax. Unlike a guessed
# unused display number, this cannot connect to another user's X server.
INVALID_DISPLAY = "conky-test-invalid-display"
SESSION_VARIABLES = (
    "DISPLAY",
    "WAYLAND_DISPLAY",
    "XDG_SESSION_TYPE",
    "XDG_CURRENT_DESKTOP",
    "XDG_SESSION_DESKTOP",
    "DESKTOP_SESSION",
    "GDMSESSION",
    "XAUTHORITY",
)


class X11StartupFallback(unittest.TestCase):
    def test_failed_connections_fall_back_to_console(self):
        # name, explicit output selection, environment, should attempt X11
        cases = (
            ("headless_auto", "", {}, False),
            ("empty_display_auto", "", {"DISPLAY": ""}, True),
            (
                "invalid_display_auto",
                "",
                {"DISPLAY": INVALID_DISPLAY},
                True,
            ),
            (
                "x11_session_without_display",
                "",
                {"XDG_SESSION_TYPE": "x11"},
                True,
            ),
            ("explicit_x11_without_display", "'x11'", {}, True),
            (
                "explicit_x11_empty_display",
                "'x11'",
                {"DISPLAY": ""},
                True,
            ),
            (
                "explicit_x11_invalid_display",
                "'x11'",
                {"DISPLAY": INVALID_DISPLAY},
                True,
            ),
            (
                "failed_x11_preserves_console",
                "{'x11', 'console'}",
                {"DISPLAY": INVALID_DISPLAY},
                True,
            ),
            (
                "explicit_console_ignores_invalid_display",
                "'console'",
                {"DISPLAY": INVALID_DISPLAY},
                False,
            ),
        )

        with tempfile.TemporaryDirectory(prefix="conky-x11-startup-") as tmp:
            for name, selection, session, attempts_x11 in cases:
                with self.subTest(name=name):
                    config = Path(tmp) / (name + ".lua")
                    backend = (
                        "output_backend=" + selection + "," if selection else ""
                    )
                    config.write_text(
                        "conky.config={"
                        "background=false,total_run_times=1,update_interval=0.01,"
                        "own_window=true,"
                        + backend
                        + "};conky.text='"
                        + MARKER
                        + "'\n",
                        encoding="utf-8",
                    )
                    environment = os.environ.copy()
                    for variable in SESSION_VARIABLES:
                        environment.pop(variable, None)
                    environment.update(session)
                    result = subprocess.run(
                        [BINARY, "-D", "-c", str(config)],
                        env=environment,
                        cwd=tmp,
                        stdin=subprocess.DEVNULL,
                        capture_output=True,
                        text=True,
                        timeout=10,
                        check=False,
                    )
                    diagnostic = (
                        f"exit={result.returncode}\n"
                        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
                    )
                    self.assertEqual(result.returncode, 0, diagnostic)
                    self.assertIn(MARKER, result.stdout, diagnostic)
                    if attempts_x11:
                        self.assertIn("can't open display", result.stderr)
                        # An explicitly requested console is already usable;
                        # only the other failed-X11 cases need the fallback.
                        if selection != "{'x11', 'console'}":
                            self.assertIn(
                                "falling back to console output", result.stderr
                            )
                    else:
                        self.assertNotIn("can't open display", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
