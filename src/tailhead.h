/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
#ifndef _TAILHEAD_H
#define _TAILHEAD_H

#include "text_object.h"

#define MAX_TAIL_LINES 100

enum tailhead_type {
	TAIL,
	HEAD,
};

#define init_tail_object(o, a) init_tailhead_object(TAIL, o, a)
#define init_head_object(o, a) init_tailhead_object(HEAD, o, a)

int init_tailhead_object(enum tailhead_type,
		struct text_object *, const char *);
int print_head_object(struct text_object *, char *, size_t);
int print_tail_object(struct text_object *, char *, size_t);

#endif /* _TAILHEAD_H */
