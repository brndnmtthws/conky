/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#include "../conky.h"
#include "../content/colours.hh"
#include "display-ncurses.hh"
#include "gui.h"
#include "nc.h"

#include <iostream>
#include <sstream>
#include <unordered_map>

#include <ncurses.h>

extern WINDOW* ncurses_window;

namespace conky {
namespace {
conky::display_output_ncurses ncurses_output;
}  // namespace
template <>
void register_output<output_t::NCURSES>(display_outputs_t& outputs) {
  outputs.push_back(&ncurses_output);
}

// namespace priv {

//}  // namespace priv

#define COLORS_BUILTIN 8

Colour ncurses_colors[COLORS_BUILTIN + COLORS_CUSTOM] = {
    {0x00, 0x00, 0x00, 0xff},  // BLACK
    {0xff, 0x00, 0x00, 0xff},  // RED
    {0x00, 0xff, 0x00, 0xff},  // GREEN
    {0xff, 0xff, 0x00, 0xff},  // YELLOW
    {0x00, 0x00, 0xff, 0xff},  // BLUE
    {0xff, 0x00, 0xff, 0xff},  // MAGENTA
    {0x00, 0xff, 0xff, 0xff},  // CYAN
    {0xff, 0xff, 0xff, 0xff},  // WHITE
};

// Find the nearest ncurses color.
int to_ncurses(const Colour& c) {
  int mindiff = INT_MAX;
  int best_nccolor = 0;
  for (int nccolor = 0; nccolor < COLORS_BUILTIN + COLORS_CUSTOM; nccolor++) {
    const Colour& other = ncurses_colors[nccolor];
    int diff = abs(c.red - other.red) + abs(c.green - other.green) +
               abs(c.blue - other.blue);

    if (diff < mindiff) {
      mindiff = diff;
      best_nccolor = nccolor;
    }
  }
  return best_nccolor;
}

Colour from_ncurses(int nccolor) {
  if (nccolor >= 0 && nccolor < COLORS_BUILTIN + COLORS_CUSTOM) {
    return ncurses_colors[nccolor];
  }
  return ERROR_COLOUR;
}

display_output_ncurses::display_output_ncurses()
    : display_output_console("ncurses") {}

bool display_output_ncurses::detect() {
  if (out_to_ncurses.get(*state)) {
    DBGP2("Display output '%s' enabled in config.", name.c_str());
    return true;
  }
  return false;
}

bool display_output_ncurses::initialize() {
  for (int i = 0; i < COLORS_CUSTOM; i++) {
    Colour c = color[i].get(*state);
    init_color(COLORS_BUILTIN + i, (1000 * c.red) / 255, (1000 * c.green) / 255,
               (1000 * c.blue) / 255);
    ncurses_colors[COLORS_BUILTIN + i] = c;
  }

  is_active = ncurses_window != nullptr;
  return is_active;
}

bool display_output_ncurses::shutdown() { return false; }

void display_output_ncurses::set_foreground_color(Colour c) {
  int nccolor = to_ncurses(c);
  init_pair(nccolor + 1, nccolor, COLOR_BLACK);
  attron(COLOR_PAIR(nccolor + 1));
}

void display_output_ncurses::begin_draw_text() {
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
  attron(COLOR_PAIR(COLOR_WHITE));
}

void display_output_ncurses::end_draw_text() {}

void display_output_ncurses::draw_string(const char* s, int) {
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
}  // namespace conky
