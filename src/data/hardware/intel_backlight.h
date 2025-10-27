/*
 *
 * Conky, a system monitor, based on torsmo.
 *
 * Any original torsmo code is licensed under the BSD license.
 * All code written since the fork of torsmo is licensed under the GPL.
 * Please see COPYING for details.
 *
 * Copyright (c) 2021 Rogier Reerink
 *  (See AUTHORS)
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef _INTEL_BACKLIGHT_H
#define _INTEL_BACKLIGHT_H

#include "../../conky.h"
#include "../../content/text_object.h"

void init_intel_backlight(struct text_object *obj);
void free_intel_backlight(struct text_object *obj);
void print_intel_backlight(struct text_object *obj, char *p,
                           unsigned int p_max_size);

#endif /* _INTEL_BACKLIGHT_H */