/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 */

#include <config.h>

#include "nc.h"

#ifdef BUILD_NCURSES
WINDOW *ncurses_window;
#endif

namespace priv {
void out_to_ncurses_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && do_convert(l, -1).first) {
    ncurses_window = initscr();
    start_color();
  }

  ++s;
}

void out_to_ncurses_setting::cleanup(lua::state &l) {
  lua::stack_sentry s(l, -1);

  if (do_convert(l, -1).first) endwin();

  l.pop();
}
}  // namespace priv

priv::out_to_ncurses_setting out_to_ncurses;
