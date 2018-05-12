/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
 * Copyright (c) 2009 Yeon-Hyeong Yang <lbird94@gmail.com>
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
/* conky support for information from sony_laptop kernel module
 *   information from sony_laptop kernel module
 *   /sys/devices/platform/sony-laptop
 *   I mimicked the methods from ibm.c
 * Yeon-Hyeong Yang <lbird94@gmail.com> */

#ifndef _SONY_H
#define _SONY_H

void get_sony_fanspeed(struct text_object *, char *, int);

#endif /* _SONY_H */
