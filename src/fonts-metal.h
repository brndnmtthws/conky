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
#ifndef _FONTS_H
#define _FONTS_H

#include <vector>

#include "conky.h"

/* for fonts */
struct font_list {
  std::string name;

  font_list()
      : name()
  {
  }
};


#define font_height() 0
#define font_ascent() 0
#define font_descent() 0
#define font_height() 0
#define font_ascent() 0
#define font_descent() 0

/* direct access to registered fonts (FIXME: bad encapsulation) */
extern std::vector<font_list> fonts;
extern unsigned int selected_font;

void setup_fonts(void);
void set_font(void);
int add_font(const char *);
void free_fonts(bool utf8);
void load_fonts(bool utf8);

class font_setting : public conky::simple_config_setting<std::string> {
  typedef conky::simple_config_setting<std::string> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);

 public:
  font_setting() : Base("font", "6x10", false) {}
};

extern font_setting font;

extern conky::simple_config_setting<std::string> font_template[10];

#endif /* _FONTS_H */
