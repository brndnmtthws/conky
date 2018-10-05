/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
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

#ifndef DISPLAY_HTTP_HH
#define DISPLAY_HTTP_HH

#include <limits>
#include <string>
#include <type_traits>

#include "luamm.hh"
#include "display-output.hh"

namespace conky {

/*
 * A base class for HTTP display output.
 */
class display_output_http : public display_output_base {
 public:
  explicit display_output_http();

  virtual ~display_output_http() {}

  // check if available and enabled in settings
  virtual bool detect();
  // connect to DISPLAY and other stuff
  virtual bool initialize();
  virtual bool shutdown();

  // drawing primitives
  virtual bool begin_draw_text();
  virtual bool end_draw_text();
  virtual bool draw_string(const char *s, int w);


  // HTTP-specific
 private:
  //std::string webpage;
  //struct MHD_Daemon *httpd;
};

}  // namespace conky

#endif /* DISPLAY_HTTP_HH */
