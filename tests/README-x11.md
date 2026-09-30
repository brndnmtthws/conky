# X11 integration tests

The standard CTest suite registers `x11_startup_fallback` when Conky has X11
support and Python 3 is available. This launches Conky with missing, empty or
syntactically invalid display settings and checks that the console fallback
renders the requested text and exits successfully. It needs no X server.

`x11_reload` is also registered when own-window support is enabled. The
`x11_reload_xft` variant is registered when Xft is enabled. Both tests require
an **isolated test X server**, Python 3 and fonts (`xfonts-base`, and
`fonts-dejavu-core` for Xft on Debian). They skip with status 77 unless
`CONKY_TEST_X11_DISPLAY` is set. Do not point them at a real desktop: the tests
deliberately draw on the root window.

For example, after building Conky and the `x11-test-probe` target:

```sh
xvfb-run -a -s '-screen 0 800x600x24' sh -c \
  'CONKY_TEST_X11_DISPLAY=$DISPLAY ctest --test-dir build -R x11_ --output-on-failure'
```

Alternatively, launch your own isolated X server and set
`CONKY_TEST_X11_DISPLAY` plus any needed `XAUTHORITY` before running CTest. The
tests use the inherited font configuration. An X server without a window
manager is intentional, so the probe can directly inspect the test windows.

Each font variant covers:

- Five ordinary SIGUSR1 reloads, checking that the XID stays unchanged and
  visible text remains present throughout sampled reload frames
- Root drawing to an owned window, correctly sized and rendered after one signal
- Owned window to root drawing, removing the old XID and rendering on the root
- Three complete root/owned-window cycles
- Two complete X11/console cycles, removing the window while console-only and
  rendering successfully after returning to X11
- Graceful exit without stale owned windows or common X resource errors

The pixel probe distinguishes rendered content from a solid window. It is not
OCR or a guarantee against sub-frame flicker. It does not cover real window
managers, compositors, native Wayland, or ARGB visual changes.
