/* X11 integration-test observer; run only against an isolated test server. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/select.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <sstream>
#include <string>

namespace {
int errors = 0;
int on_error(Display*, XErrorEvent*) {
  ++errors;
  return 0;
}

bool has_text(Display* display, Window window, const XWindowAttributes& attrs) {
  if (attrs.map_state != IsViewable) { return false; }
  auto image = XGetImage(display, window, 0, 0, attrs.width, attrs.height,
                         AllPlanes, ZPixmap);
  if (!image) { return false; }
  auto first = XGetPixel(image, 0, 0);
  bool varied = false;
  for (int y = 0; y < attrs.height && !varied; ++y) {
    for (int x = 0; x < attrs.width; ++x) {
      if (XGetPixel(image, x, y) != first) {
        varied = true;
        break;
      }
    }
  }
  XDestroyImage(image);
  return varied;
}

void configure_window(Display* display, const XConfigureRequestEvent& request) {
  XWindowChanges changes{};
  changes.x = request.x;
  changes.y = request.y;
  changes.width = request.width;
  changes.height = request.height;
  changes.border_width = request.border_width;
  changes.sibling = request.above;
  changes.stack_mode = request.detail;
  XConfigureWindow(display, request.window, request.value_mask, &changes);
}

// A deliberately small WM for testing redirected, asynchronous resizes.
// Map requests are honored immediately; geometry requests wait for a command
// from the test. Every response is one JSON line, flushed before waiting.
int run_window_manager(Display* display) {
  XSelectInput(display, DefaultRootWindow(display),
               SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(display, False);
  if (errors) { return 1; }
  std::puts("{\"event\":\"ready\"}");
  std::fflush(stdout);
  std::deque<XConfigureRequestEvent> pending;
  std::string input;
  while (!errors) {
    while (XPending(display)) {
      XEvent event{};
      XNextEvent(display, &event);
      if (event.type == MapRequest) {
        XMapWindow(display, event.xmaprequest.window);
      } else if (event.type == ConfigureRequest) {
        const auto& request = event.xconfigurerequest;
        pending.push_back(request);
        std::printf(
            "{\"event\":\"request\",\"id\":%lu,\"width\":%d,"
            "\"height\":%d,\"resizes\":%s}\n",
            request.window, request.width, request.height,
            request.value_mask & (CWWidth | CWHeight) ? "true" : "false");
        std::fflush(stdout);
      }
    }
    XFlush(display);
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(STDIN_FILENO, &readable);
    FD_SET(ConnectionNumber(display), &readable);
    int maximum = ConnectionNumber(display) > STDIN_FILENO
                      ? ConnectionNumber(display)
                      : STDIN_FILENO;
    if (select(maximum + 1, &readable, nullptr, nullptr, nullptr) < 0) {
      return 2;
    }
    if (!FD_ISSET(STDIN_FILENO, &readable)) { continue; }
    char buffer[256];
    auto count = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (count <= 0) { return 0; }
    input.append(buffer, count);
    std::size_t newline;
    while ((newline = input.find('\n')) != std::string::npos) {
      std::istringstream command(input.substr(0, newline));
      input.erase(0, newline + 1);
      std::string operation;
      command >> operation;
      if (operation == "quit") { return 0; }
      if (operation == "apply" || operation == "constrain" ||
          operation == "reject") {
        if (pending.empty()) { return 2; }
        auto request = pending.front();
        pending.pop_front();
        if (operation == "constrain") {
          if (!(command >> request.width >> request.height)) { return 2; }
          request.value_mask |= CWWidth | CWHeight;
        }
        if (operation == "reject") {
          // ICCCM requires a synthetic acknowledgement when a WM leaves
          // the requested geometry unchanged.
          XWindowAttributes attrs{};
          if (!XGetWindowAttributes(display, request.window, &attrs)) {
            return 1;
          }
          XEvent event{};
          event.xconfigure.type = ConfigureNotify;
          event.xconfigure.display = display;
          event.xconfigure.event = event.xconfigure.window = request.window;
          event.xconfigure.x = attrs.x;
          event.xconfigure.y = attrs.y;
          event.xconfigure.width = attrs.width;
          event.xconfigure.height = attrs.height;
          event.xconfigure.border_width = attrs.border_width;
          XSendEvent(display, request.window, False, StructureNotifyMask,
                     &event);
        } else {
          configure_window(display, request);
        }
      } else if (operation == "resize") {
        unsigned long target;
        int width, height;
        if (!(command >> target >> width >> height)) { return 2; }
        XResizeWindow(display, target, width, height);
      } else {
        return 2;
      }
      XSync(display, False);
      std::printf("{\"event\":\"applied\",\"errors\":%d}\n", errors);
      std::fflush(stdout);
    }
  }
  return 1;
}
}  // namespace

int main(int argc, char** argv) {
  auto display = XOpenDisplay(nullptr);
  if (!display) { return 2; }
  XSetErrorHandler(on_error);
  if (argc == 2 && std::strcmp(argv[1], "--wm") == 0) {
    int result = run_window_manager(display);
    XCloseDisplay(display);
    return result;
  }
  // The resize tests use an independent client for real external resizes and
  // for notifications with obsolete dimensions. XSendEvent assigns the
  // receiving client's current serial; old serials are covered separately by
  // the ConfigureNotify handler tests.
  if (argc == 5) {
    Window target = std::strtoul(argv[2], nullptr, 10);
    int width = std::atoi(argv[3]), height = std::atoi(argv[4]);
    if (std::strcmp(argv[1], "--resize") == 0) {
      XResizeWindow(display, target, width, height);
    } else if (std::strcmp(argv[1], "--notify") == 0) {
      XEvent event{};
      event.xconfigure.type = ConfigureNotify;
      event.xconfigure.display = display;
      event.xconfigure.event = target;
      event.xconfigure.window = target;
      event.xconfigure.width = width;
      event.xconfigure.height = height;
      XSendEvent(display, target, False, StructureNotifyMask, &event);
    } else {
      XCloseDisplay(display);
      return 2;
    }
    XSync(display, False);
    XCloseDisplay(display);
    return errors ? 1 : 0;
  }
  if (argc != 1) {
    XCloseDisplay(display);
    return 2;
  }
  // Keep the snapshot consistent while Conky reloads: an enumerated window may
  // otherwise be destroyed, unmapped, or resized before its later queries.
  // This observer is only used on an isolated test server.
  XGrabServer(display);
  auto root = DefaultRootWindow(display);
  XWindowAttributes attrs{};
  XGetWindowAttributes(display, root, &attrs);
  std::printf("{\"root\":%lu,\"root_text\":%s,\"windows\":[", root,
              has_text(display, root, attrs) ? "true" : "false");
  Window returned_root, parent, *children = nullptr;
  unsigned count = 0;
  XQueryTree(display, root, &returned_root, &parent, &children, &count);
  bool first = true;
  for (unsigned i = 0; i < count; ++i) {
    char* name = nullptr;
    XFetchName(display, children[i], &name);
    bool ours = name && std::strcmp(name, "ConkyReloadTest") == 0;
    if (name) { XFree(name); }
    if (!ours || !XGetWindowAttributes(display, children[i], &attrs)) {
      continue;
    }
    int root_x, root_y;
    Window child;
    XTranslateCoordinates(display, children[i], root, 0, 0, &root_x, &root_y,
                          &child);
    std::printf(
        "%s{\"id\":%lu,\"width\":%d,\"height\":%d,"
        "\"root_x\":%d,\"root_y\":%d,\"text\":%s}",
        first ? "" : ",", children[i], attrs.width, attrs.height, root_x,
        root_y, has_text(display, children[i], attrs) ? "true" : "false");
    first = false;
  }
  if (children) { XFree(children); }
  XUngrabServer(display);
  XSync(display, False);
  std::printf("],\"errors\":%d}\n", errors);
  XCloseDisplay(display);
  return errors ? 1 : 0;
}
