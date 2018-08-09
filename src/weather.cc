/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#include <ctype.h>
#include <time.h>
#include <array>
#include <cmath>
#include <mutex>
#include <string>
#include "ccurl_thread.h"
#include "config.h"
#include "conky.h"
#include "logging.h"
#include "temphelper.h"
#include "text_object.h"
#include "weather.h"

void print_weather(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void) obj;
  strncpy(p ,"TODO...", p_max_size);
}
