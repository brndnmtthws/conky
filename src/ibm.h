/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _IBM_H
#define _IBM_H

void get_ibm_acpi_fan(struct text_object *, char *, int);
int get_ibm_acpi_temps(void);
void get_ibm_acpi_volume(struct text_object *, char *, int);
void get_ibm_acpi_brightness(struct text_object *, char *, int);
void get_ibm_acpi_thinklight(struct text_object *, char *, int);

void parse_ibm_temps_arg(struct text_object *, const char *);
void print_ibm_temps(struct text_object *, char *, int);

#endif /* _IBM_H */
