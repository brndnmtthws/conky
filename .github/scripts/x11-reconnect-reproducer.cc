/* Diagnostic-only Xlib/Xft reconnect stressor for an isolated X server.
 * Run x11-test-probe independently against the same server to exercise grabs.
 * No Conky core, Cairo, threads, or forks are involved.
 */
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace {
long iteration = 0;

void stage(const char* name) {
  std::printf("iteration=%ld pid=%ld stage=%s\n", iteration,
              static_cast<long>(getpid()), name);
  std::fflush(stdout);
}

int on_error(Display*, XErrorEvent* error) {
  std::fprintf(stderr,
               "iteration=%ld X error=%u request=%u minor=%u resource=%lu "
               "serial=%lu\n",
               iteration, error->error_code, error->request_code,
               error->minor_code, error->resourceid, error->serial);
  std::fflush(stderr);
  std::_Exit(1);
}

int on_io_error(Display*) {
  std::fprintf(stderr, "iteration=%ld X connection failed\n", iteration);
  std::fflush(stderr);
  std::_Exit(1);
}
}  // namespace

int main(int argc, char** argv) {
  const char* display_name = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!display_name || !*display_name) {
    std::fprintf(stderr,
                 "Refusing to run: set CONKY_TEST_X11_DISPLAY to an isolated "
                 "test server, never a desktop\n");
    return 77;
  }
  long iterations = 1000;
  if (argc > 2) {
    std::fprintf(stderr, "Usage: %s [iterations: 1..100000]\n", argv[0]);
    return 2;
  }
  if (argc == 2) {
    char* end = nullptr;
    errno = 0;
    iterations = std::strtol(argv[1], &end, 10);
    if (errno || !*argv[1] || *end || iterations < 1 || iterations > 100000) {
      std::fprintf(stderr, "Invalid iteration count: %s\n", argv[1]);
      return 2;
    }
  }

  XSetErrorHandler(on_error);
  XSetIOErrorHandler(on_io_error);
  for (iteration = 1; iteration <= iterations; ++iteration) {
    stage("XOpenDisplay begin");
    Display* display = XOpenDisplay(display_name);
    if (!display) {
      std::fprintf(stderr, "Cannot open isolated display %s\n", display_name);
      return 1;
    }
    const int screen = DefaultScreen(display);
    stage("create/map window begin");
    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen), 30, 30, 5, 5, 0,
        BlackPixel(display, screen), BlackPixel(display, screen));
    if (window == None) {
      std::fprintf(stderr, "XCreateSimpleWindow returned None\n");
      XCloseDisplay(display);
      return 1;
    }
    XStoreName(display, window, "ConkyReloadTest");
    XMapWindow(display, window);
    XSync(display, False);
    stage("create/map window done");
    // Give an external observer opportunities to inspect the mapped window.
    usleep(10000);

    stage("XftFontOpenName begin");
    XftFont* font =
        XftFontOpenName(display, screen, "DejaVu Sans Mono:size=20");
    if (!font) {
      std::fprintf(stderr, "XftFontOpenName returned null\n");
      XDestroyWindow(display, window);
      XCloseDisplay(display);
      return 1;
    }
    stage("XftFontOpenName done");
    usleep(10000);
    stage("XftFontClose begin");
    XftFontClose(display, font);
    stage("XDestroyWindow begin");
    XDestroyWindow(display, window);
    stage("XCloseDisplay begin");
    XCloseDisplay(display);
    stage("XCloseDisplay done");
  }
  stage("complete");
  return 0;
}
