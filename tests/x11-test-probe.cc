/* X11 integration-test observer; run only against an isolated test server. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
}  // namespace

int main(int argc, char** argv) {
  auto display = XOpenDisplay(nullptr);
  if (!display) { return 2; }
  XSetErrorHandler(on_error);
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
    std::printf("%s{\"id\":%lu,\"width\":%d,\"height\":%d,\"text\":%s}",
                first ? "" : ",", children[i], attrs.width, attrs.height,
                has_text(display, children[i], attrs) ? "true" : "false");
    first = false;
  }
  if (children) { XFree(children); }
  XUngrabServer(display);
  XSync(display, False);
  std::printf("],\"errors\":%d}\n", errors);
  XCloseDisplay(display);
  return errors ? 1 : 0;
}
