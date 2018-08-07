/*
 *
 * temphelper.h:  aid in converting temperature units
 *
 * Copyright (C) 2008 Phil Sutter <Phil@nwl.cc>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#ifndef TEMPHELPER_H
#define TEMPHELPER_H

#include <cstddef>

enum TEMP_UNIT { TEMP_CELSIUS, TEMP_FAHRENHEIT };

int temp_print(char *, std::size_t, double, enum TEMP_UNIT, int);

#endif /* TEMPHELPER_H */
