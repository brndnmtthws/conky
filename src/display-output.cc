/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
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

#include "display-output.hh"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace conky {
namespace priv {
void out_to_x_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);
  
  Base::lua_setter(l, init);
  
  if (init && do_convert(l, -1).first) { init_X11(); }
  
  ++s;
}

void out_to_x_setting::cleanup(lua::state &l) {
  lua::stack_sentry s(l, -1);
  
  if (do_convert(l, -1).first) { deinit_X11(); }
  
  l.pop();
}

void colour_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);
  
  if (!out_to_x.get(l)) {
    // ignore if we're not using X
    l.replace(-2);
  } else {
    Base::lua_setter(l, init);
  }
  
  ++s;
}

typedef std::unordered_map<std::string, display_output_base *> display_outputs_t;

/*
 * We cannot construct this object statically, because order of object
 * construction in different modules is not defined, so register_source could be
 * called before this object is constructed. Therefore, we create it on the
 * first call to register_source.
 */
display_outputs_t *display_outputs;

}  // namespace

/*
 * The selected and active display output.
 */
std::vector<display_output_base *> active_display_outputs;

/*
 * the list of the only current output, when inside draw_text,
 * else we iterate over each active outputs.
 */
std::vector<conky::display_output_base *> current_display_outputs;

namespace priv {
void do_register_display_output(const std::string &name,
                                display_output_base *output) {
  struct display_output_constructor {
    display_output_constructor() { display_outputs = new display_outputs_t(); }
    ~display_output_constructor() {
      delete display_outputs;
      display_outputs = nullptr;
    }
  };
  static display_output_constructor constructor;

  bool inserted = display_outputs->insert({name, output}).second;
  if (not inserted) {
    throw std::logic_error("Display output with name '" + name +
                           "' already registered");
  }
}

}  // namespace priv

display_output_base::display_output_base(const std::string &name_)
    : name(name_), is_active(false), is_graphical(false), priority(-1) {
  priv::do_register_display_output(name, this);
}


disabled_display_output::disabled_display_output(
                                           const std::string &name,
                                           const std::string &define)
    : display_output_base(name) {
  priority = -2;
  // XXX some generic way of reporting errors? NORM_ERR?
  std::cerr << "Support for display output '" << name
            << "' has been disabled during compilation. Please recompile with '"
            << define << "'" << std::endl;
}

bool initialize_display_outputs() {
  std::vector<display_output_base *> outputs;
  outputs.reserve(display_outputs->size());

  for (auto &output : *display_outputs) {
    outputs.push_back(output.second);
  }
  // Sort display outputs by descending priority, to try graphical ones first.
  sort(outputs.begin(), outputs.end(), &display_output_base::priority_compare);

  int graphical_count = 0;

  for (auto output : outputs) {
    if (output->priority < 0)
      continue;
    std::cerr << "Testing display output '" << output->name
              << "'... " << std::endl;
    if (output->detect()) {
      std::cerr << "Detected display output '" << output->name
                << "'... " << std::endl;

      if (graphical_count && output->graphical())
        continue;

      // X11 init needs to draw, so we must add it to the list first.
      active_display_outputs.push_back(output);

      if (output->initialize()) {
        std::cerr << "Initialized display output '" << output->name
                  << "'... " << std::endl;

        output->is_active = true;
        if (output->graphical())
          graphical_count++;
        /*
         * We only support a single graphical display for now.
         * More than one text display (ncurses + http, ...) should be ok.
         */
        //if (graphical_count)
          //return true;
      } else {
        // failed, so remove from list
        active_display_outputs.pop_back();
      }
    }
  }
  if (active_display_outputs.size())
    return true;

  std::cerr << "Unable to find a usable display output." << std::endl;
  return false;
}

bool shutdown_display_outputs() {
  bool ret = true;
  for (auto output : active_display_outputs) {
    output->is_active = false;
    ret = output->shutdown();
  }
  active_display_outputs.clear();
  return ret;
}

}  // namespace conky


/*
 * The order of these settings cannot be completely arbitrary. Some of them
 * depend on others, and the setters are called in the order in which they are
 * defined. The order should be: display_name -> out_to_x -> everything colour
 * related
 *                          -> border_*, own_window_*, etc -> own_window ->
 * double_buffer ->  imlib_cache_size
 */

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
