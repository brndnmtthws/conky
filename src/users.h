/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _USERS_H
#define _USERS_H

#ifdef __cplusplus
extern "C" {
#endif

void update_users(void);

void print_user_names(struct text_object *, char *, int);
void print_user_terms(struct text_object *, char *, int);
void print_user_times(struct text_object *, char *, int);
void print_user_time(struct text_object *, char *, int);
void print_user_number(struct text_object *, char *, int);

void free_user_names(struct text_object *);
void free_user_terms(struct text_object *);
void free_user_times(struct text_object *);
void free_user_time(struct text_object *);

#ifdef __cplusplus
}
#endif

#endif /* _USERS_H */
