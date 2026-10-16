/* Diagnostic: isolated Xlib/DBE/Xft reconnects, independent of Conky/Cairo.
 * The default path deliberately has no sync or delay between DBE flush and
 * font loading. External x11-test-probe supplies concurrent server grabs.
 */
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
long iteration = 0;
volatile sig_atomic_t signal_seen = 0;
void on_signal(int) { signal_seen = 1; }
void stage(const char* name) {
  std::printf("iteration=%ld pid=%ld signal_seen=%d stage=%s\n", iteration,
              static_cast<long>(getpid()), static_cast<int>(signal_seen), name);
  std::fflush(stdout);
}
[[noreturn]] void fail(const char* message) {
  std::fprintf(stderr, "iteration=%ld: %s\n", iteration, message);
  std::fflush(stderr);
  std::exit(1);
}
int on_error(Display*, XErrorEvent* error) {
  std::fprintf(stderr, "X error=%u request=%u minor=%u xid=%lu serial=%lu\n",
               error->error_code, error->request_code, error->minor_code,
               error->resourceid, error->serial);
  std::fflush(stderr);
  std::_Exit(1);
}
int on_io_error(Display*) { fail("X connection failed"); }
long number(const char* text) {
  char* end = nullptr;
  errno = 0;
  long value = std::strtol(text, &end, 10);
  if (errno || !*text || *end || value < 0 || value > 10000000)
    fail("numeric argument must be 0..10000000");
  return value;
}
void usage(const char* program) {
  std::fprintf(stderr,
      "Usage: %s [--iterations N] [--retain N] [--sync-before-font]\n"
      "  [--delay-before-font-us N] [--hold-us N] [--signal-us N]\n"
      "  [--no-restart] [--no-xft-draw]\n"
      "Defaults: 1000 iterations, retain 1, no pre-font sync/delay, hold 20000us,\n"
      "  no timer, SA_RESTART enabled, XftDraw create/destroy enabled.\n"
      "--retain N keeps Display/window for N font/back-buffer reloads before\n"
      "  retiring both. --signal-us enables periodic harmless SIGALRM; SIGUSR1\n"
      "  also has the same flag-only handler. Requires CONKY_TEST_X11_DISPLAY.\n",
      program);
}
}  // namespace

int main(int argc, char** argv) {
  long iterations = 1000, retain = 1, delay = 0, hold = 20000, signal_us = 0;
  bool sync = false, restart = true, draw = true;
  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (!std::strcmp(arg, "--sync-before-font")) sync = true;
    else if (!std::strcmp(arg, "--no-restart")) restart = false;
    else if (!std::strcmp(arg, "--no-xft-draw")) draw = false;
    else if (!std::strcmp(arg, "--help")) { usage(argv[0]); return 0; }
    else if (i + 1 < argc) {
      if (!std::strcmp(arg, "--iterations")) iterations = number(argv[++i]);
      else if (!std::strcmp(arg, "--retain")) retain = number(argv[++i]);
      else if (!std::strcmp(arg, "--delay-before-font-us")) delay = number(argv[++i]);
      else if (!std::strcmp(arg, "--hold-us")) hold = number(argv[++i]);
      else if (!std::strcmp(arg, "--signal-us")) signal_us = number(argv[++i]);
      else { usage(argv[0]); return 2; }
    } else { usage(argv[0]); return 2; }
  }
  if (!iterations || !retain) fail("iterations and retain must be positive");
  const char* display_name = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!display_name || !*display_name) {
    std::fprintf(stderr, "Set CONKY_TEST_X11_DISPLAY to an isolated X server; never a desktop\n");
    return 77;
  }
  struct sigaction action {};
  action.sa_handler = on_signal;
  sigemptyset(&action.sa_mask);
  action.sa_flags = restart ? SA_RESTART : 0;
  if (sigaction(SIGUSR1, &action, nullptr) || sigaction(SIGALRM, &action, nullptr))
    fail("sigaction failed");
  if (signal_us) {
    struct itimerval timer {};
    timer.it_interval.tv_sec = signal_us / 1000000;
    timer.it_interval.tv_usec = signal_us % 1000000;
    timer.it_value = timer.it_interval;
    if (setitimer(ITIMER_REAL, &timer, nullptr)) fail("setitimer failed");
  }
  XSetErrorHandler(on_error);
  XSetIOErrorHandler(on_io_error);
  Display* display = nullptr;
  Window window = None;
  XdbeBackBuffer back = None;
  for (iteration = 1; iteration <= iterations; ++iteration) {
    if (!display) {
      stage("XOpenDisplay begin");
      display = XOpenDisplay(display_name);
      if (!display) fail("XOpenDisplay failed");
      int screen = DefaultScreen(display);
      XSetWindowAttributes attrs {};
      attrs.background_pixel = BlackPixel(display, screen);
      attrs.backing_store = Always;
      attrs.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask |
                         ButtonReleaseMask;
      attrs.override_redirect = False;
      stage("XCreateWindow/map begin");
      window = XCreateWindow(display, RootWindow(display, screen), 30, 30, 5, 5,
                             0, CopyFromParent, InputOutput,
                             DefaultVisual(display, screen),
                             CWBackPixel | CWBackingStore | CWEventMask |
                             CWOverrideRedirect, &attrs);
      if (window == None) fail("XCreateWindow failed");
      XStoreName(display, window, "ConkyReloadTest");
      XMapWindow(display, window);
    }
    if (back != None) {
      stage("XdbeDeallocateBackBufferName begin");
      XdbeDeallocateBackBufferName(display, back);
    }
    int major = 0, minor = 0;
    stage("XdbeQueryExtension begin");
    if (!XdbeQueryExtension(display, &major, &minor)) fail("DBE unavailable");
    stage("XdbeAllocateBackBufferName/flush begin");
    back = XdbeAllocateBackBufferName(display, window, XdbeBackground);
    if (back == None) fail("DBE allocation failed");
    XFlush(display);
    if (sync) { stage("control XSync begin"); XSync(display, False); }
    if (delay) usleep(static_cast<useconds_t>(delay));
    stage("XftFontOpenName begin");
    int screen = DefaultScreen(display);
    XftFont* font = XftFontOpenName(display, screen, "DejaVu Sans Mono:size=20");
    if (!font) fail("XftFontOpenName failed");
    stage("XftFontOpenName done");
    XftDraw* xft_draw = nullptr;
    if (draw) {
      stage("XftDrawCreate begin");
      xft_draw = XftDrawCreate(display, back, DefaultVisual(display, screen),
                               DefaultColormap(display, screen));
      if (!xft_draw) fail("XftDrawCreate failed");
    }
    if (hold) usleep(static_cast<useconds_t>(hold));
    stage("XftFontClose begin");
    XftFontClose(display, font);
    if (xft_draw) { stage("XftDrawDestroy begin"); XftDrawDestroy(xft_draw); }
    if (iteration % retain == 0 || iteration == iterations) {
      stage("retire DBE/window begin");
      XdbeDeallocateBackBufferName(display, back);
      back = None;
      XDestroyWindow(display, window);
      window = None;
      stage("XCloseDisplay begin");
      XCloseDisplay(display);
      display = nullptr;
      stage("XCloseDisplay done");
    }
  }
  stage("complete");
  return 0;
}
