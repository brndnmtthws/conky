#include "x11-settings.h"

#include "x11.h"

#include "conky-imlib2.h"

#include <X11/Xlib.h>

conky::simple_config_setting<std::string> display_name("display", std::string(),
                                                       false);

namespace priv {
void out_to_x_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && do_convert(l, -1).first) { init_x11(); }

  ++s;
}

void out_to_x_setting::cleanup(lua::state &l) {
  lua::stack_sentry s(l, -1);

  if (do_convert(l, -1).first) { deinit_x11(); }

  l.pop();
}

#ifdef BUILD_XDBE
bool use_xdbe_setting::set_up(lua::state &l) {
  // double_buffer makes no sense when not drawing to X
  if (!out_to_x.get(l) || !display || !window.window) { return false; }

  int major, minor;

  if (XdbeQueryExtension(display, &major, &minor) == 0) {
    LOG_WARNING("No compatible double buffer extension found");
    return false;
  }

  window.back_buffer =
      XdbeAllocateBackBufferName(display, window.window, XdbeBackground);
  if (window.back_buffer != None) {
    window.drawable = window.back_buffer;
  } else {
    LOG_WARNING("Failed to allocate back buffer");
    return false;
  }

  XFlush(display);
  return true;
}

void use_xdbe_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && do_convert(l, -1).first) {
    if (!set_up(l)) {
      l.pop();
      l.pushboolean(false);
    }

    LOG_INFO("drawing to %s buffer",
             do_convert(l, -1).first ? "double" : "single");
  }

  ++s;
}

#else
bool use_xpmdb_setting::set_up(lua::state &l) {
  // double_buffer makes no sense when not drawing to X
  if (!out_to_x.get(l)) return false;

  window.back_buffer =
      XCreatePixmap(display, window.window, window.geometry.width() + 1, window.geometry.height() + 1,
                    DefaultDepth(display, screen));
  if (window.back_buffer != None) {
    window.drawable = window.back_buffer;
  } else {
    LOG_WARNING("Failed to allocate back buffer");
    return false;
  }

  XFlush(display);
  return true;
}

void use_xpmdb_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && do_convert(l, -1).first) {
    if (!set_up(l)) {
      l.pop();
      l.pushboolean(false);
    }

    LOG_INFO("drawing to %s buffer",
             do_convert(l, -1).first ? "double" : "single");
  }

  ++s;
}
#endif
}  // namespace priv

conky::simple_config_setting<int> head_index("xinerama_head", 0, true);
priv::out_to_x_setting out_to_x;

#ifdef BUILD_XFT
conky::simple_config_setting<bool> use_xft("use_xft", false, false);
#endif

conky::simple_config_setting<bool> forced_redraw("forced_redraw", false, false);

#ifdef BUILD_XDBE
priv::use_xdbe_setting use_xdbe;
#else
priv::use_xpmdb_setting use_xpmdb;
#endif
