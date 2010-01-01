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
 *   (see AUTHORS)
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

#include <logging.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include "conky.h"

void print_uid_name(struct text_object *obj, char *p, int p_max_size) {
	struct passwd *pw;
	uid_t uid;
	char* firstinvalid;

	errno = 0;
	uid = strtol(obj->data.s, &firstinvalid, 10);
	if (errno == 0 && obj->data.s != firstinvalid) {
		pw = getpwuid(uid);
		if(pw != NULL) {
			snprintf(p, p_max_size, "%s", pw->pw_name);
		} else {
			NORM_ERR("The uid %d doesn't exist", uid)
		}
	} else {
		NORM_ERR("$uid_name didn't receive a uid as argument")
	}
}

void print_gid_name(struct text_object *obj, char *p, int p_max_size) {
	struct group *grp;
	gid_t gid;
	char* firstinvalid;

	errno = 0;
	gid = strtol(obj->data.s, &firstinvalid, 10);
	if (errno == 0 && obj->data.s != firstinvalid) {
		grp = getgrgid(gid);
		if(grp != NULL) {
			snprintf(p, p_max_size, "%s", grp->gr_name);
		} else {
			NORM_ERR("The gid %d doesn't exist", gid)
		}
	} else {
		NORM_ERR("$gid_name didn't receive a gid as argument")
	}
}
