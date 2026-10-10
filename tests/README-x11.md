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

`x11_foreground_alpha` is a discovered Catch2 case in
`test-x11-foreground.cc`, with separate `rgb_drawable` and `argb_drawable`
sections. It is registered whenever X11 support is enabled and shares the
`x11_test_display` resource lock with the reload tests. It requires
`CONKY_TEST_X11_DISPLAY`, a standard 24-bit RGB root visual and a 32-bit ARGB
visual. Missing display opt-in and unsupported servers use Catch2 `SKIP`, which
CTest reports as skipped (Catch2 exit status 4). It draws directly
into 24- and 32-bit pixmaps, so no compositor, window manager, Python or fonts
are needed. It checks exact border, horizontal-rule and filled-rectangle pixels
for every combination of background and foreground alpha 0, 128 and 255,
including premultiplied foreground colors and the non-ARGB fallback. This tests
GC rendering, not compositor blending or window-background changes on reload.

`x11_input_alpha` is a discovered Catch2 case in `test-x11-input-alpha.cc`,
registered with X11, SHAPE and own-window support. It uses the same opt-in,
skip behavior and display lock. It requires SHAPE 1.1, a 24-bit root and a
32-bit ARGB visual; XDBE builds additionally require double-buffer support for
both visuals. It calls the real window creation and buffer-swap paths, then
queries the server's input region. Coverage includes transparent/nonzero-alpha
pixels, partial 4x4 cells, unchanged frames, resize/new-window cache
invalidation, a fragmented checkerboard spanning multiple SHAPE requests,
initial unsupported configurations, and restoring normal, undecorated,
utility and override-window behavior when alpha input or buffering is disabled.
It also checks opaque-to-ARGB recreation, retained ARGB depth on reload, and
retaining an opaque window when no compositor is available.
The test temporarily owns an otherwise unowned compositor selection to enable
ARGB visual selection; it does not require or test compositor blending, fonts
or a window manager. Both pixmap and XDBE back-buffer builds are supported.

Linux CI jobs with X11 enabled run the full CTest suite under an isolated
Xvfb with the required fonts. Non-X11 jobs keep their ordinary CTest path.

For example, after building all targets:

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
