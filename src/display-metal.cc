/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (C) 2018 Nickolas Pylarinos et al.
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
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "conky.h"
#include "llua.h"

/*
conky::simple_config_setting<alignment> text_alignment("alignment", BOTTOM_LEFT,
                                                       false);
conky::simple_config_setting<std::string> display_name("display", std::string(),
                                                       false);
conky::simple_config_setting<int> head_index("xinerama_head", 0, true);
priv::out_to_x_setting out_to_x;

priv::colour_setting color[10] = {{"color0", 0xffffff}, {"color1", 0xffffff},
    {"color2", 0xffffff}, {"color3", 0xffffff},
    {"color4", 0xffffff}, {"color5", 0xffffff},
    {"color6", 0xffffff}, {"color7", 0xffffff},
    {"color8", 0xffffff}, {"color9", 0xffffff}};
priv::colour_setting default_color("default_color", 0xffffff);
priv::colour_setting default_shade_color("default_shade_color", 0x000000);
priv::colour_setting default_outline_color("default_outline_color", 0x000000);

conky::range_config_setting<int> border_inner_margin(
                                                     "border_inner_margin", 0, std::numeric_limits<int>::max(), 3, true);
conky::range_config_setting<int> border_outer_margin(
                                                     "border_outer_margin", 0, std::numeric_limits<int>::max(), 1, true);
conky::range_config_setting<int> border_width("border_width", 0,
                                              std::numeric_limits<int>::max(),
                                              1, true);
*/
namespace conky {
namespace {
    
}  // namespace

namespace priv {


}  // namespace priv

#ifdef BUILD_METAL

display_output_metal::display_output_metal()
    : display_output_base("mtl") {
  is_graphical = true;
  priority = 2;
}

bool display_output_metal::detect() {
  if (out_to_x.get(*state)) {
    std::cerr << "Display output '" << name << "' enabled in config." << std::endl;
    return true;
  }
  return false;
}

bool display_output_metal::initialize() {
  return true;
}

bool display_output_metal::shutdown() {
  return false;
}

bool display_output_metal::main_loop_wait(double t) {
  return true;
}

void display_output_metal::sigterm_cleanup() {
}

void display_output_metal::cleanup() {
}

void display_output_metal::set_foreground_color(long c) {
}

int display_output_metal::calc_text_width(const char *s) {
}

void display_output_metal::draw_string_at(int x, int y, const char *s, int w) {
}

void display_output_metal::set_line_style(int w, bool solid) {
}

void display_output_metal::set_dashes(char *s) {
}

void display_output_metal::draw_line(int x1, int y1, int x2, int y2) {
}

void display_output_metal::draw_rect(int x, int y, int w, int h) {
}

void display_output_metal::fill_rect(int x, int y, int w, int h) {
}

void display_output_metal::draw_arc(int x, int y, int w, int h, int a1, int a2) {
}

void display_output_metal::move_win(int x, int y) {
}

void display_output_metal::end_draw_stuff() {
}

void display_output_metal::clear_text(int exposures) {
}

void display_output_metal::load_fonts(bool utf8) {
}

#endif /* BUILD_METAL */

}  // namespace conky

