/* temphelper.c:  aid in converting temperature units
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
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "temphelper.h"
#include "conky.h"

/* default to output in celsius */
static enum TEMP_UNIT output_unit = TEMP_CELSIUS;

static double
fahrenheit_to_celsius(double n)
{
	return ((n - 32) * 5 / 9);
}

static double
celsius_to_fahrenheit(double n)
{
	return ((n * 9 / 5) + 32);
}

int
set_temp_output_unit(const char *name)
{
	size_t i;
	int rc = 0;
	char *buf;

	if (!name)
		return 1;

	buf = strdup(name);
	for (i = 0; i < strlen(name); i++)
		buf[i] = tolower(name[i]);

	if (!strcmp(buf, "celsius"))
		output_unit = TEMP_CELSIUS;
	else if (!strcmp(buf, "fahrenheit"))
		output_unit = TEMP_FAHRENHEIT;
	else
		rc = 1;
	free(buf);
	return rc;
}

static double
convert_temp_output(double n, enum TEMP_UNIT input_unit)
{
	if (input_unit == output_unit)
		return n;

	switch(output_unit) {
		case TEMP_CELSIUS:
			return fahrenheit_to_celsius(n);
		case TEMP_FAHRENHEIT:
			return celsius_to_fahrenheit(n);
	}
	/* NOT REACHED */
	return 0.0;
}

int temp_print(char *p, size_t p_max_size, double n, enum TEMP_UNIT input_unit)
{
	double out, plen;

	out = convert_temp_output(n, input_unit);
	plen = spaced_print(p, p_max_size, "%.lf", 5, out);

	return !(plen >= p_max_size);
}
