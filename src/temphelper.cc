/*
 *
 * temphelper.c:  aid in converting temperature units
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
#include "temphelper.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "conky.h"

template <>
conky::lua_traits<TEMP_UNIT>::Map conky::lua_traits<TEMP_UNIT>::map = {
    {"celsius", TEMP_CELSIUS}, {"fahrenheit", TEMP_FAHRENHEIT}};

static conky::simple_config_setting<TEMP_UNIT> output_unit("temperature_unit",
                                                           TEMP_CELSIUS, true);

static double fahrenheit_to_celsius(double n) { return ((n - 32) * 5 / 9); }

static double celsius_to_fahrenheit(double n) { return ((n * 9 / 5) + 32); }

static double convert_temp_output(double n, enum TEMP_UNIT input_unit) {
  if (input_unit == output_unit.get(*state)) {
    return n;
  }

  switch (output_unit.get(*state)) {
    case TEMP_CELSIUS:
      return fahrenheit_to_celsius(n);
    case TEMP_FAHRENHEIT:
      return celsius_to_fahrenheit(n);
  }
  /* NOT REACHED */
  return 0.0;
}

int temp_print(char *p, size_t p_max_size, double n,
               enum TEMP_UNIT input_unit) {
  int out;
  size_t plen;

  out = round_to_int_temp(convert_temp_output(n, input_unit));
  plen = spaced_print(p, p_max_size, "%d", 3, out);

  return static_cast<int>(!(plen >= p_max_size));
}
