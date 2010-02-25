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

#ifndef CONKY_PROC_H
#define CONKY_PROC_H

#define PROCDIR	"/proc"
#define READERR	"Can't read '%s'"
#define READSIZE 128

void print_pid_chroot(struct text_object *obj, char *p, int p_max_size);
void print_pid_cmdline(struct text_object *obj, char *p, int p_max_size);
void print_pid_cwd(struct text_object *obj, char *p, int p_max_size);
void print_pid_environ(struct text_object *obj, char *p, int p_max_size);
void print_pid_environ_list(struct text_object *obj, char *p, int p_max_size);
void print_pid_exe(struct text_object *obj, char *p, int p_max_size);
void print_pid_nice(struct text_object *obj, char *p, int p_max_size);
void print_pid_openfiles(struct text_object *obj, char *p, int p_max_size);
void print_pid_parent(struct text_object *obj, char *p, int p_max_size);
void print_pid_priority(struct text_object *obj, char *p, int p_max_size);
void print_pid_state(struct text_object *obj, char *p, int p_max_size);
void print_pid_state_short(struct text_object *obj, char *p, int p_max_size);
void print_pid_stderr(struct text_object *obj, char *p, int p_max_size);
void print_pid_stdin(struct text_object *obj, char *p, int p_max_size);
void print_pid_stdout(struct text_object *obj, char *p, int p_max_size);
void print_pid_threads(struct text_object *obj, char *p, int p_max_size);
void print_pid_thread_list(struct text_object *obj, char *p, int p_max_size);
void print_pid_time_kernelmode(struct text_object *obj, char *p, int p_max_size);
void print_pid_time_usermode(struct text_object *obj, char *p, int p_max_size);
void print_pid_time(struct text_object *obj, char *p, int p_max_size);
void print_pid_uid(struct text_object *obj, char *p, int p_max_size);
void print_pid_euid(struct text_object *obj, char *p, int p_max_size);
void print_pid_suid(struct text_object *obj, char *p, int p_max_size);
void print_pid_fsuid(struct text_object *obj, char *p, int p_max_size);
void print_pid_gid(struct text_object *obj, char *p, int p_max_size);
void print_pid_egid(struct text_object *obj, char *p, int p_max_size);
void print_pid_sgid(struct text_object *obj, char *p, int p_max_size);
void print_pid_fsgid(struct text_object *obj, char *p, int p_max_size);
void print_pid_read(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmpeak(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmsize(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmlck(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmhwm(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmrss(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmdata(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmstk(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmexe(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmlib(struct text_object *obj, char *p, int p_max_size);
void print_pid_vmpte(struct text_object *obj, char *p, int p_max_size);
void print_pid_write(struct text_object *obj, char *p, int p_max_size);

void scan_cmdline_to_pid_arg(struct text_object *obj, const char *arg, void* free_at_crash);
void print_cmdline_to_pid(struct text_object *obj, char *p, int p_max_size);

#endif /* CONKY_PROC_H */
