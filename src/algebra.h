/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */
#ifndef _ALGEBRA_H
#define _ALGEBRA_H

enum match_type {
	OP_LT = 1,	/* < */
	OP_GT = 2,	/* > */
	OP_EQ = 3,	/* == */
	OP_LEQ = 4,	/* <= */
	OP_GEQ = 5,	/* >= */
	OP_NEQ = 6,	/* != */
};

enum arg_type {
	ARG_STRING = 1, /* "asdf" */
	ARG_LONG = 2,	/* 123456 */
	ARG_DOUBLE = 3, /* 12.456 */
};

int compare(const char *);

#endif /* _ALGEBRA_H */
