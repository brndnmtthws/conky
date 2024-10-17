/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, et. al.
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

#ifndef _CONKY_IMBLI2_H_
#define _CONKY_IMBLI2_H_

#include "setting.hh"
#include "text_object.h"

#include <array>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <X11/Xlib.h>
#pragma GCC diagnostic pop

using saved_coordinates_t = std::array<std::array<int, 2>, 100>;
extern saved_coordinates_t saved_coordinates;

void cimlib_add_image(const char *args);
void cimlib_set_cache_size(long size);
void cimlib_set_cache_flush_interval(long interval);
void cimlib_render(int x, int y, int width, int height, uint32_t flush_interval,
                   bool draw_blended);
void cimlib_cleanup(void);

void print_image_callback(struct text_object *, char *, unsigned int);

class imlib_cache_size_setting
    : public conky::range_config_setting<unsigned long> {
  typedef conky::range_config_setting<unsigned long> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);
  virtual void cleanup(lua::state &l);

 public:
  imlib_cache_size_setting()
      : Base("imlib_cache_size", 0, std::numeric_limits<unsigned long>::max(),
             4096 * 1024, true) {}
};

extern conky::range_config_setting<unsigned int> imlib_cache_flush_interval;
extern conky::simple_config_setting<bool> imlib_draw_blended;

#endif /* _CONKY_IMBLI2_H_ */
