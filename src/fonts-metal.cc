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
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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
#include "logging.h"

unsigned int selected_font = 0;
std::vector<font_list> fonts;
char fontloaded = 0;

void font_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && out_to_x.get(*state)) {
    if (fonts.empty()) { fonts.resize(1); }
    fonts[0].name = do_convert(l, -1).first;
  }

  ++s;
}

font_setting font;

conky::simple_config_setting<std::string> font_template[10] = {{"font0", ""}, {"font1", ""},
                                                               {"font2", ""}, {"font3", ""},
                                                               {"font4", ""}, {"font5", ""},
                                                               {"font6", ""}, {"font7", ""},
                                                               {"font8", ""}, {"font9", ""}};

#ifdef BUILD_XFT
namespace {
class xftalpha_setting : public conky::simple_config_setting<float> {
  using Base = conky::simple_config_setting<float>;

 protected:
  void lua_setter(lua::state &l, bool init) override {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init && out_to_x.get(*state)) {
      fonts[0].font_alpha = do_convert(l, -1).first * 0xffff;
    }

    ++s;
  }

 public:
  xftalpha_setting() : Base("xftalpha", 1.0, false) {}
};

xftalpha_setting xftalpha;
}  // namespace
#endif /* BUILD_XFT */

void set_font() {
}

void setup_fonts() {
  set_font();
}

int add_font(const char *data_in) {
  return fonts.size() - 1;
}

void free_fonts(bool utf8) {
}

void load_fonts(bool utf8) {
}
