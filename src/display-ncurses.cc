/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include "conky.h"
#include "display-ncurses.hh"
#include "nc.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#ifdef BUILD_NCURSES
#include <ncurses.h>
#endif

#ifdef BUILD_NCURSES
extern WINDOW *ncurses_window;
#endif

namespace conky {
namespace {

#ifdef BUILD_NCURSES
conky::display_output_ncurses ncurses_output;
#else
conky::disabled_display_output ncurses_output_disabled("ncurses",
                                                       "BUILD_NCURSES");
#endif

}  // namespace
extern void init_ncurses_output() {}

// namespace priv {

//}  // namespace priv

#ifdef BUILD_NCURSES

display_output_ncurses::display_output_ncurses()
    : display_output_console("ncurses") {
  priority = 1;
}

bool display_output_ncurses::detect() {
  if (out_to_ncurses.get(*state)) {
    DBGP2("Display output '%s' enabled in config.", name.c_str());
    return true;
  }
  return false;
}

bool display_output_ncurses::initialize() {
  is_active = ncurses_window != nullptr;
  return is_active;
}

bool display_output_ncurses::shutdown() { return false; }

void display_output_ncurses::set_foreground_color(Colour c) {
  init_pair(c.to_ncurses(), c.to_ncurses(), COLOR_BLACK);
  attron(COLOR_PAIR(c.to_ncurses()));
}

void display_output_ncurses::begin_draw_text() {
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
  attron(COLOR_PAIR(COLOR_WHITE));
}

void display_output_ncurses::end_draw_text() {}

void display_output_ncurses::draw_string(const char *s, int) {
  printw("%s", s);
}

void display_output_ncurses::line_inner_done() { printw("\n"); }

int display_output_ncurses::getx() {
  int x, y;
  getyx(ncurses_window, y, x);
  (void)y;
  return x;
}

int display_output_ncurses::gety() {
  int x, y;
  getyx(ncurses_window, y, x);
  (void)x;
  return y;
}

void display_output_ncurses::gotox(int x) {
  int y, old_x;
  getyx(ncurses_window, y, old_x);
  (void)old_x;
  move(y, x);
}

void display_output_ncurses::gotoy(int y) {
  int x, old_y;
  getyx(ncurses_window, old_y, x);
  (void)old_y;
  move(y, x);
}

void display_output_ncurses::gotoxy(int x, int y) { move(y, x); }

void display_output_ncurses::flush() {
  refresh();
  clear();
}

#endif /* BUILD_NCURSES */

}  // namespace conky
