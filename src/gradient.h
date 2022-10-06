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

#ifndef _GRADIENT_H
#define _GRADIENT_H

#include <memory>

namespace conky {
class gradient_factory {
 public:
  typedef std::unique_ptr<unsigned long[]> colour_array;
  static const long SCALE = 512L;
  static const long SCALE2 = SCALE * 2;
  static const long SCALE4 = SCALE * 4;
  static const long SCALE6 = SCALE * 6;
  static const long SCALE60 = SCALE * 60;
  static const long SCALE120 = SCALE * 120;
  static const long SCALE180 = SCALE * 180;
  static const long SCALE240 = SCALE * 240;
  static const long SCALE300 = SCALE * 300;
  static const long SCALE360 = SCALE * 360;

 public:
  gradient_factory(int width, unsigned long first_colour,
                   unsigned long last_colour);
  virtual ~gradient_factory() { }

  colour_array create_gradient();

  virtual void convert_from_scaled_rgb(long *const scaled, long *target) = 0;
  virtual void convert_to_scaled_rgb(long *const target, long *scaled) = 0;

  void convert_from_rgb(long original, long *array);
  int convert_to_rgb(long *const array);

 protected:
  virtual void fix_diff(long *diff) {}

  static long get_hue(long *const scaled, long chroma, long value);
  static long get_intermediate(long hue, long chroma);

  static short colour_depth;
  static long mask[3];
  static short shift[3];

 private:
  int width;
  unsigned long first_colour;
  unsigned long last_colour;

  static bool is_set;
  static void setup_colour_depth();
  static void setup_masks();
  static void setup_shifts();
};

class rgb_gradient_factory : public gradient_factory {
  using gradient_factory::gradient_factory;

 public:
  void convert_from_scaled_rgb(long *const scaled, long *target);
  void convert_to_scaled_rgb(long *const target, long *scaled);
};

class hsv_gradient_factory : public gradient_factory {
  using gradient_factory::gradient_factory;

 public:
  void fix_diff(long *diff);
  void convert_from_scaled_rgb(long *const scaled, long *target);
  void convert_to_scaled_rgb(long *const target, long *scaled);
};

class hcl_gradient_factory : public gradient_factory {
  using gradient_factory::gradient_factory;

 public:
  void fix_diff(long *diff);
  void convert_from_scaled_rgb(long *const scaled, long *target);
  void convert_to_scaled_rgb(long *const target, long *scaled);
};
}  // namespace conky

#endif /* _GRADIENT_H */
