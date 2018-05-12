/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2007 Toni Spets
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

#ifndef _I8K_H
#define _I8K_H

int update_i8k(void);
void print_i8k_left_fan_status(struct text_object *, char *, int);
void print_i8k_cpu_temp(struct text_object *, char *, int);
void print_i8k_right_fan_status(struct text_object *, char *, int);
void print_i8k_ac_status(struct text_object *, char *, int);
void print_i8k_version(struct text_object *, char *, int);
void print_i8k_bios(struct text_object *, char *, int);
void print_i8k_serial(struct text_object *, char *, int);
void print_i8k_left_fan_rpm(struct text_object *, char *, int);
void print_i8k_right_fan_rpm(struct text_object *, char *, int);
void print_i8k_buttons_status(struct text_object *, char *, int);

#endif /* _I8K_H */
