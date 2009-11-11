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
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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
#include "conky.h"
#include "proc.h"
#include <unistd.h>

void scan_pid_arg(struct text_object *obj, const char *arg, void* free_at_crash, const char *file)
{
	pid_t pid;

	if(sscanf(arg, "%d", &pid) == 1) {
		asprintf(&obj->data.s, PROCDIR "/%d/%s", pid, file);
	} else {
		CRIT_ERR(obj, free_at_crash, "${pid_cmdline pid}");
	}
}

void scan_pid_cmdline_arg(struct text_object *obj, const char *arg, void* free_at_crash)
{
	scan_pid_arg(obj, arg, free_at_crash, "cmdline");
}

void print_pid_cmdline(struct text_object *obj, char *p, int p_max_size)
{
	char buf[p_max_size];
	FILE* infofile;
	int i, bytes_read;

	infofile = fopen(obj->data.s, "r");
	if(infofile) {
		bytes_read = fread(buf, 1, p_max_size, infofile);
		for(i = 0; i < bytes_read-1; i++) {
			if(buf[i] == 0) {
				buf[i] = ' ';
			}
		}
		snprintf(p, p_max_size, "%s", buf);
		fclose(infofile);
	} else {
		NORM_ERR(READERR, obj->data.s);
	}
}

void scan_pid_cwd_arg(struct text_object *obj, const char *arg, void* free_at_crash)
{
	scan_pid_arg(obj, arg, free_at_crash, "cwd");
}

void print_pid_cwd(struct text_object *obj, char *p, int p_max_size)
{
	char buf[p_max_size];
	int bytes_read;

	memset(buf, 0, p_max_size);
	bytes_read = readlink(obj->data.s, buf, p_max_size);
	if(bytes_read != -1) {
		snprintf(p, p_max_size, "%s", buf);
	} else {
		NORM_ERR(READERR, obj->data.s);
	}
}
