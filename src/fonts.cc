/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
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

#include "fonts.h"
#include "gui.h"
#include "display-output.hh"
#include "logging.h"

unsigned int selected_font = 0;
std::vector<font_list> fonts;
char fontloaded = 0;

void font_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init) {
    if (fonts.empty()) { fonts.resize(1); }
    fonts[0].name = do_convert(l, -1).first;
  }

  ++s;
}

font_setting font;

conky::simple_config_setting<std::string> font_template[10] = {
    {"font0", ""}, {"font1", ""}, {"font2", ""}, {"font3", ""}, {"font4", ""},
    {"font5", ""}, {"font6", ""}, {"font7", ""}, {"font8", ""}, {"font9", ""}};

void set_font() {
  if (selected_font >= fonts.size()) return;
  for (auto output : display_outputs()) output->set_font(selected_font);
}

void setup_fonts() {
  DBGP2("setting up fonts");
  for (auto output : display_outputs()) output->setup_fonts();
  set_font();
}

int add_font(const char *data_in) {
  if (!out_to_gui(*state)) { return 0; }
  fonts.emplace_back();
  fonts.rbegin()->name = data_in;

  return fonts.size() - 1;
}

void free_fonts(bool utf8) {
  for (auto output : display_outputs()) output->free_fonts(utf8);
  fonts.clear();
  selected_font = 0;
}

void load_fonts(bool utf8) {
  DBGP2("loading fonts");
  for (auto output : display_outputs()) output->load_fonts(utf8);
}

int font_height() {
  //assert(selected_font < fonts.size());
  return display_output()->font_height(selected_font);
}

int font_ascent() {
  //assert(selected_font < fonts.size());
  return display_output()->font_ascent(selected_font);
}

int font_descent() {
  //assert(selected_font < fonts.size());
  return display_output()->font_descent(selected_font);
}
