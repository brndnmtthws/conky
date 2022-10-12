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
#include "gradient.h"
#include "conky.h"
#include "logging.h"

#ifdef BUILD_X11
#include "x11.h"
#endif /* BUILD_X11 */

namespace conky {
bool gradient_factory::is_set = false;
short gradient_factory::colour_depth = 0;
long gradient_factory::mask[3];
short gradient_factory::shift[3];

gradient_factory::gradient_factory(int width, unsigned long first_colour,
                                   unsigned long last_colour) {
  // Make sure the width is always at least 2
  this->width = std::max(2, width);
  this->first_colour = first_colour;
  this->last_colour = last_colour;

  if (!is_set) {
    setup_colour_depth();
    setup_shifts();
    setup_masks();
    is_set = true;
  }
}

void gradient_factory::setup_shifts() {
  shift[0] = (2 * colour_depth / 3 + colour_depth % 3);
  shift[1] = (colour_depth / 3);
  shift[2] = 0;
}

void gradient_factory::setup_masks() {
  mask[0] = mask[1] = mask[2] = 0;

  for (int i = (colour_depth / 3) - 1; i >= 0; i--) {
    mask[0] |= 1 << i;
    mask[1] |= 1 << i;
    mask[2] |= 1 << i;
  }

  if (colour_depth % 3 == 1) {
    mask[1] |= 1 << (colour_depth / 3);
  }

  for (int i = 0; i < 3; i++) {
    mask[i] = mask[i] << shift[i];
  }
}

void gradient_factory::setup_colour_depth() {
#ifdef BUILD_X11
  if (state == nullptr) { // testing purposes
    colour_depth = 24;
  } else if (out_to_x.get(*state)) {
    colour_depth = DisplayPlanes(display, screen);
  } else
#endif /* BUILD_X11 */
  {
    colour_depth = 16;
  }

  if (colour_depth != 24 && colour_depth != 16) {
    NORM_ERR(
        "using non-standard colour depth, "
        "gradients may look like a lolly-pop");
  }
}

void gradient_factory::convert_from_rgb(long original, long *array) {
  long scaled[3];

  for (int i = 0; i < 3; i++) {
    auto value = (original & mask[i]) >> shift[i];
    scaled[i] = value * SCALE;
  }
  convert_from_scaled_rgb(scaled, array);
}

int gradient_factory::convert_to_rgb(long *const array) {
  long scaled_rgb[3];
  int rgb = 0;

  convert_to_scaled_rgb(array, scaled_rgb);
  for (int i = 0; i < 3; i++) {
    auto value = scaled_rgb[i] / SCALE;
    rgb |= value << shift[i];
  }

  return rgb;
}

gradient_factory::colour_array gradient_factory::create_gradient() {
  colour_array colours(new unsigned long[width]);

  long first_converted[3];
  long last_converted[3];
  long diff[3], delta[3];

  colours[0] = first_colour;
  colours[width - 1] = last_colour;

  convert_from_rgb(first_colour, first_converted);
  convert_from_rgb(last_colour, last_converted);

  for (int i = 0; i < 3; i++) {
    diff[i] = last_converted[i] - first_converted[i];
  }
  fix_diff(diff);
  for (int i = 0; i < 3; i++) { delta[i] = diff[i] / (width - 1); }
  for (int i = 1; i < width - 1; i++) {
    for (int k = 0; k < 3; k++) { first_converted[k] += delta[k]; }
    colours[i] = convert_to_rgb(first_converted);
  }

  return colours;
}

long gradient_factory::get_hue(long *const rgb, long chroma, long value) {
  if (chroma == 0) { return 0; }

  long diff, offset;

  if (rgb[0] == value) {
    diff = rgb[1] - rgb[2];
    offset = 0;
  } else if (rgb[1] == value) {
    diff = rgb[2] - rgb[0];
    offset = SCALE2;
  } else {
    diff = rgb[0] - rgb[1];
    offset = SCALE4;
  }
  long h = (SCALE * diff) / chroma + offset;

  return 60L * ((SCALE6 + h) % SCALE6);
}

long gradient_factory::get_intermediate(long hue, long chroma) {
  long h = hue / 60L;
  long multiplier = SCALE - std::abs(h % SCALE2 - SCALE);

  return (chroma * multiplier) / SCALE;
}

/* rgb_gradient_factory  */
void rgb_gradient_factory::convert_from_scaled_rgb(long *const scaled,
                                                   long *target) {
  target[0] = scaled[0] * 360L;
  target[1] = scaled[1] * 360L;
  target[2] = scaled[2] * 360L;
}

void rgb_gradient_factory::convert_to_scaled_rgb(long *const target,
                                                 long *scaled) {
  scaled[0] = target[0] / 360L;
  scaled[1] = target[1] / 360L;
  scaled[2] = target[2] / 360L;
}
/* rgb_gradient_factory  */

namespace {
long get_value(long *const rgb) {
  if (rgb[0] > rgb[1]) { return std::max(rgb[0], rgb[2]); }
  return std::max(rgb[1], rgb[2]);
}

long get_minimum(long *const rgb) {
  if (rgb[0] < rgb[1]) { return std::min(rgb[0], rgb[2]); }
  return std::min(rgb[1], rgb[2]);
}
}  // namespace

/* hsv_gradient_factory */
void hsv_gradient_factory::fix_diff(long *diff) {
  if (diff[0] > SCALE180) {
    diff[0] -= SCALE360;
  } else if (diff[0] < -SCALE180) {
    diff[0] += SCALE360;
  }
}

void hsv_gradient_factory::convert_from_scaled_rgb(long *const scaled,
                                                   long *target) {
  auto value = get_value(scaled);
  auto minimum = get_minimum(scaled);
  auto chroma = value - minimum;
  auto saturation = (SCALE360 * chroma) / value;

  target[0] = get_hue(scaled, chroma, value);
  target[1] = saturation;
  target[2] = value * 360L;
}

void hsv_gradient_factory::convert_to_scaled_rgb(long *const target,
                                                 long *scaled) {
  auto hue = target[0] % SCALE360;
  auto saturation = target[1] / 360L;
  auto value = target[2] / 360L;
  auto chroma = (saturation * value) / SCALE;
  auto x = get_intermediate(hue, chroma);

  scaled[0] = scaled[1] = scaled[2] = (value - chroma);

  if (hue < SCALE60) {
    scaled[0] += chroma;
    scaled[1] += x;
  } else if (hue < SCALE120) {
    scaled[0] += x;
    scaled[1] += chroma;
  } else if (hue < SCALE180) {
    scaled[1] += chroma;
    scaled[2] += x;
  } else if (hue < SCALE240) {
    scaled[1] += x;
    scaled[2] += chroma;
  } else if (hue < SCALE300) {
    scaled[2] += chroma;
    scaled[0] += x;
  } else {
    scaled[2] += x;
    scaled[0] += chroma;
  }
}
/* hsv_gradient_factory */

namespace {
// Using Rec.2020 color space
// Y' = 0.2627 x R + 0.6780 x G + 0.0593 x B
long get_luma(long *const rgb) {
  return 360L * (2627L * rgb[0] + 6780L * rgb[1] + 593L * rgb[2]) / 10000L;
}

// Using Rec.2020 color space
// m = Y' - (0.2627 x R + 0.6780 x G + 0.0593 x B)
long get_minimum_from_luma(long luma, long r, long g, long b) {
  return luma - (2627L * r + 6780L * g + 593L * b) / 10000L;
}
}  // namespace

/* hcl_gradient_factory */
void hcl_gradient_factory::fix_diff(long *diff) {
  if (diff[0] > SCALE180) {
    diff[0] -= SCALE360;
  } else if (diff[0] < -SCALE180) {
    diff[0] += SCALE360;
  }
}

void hcl_gradient_factory::convert_from_scaled_rgb(long *const scaled,
                                                   long *target) {
  auto value = get_value(scaled);
  auto minimum = get_minimum(scaled);
  auto luma = get_luma(scaled);
  auto chroma = value - minimum;

  target[0] = get_hue(scaled, chroma, value);
  target[1] = chroma * 360L;
  target[2] = luma;
}

void hcl_gradient_factory::convert_to_scaled_rgb(long *const target,
                                                 long *scaled) {
  auto hue = target[0] % SCALE360;
  auto chroma = target[1] / 360L;
  auto luma = target[2] / 360L;

  auto x = get_intermediate(hue, chroma);
  long m;

  if (hue < SCALE60) {
    m = get_minimum_from_luma(luma, chroma, x, 0);
    scaled[0] = scaled[1] = scaled[2] = m;
    scaled[0] += chroma;
    scaled[1] += x;
  } else if (hue < SCALE120) {
    m = get_minimum_from_luma(luma, x, chroma, 0);
    scaled[0] = scaled[1] = scaled[2] = m;
    scaled[0] += x;
    scaled[1] += chroma;
  } else if (hue < SCALE180) {
    m = get_minimum_from_luma(luma, 0, chroma, x);
    scaled[0] = scaled[1] = scaled[2] = m;
    scaled[1] += chroma;
    scaled[2] += x;
  } else if (hue < SCALE240) {
    m = get_minimum_from_luma(luma, 0, x, chroma);
    scaled[0] = scaled[1] = scaled[2] = m;
    scaled[1] += x;
    scaled[2] += chroma;
  } else if (hue < SCALE300) {
    m = get_minimum_from_luma(luma, x, 0, chroma);
    scaled[0] = scaled[1] = scaled[2] = m;
    scaled[2] += chroma;
    scaled[0] += x;
  } else {
    m = get_minimum_from_luma(luma, chroma, 0, x);
    scaled[0] = scaled[1] = scaled[2] = m;
    scaled[2] += x;
    scaled[0] += chroma;
  }
}
/* hcl_gradient_factory */
}  // namespace conky
