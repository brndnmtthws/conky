#include "system-details.hh"

#include <cstdlib>
#include <cstring>

#include "data/top.h"
#include "logging.h"

void populate_system_details() {
  static bool once = false;
  if (once) return;
  once = true;

  conky::info::system *info = user_system();

  // XDG_SESSION_TYPE is authoritative when present, but it's frequently unset
  // or reports "tty"/"unspecified". Fall back to the protocol-specific display
  // sockets: Wayland always exports WAYLAND_DISPLAY, and X11 virtually always
  // exports DISPLAY. DISPLAY wins over WAYLAND_DISPLAY: when both are present
  // an X server (Xwayland) is provably available and an X11 surface works on
  // either session type, so it's the safe choice to commit to.
  const char *session_ty = getenv("XDG_SESSION_TYPE");
  if (session_ty != nullptr && std::strcmp(session_ty, "wayland") == 0) {
    info->session = conky::info::display_session::wayland;
  } else if (session_ty != nullptr && std::strcmp(session_ty, "x11") == 0) {
    info->session = conky::info::display_session::x11;
  } else if (getenv("DISPLAY") != nullptr) {
    info->session = conky::info::display_session::x11;
  } else if (getenv("WAYLAND_DISPLAY") != nullptr) {
    info->session = conky::info::display_session::wayland;
  } else {
    info->session = conky::info::display_session::unknown;
  }

  info->wm_name = getenv("XDG_CURRENT_DESKTOP");
  // Per spec, XDG_CURRENT_DESKTOP is a colon-separated list (e.g.
  // "ubuntu:GNOME" on Ubuntu). The first matching token wins.
  // Others below are non-standard:
  if (info->wm_name == nullptr) {
    info->wm_name = getenv("XDG_SESSION_DESKTOP");
  }
  if (info->wm_name == nullptr) { info->wm_name = getenv("DESKTOP_SESSION"); }
  if (info->wm_name == nullptr) { info->wm_name = getenv("GDMSESSION"); }

#ifdef ENABLE_RUNTIME_TWEAKS
  std::vector<std::string_view> wm_name_tokens;
  if (info->wm_name != nullptr) {
    std::string_view rest{info->wm_name};
    while (!rest.empty()) {
      auto sep = rest.find(':');
      wm_name_tokens.push_back(rest.substr(0, sep));
      if (sep == std::string_view::npos) { break; }
      rest.remove_prefix(sep + 1);
    }
  }

  const auto is_wayland = [&]() {
    return info->session == conky::info::display_session::wayland;
  };

  constexpr auto is_session = [](std::string_view token, auto &&...names) {
    return ((token == std::string_view{names}) || ...);
  };

  // Only add is_wayland guard for WM/DE that will never support another display
  // session protocol. e.g. Budgie will (or has) switch(ed) to Wayland at some
  // point, but older versions may use X11, so it needs to be detected for both
  // X11 and Wayland.
  const auto detect_session = [&](std::string_view token) {
    if (is_session(token, "GNOME")) {
      info->wm = conky::info::window_manager::mutter;
    } else if (is_session(token, "GNOME Classic", "metacity")) {
      info->wm = conky::info::window_manager::metacity;
    } else if (is_session(token, "MATE")) {
      info->wm = conky::info::window_manager::marco;
    } else if (is_session(token, "XFCE", "XFCE4")) {
      info->wm = conky::info::window_manager::xfwm;
    } else if (is_session(token, "KDE", "Plasma", "KDE Plasma")) {
      info->wm = conky::info::window_manager::kwin;
    } else if (is_session(token, "LXDE", "LXQt")) {
      info->wm = conky::info::window_manager::openbox;
    } else if (is_session(token, "Unity")) {
      info->wm = conky::info::window_manager::compiz;
    } else if (is_session(token, "Cinnamon") ||
               getenv("CINNAMON_VERSION") != nullptr) {
      // Muffin → Mutter
      info->wm = conky::info::window_manager::mutter;
      info->wm_name = "Cinnamon";
    } else if (!is_wayland() && is_session(token, "Openbox")) {
      // Openbox doesn't set any session name env variables; must be set
      // manually
      info->wm = conky::info::window_manager::openbox;
    } else if (!is_wayland() && is_session(token, "Fluxbox")) {
      // Fluxbox doesn't set any session name env variables; must be set
      // manually
      info->wm = conky::info::window_manager::fluxbox;
    } else if (!is_wayland() && (is_session(token, "i3", "i3wm"))) {
      info->wm = conky::info::window_manager::i3;
    } else if (is_wayland() && is_session(token, "Hyprland")) {
      info->wm = conky::info::window_manager::hyprland;
    } else if (is_wayland() && is_session(token, "Sway")) {
      info->wm = conky::info::window_manager::sway;
    } else if (!is_wayland() && is_session(token, "bspwm")) {
      info->wm = conky::info::window_manager::bspwm;
    } else if (is_session(token, "awesome")) {
      // some talks about adding Wayland support
      info->wm = conky::info::window_manager::awesome;
    } else if (!is_wayland() && is_session(token, "dwm")) {
      info->wm = conky::info::window_manager::dwm;
    } else if (!is_wayland() && is_session(token, "herbstluftwm")) {
      info->wm = conky::info::window_manager::herbstluftwm;
    } else if (!is_wayland() && is_session(token, "qtile")) {
      info->wm = conky::info::window_manager::qtile;
    } else if (!is_wayland() && is_session(token, "windowmaker")) {
      info->wm = conky::info::window_manager::windowmaker;
    } else if (is_wayland() && is_session(token, "Wayfire")) {
      info->wm = conky::info::window_manager::wayfire;
    } else if (is_wayland() && is_session(token, "River")) {
      info->wm = conky::info::window_manager::river;
    } else if (is_session(token, "Budgie")) {
      // Budgie → Mutter
      info->wm = conky::info::window_manager::mutter;
    } else if (is_session(token, "Deepin")) {
      info->wm = conky::info::window_manager::dde;
    } else if (is_session(token, "Enlightenment", "E17")) {
      info->wm = conky::info::window_manager::enlightenment;
    } else {
      return false;
    }

    return true;
  };

  bool detected_session = false;
  for (const auto &token : wm_name_tokens) {
    if (detect_session(token)) {
      detected_session = true;
      break;
    }
  }

  if (!detected_session) {
    info->wm_name = "unknown";
    info->wm = conky::info::window_manager::unknown;

    // TODO: Doesn't work yet. Process information is not yet populated.
    if (is_process_running("openbox")) {
      info->wm_name = "Openbox";
      info->wm = conky::info::window_manager::openbox;
    }
  }
#endif

  if (info->session != conky::info::display_session::unknown) {
    if (info->wm_name != nullptr) {
      LOG_INFO("'{}' {} session running", info->wm_name, info->session);
    } else {
      LOG_INFO("unknown {} session running", info->session);
    }
  }
}
