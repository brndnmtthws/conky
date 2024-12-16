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

#include <config.h>

#include "lua-config.hh"

#include "data-source.hh"
#include "setting.hh"

namespace conky {
void export_symbols(lua::state &l) {
  lua::stack_sentry s(l);
  l.checkstack(3);

  l.newtable();
  {
    export_data_sources(l);

    l.newtable();
    l.rawsetfield(-2, "config");
  }
  l.setglobal("conky");
}
}  // namespace conky
