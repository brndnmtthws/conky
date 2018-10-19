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

#include "conky.h"
#include "nc.h"
#include "display-file.hh"

#include <iostream>
#include <sstream>
#include <unordered_map>

/* filenames for output */
static conky::simple_config_setting<std::string> overwrite_file(
    "overwrite_file", std::string(), true);
static FILE *overwrite_fpointer = nullptr;
static conky::simple_config_setting<std::string> append_file("append_file",
                                                             std::string(),
                                                             true);
static FILE *append_fpointer = nullptr;

namespace conky {
namespace {

conky::display_output_file file_output("file");

}  // namespace

namespace priv {


}  // namespace priv

display_output_file::display_output_file(const std::string &name_)
    : display_output_base(name_) {
  // lowest priority, it's a fallback
  priority = 0;
}

bool display_output_file::detect() {
  if (static_cast<unsigned int>(!overwrite_file.get(*state).empty()) != 0u ||
      static_cast<unsigned int>(!append_file.get(*state).empty()) != 0u) {
    std::cerr << "Display output '" << name << "' enabled in config." << std::endl;
    return true;
  }
  return false;
}

bool display_output_file::initialize() {
  return true;
}

bool display_output_file::shutdown() {
  return true;
}

void display_output_file::draw_string(const char *s, int w) {
  if (overwrite_fpointer != nullptr) {
    fprintf(overwrite_fpointer, "%s\n", s);
  }
  if (append_fpointer != nullptr) {
    fprintf(append_fpointer, "%s\n", s);
  }
}

void display_output_file::begin_draw_stuff() {
  if (static_cast<unsigned int>(!overwrite_file.get(*state).empty()) != 0u) {
    overwrite_fpointer = fopen(overwrite_file.get(*state).c_str(), "we");
    if (overwrite_fpointer == nullptr) {
      NORM_ERR("Cannot overwrite '%s'", overwrite_file.get(*state).c_str());
    }
  }
  if (static_cast<unsigned int>(!append_file.get(*state).empty()) != 0u) {
    append_fpointer = fopen(append_file.get(*state).c_str(), "ae");
    if (append_fpointer == nullptr) {
      NORM_ERR("Cannot append to '%s'", append_file.get(*state).c_str());
    }
  }
}

void display_output_file::end_draw_stuff() {
  if (overwrite_fpointer != nullptr) {
    fclose(overwrite_fpointer);
    overwrite_fpointer = nullptr;
  }
  if (append_fpointer != nullptr) {
    fclose(append_fpointer);
    append_fpointer = nullptr;
  }
}

}  // namespace conky

