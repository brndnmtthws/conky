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

#define PROCDIR	"/proc"
#define READERR	"Can't read '%s'"
#define STATENOTFOUND	"Can't find the process state in '%s'"
#define READSIZE 128
#define STATE_ENTRY "State:\t"

struct environ_data {
	char *file;
	char *var;
};

void scan_pid_chroot_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_chroot(struct text_object *obj, char *p, int p_max_size);

void scan_pid_cmdline_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_cmdline(struct text_object *obj, char *p, int p_max_size);

void scan_pid_cwd_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_cwd(struct text_object *obj, char *p, int p_max_size);

void scan_pid_environ_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_environ(struct text_object *obj, char *p, int p_max_size);

void free_pid_environ(struct text_object *obj);

void scan_pid_environ_list_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_environ_list(struct text_object *obj, char *p, int p_max_size);

void scan_pid_exe_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_exe(struct text_object *obj, char *p, int p_max_size);

void scan_pid_state_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_state(struct text_object *obj, char *p, int p_max_size);

void scan_pid_stderr_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_stderr(struct text_object *obj, char *p, int p_max_size);

void scan_pid_stdin_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_stdin(struct text_object *obj, char *p, int p_max_size);

void scan_pid_stdout_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_stdout(struct text_object *obj, char *p, int p_max_size);

void scan_pid_openfiles_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_pid_openfiles(struct text_object *obj, char *p, int p_max_size);
