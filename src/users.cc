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

#include "conky.h"
#include <utmp.h>
#include <time.h>
#include <unistd.h>

#define BUFLEN 512

static void user_name(char *ptr)
{
	const struct utmp *usr = 0;

	setutent();
	while ((usr = getutent()) != NULL) {
		if (usr->ut_type == USER_PROCESS) {
			if (strlen(ptr) + strlen(usr->ut_name) + 1 <= BUFLEN) {
				strncat(ptr, usr->ut_name, UT_NAMESIZE);
			}
		}
	}
}
static void user_num(int *ptr)
{
	const struct utmp *usr;
	int users_num = 0;

	setutent();
	while ((usr = getutent()) != NULL) {
		if (usr->ut_type == USER_PROCESS) {
			++users_num;
		}
	}
	*ptr = users_num;
}
static void user_term(char *ptr)
{
	const struct utmp *usr;

	setutent();
	while ((usr = getutent()) != NULL) {
		if (usr->ut_type == USER_PROCESS) {
			if (strlen(ptr) + strlen(usr->ut_line) + 1 <= BUFLEN) {
				strncat(ptr, usr->ut_line, UT_LINESIZE);
			}
		}
	}
}
static void user_time(char *ptr)
{
	const struct utmp *usr;
	time_t log_in, real, diff;
	char buf[BUFLEN] = "";

	setutent();
	while ((usr = getutent()) != NULL) {
		if (usr->ut_type == USER_PROCESS) {
			log_in = usr->ut_time;
			time(&real);
			diff = difftime(real, log_in);
			format_seconds(buf, BUFLEN, diff);
			if (strlen(ptr) + strlen(buf) + 1 <= BUFLEN) {
				strncat(ptr, buf, BUFLEN-strlen(ptr)-1);
			}
		}
	}
}
static void tty_user_time(char *ptr, char *tty)
{
	time_t real, diff, log_in;
	char buf[BUFLEN] = "";

	struct utmp *usr, line;

	setutent();
	strcpy(line.ut_line, tty);
	usr = getutline(&line);
	if (usr == NULL ) {
		return;
	}

	log_in = usr->ut_time;

	time(&real);
	diff = difftime(real, log_in);
	format_seconds(buf, BUFLEN, diff);
	strncpy(ptr, buf, BUFLEN-1);
}

static void users_alloc(struct information *ptr)
{
	if (ptr->users.names == NULL) {
		ptr->users.names = (char*)malloc(text_buffer_size);

	}
	if (ptr->users.terms == NULL) {
		ptr->users.terms = (char*)malloc(text_buffer_size);
	}
	if (ptr->users.times == NULL) {
		ptr->users.times = (char*)malloc(text_buffer_size);
	}
}

static void update_user_time(char *tty)
{
	struct information *current_info = &info;
	char temp[BUFLEN] = "";

	if (current_info->users.ctime == NULL) {
		current_info->users.ctime = (char*)malloc(text_buffer_size);
	}

	tty_user_time(temp, tty);

	if (temp != NULL) {
		if (current_info->users.ctime) {
			free(current_info->users.ctime);
			current_info->users.ctime = 0;
		}
		current_info->users.ctime = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.ctime, temp, text_buffer_size);
	} else {
		if (current_info->users.ctime) {
			free(current_info->users.ctime);
			current_info->users.ctime = 0;
		}
		current_info->users.ctime = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.ctime, "broken", text_buffer_size);
	}
}

void update_users(void)
{
	struct information *current_info = &info;
	char temp[BUFLEN] = "";
	int t;
	users_alloc(current_info);
	user_name(temp);
	if (temp != NULL) {
		if (current_info->users.names) {
			free(current_info->users.names);
			current_info->users.names = 0;
		}
		current_info->users.names = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.names, temp, text_buffer_size);
	} else {
		if (current_info->users.names) {
			free(current_info->users.names);
			current_info->users.names = 0;
		}
		current_info->users.names = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.names, "broken", text_buffer_size);
	}
	user_num(&t);
	if (t != 0) {
		if (current_info->users.number) {
			current_info->users.number = 0;
		}
		current_info->users.number = t;
	} else {
		current_info->users.number = 0;
	}
	temp[0] = 0;
	user_term(temp);
	if (temp != NULL) {
		if (current_info->users.terms) {
			free(current_info->users.terms);
			current_info->users.terms = 0;
		}
		current_info->users.terms = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.terms, temp, text_buffer_size);
	} else {
		if (current_info->users.terms) {
			free(current_info->users.terms);
			current_info->users.terms = 0;
		}
		current_info->users.terms = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.terms, "broken", text_buffer_size);
	}
	user_time(temp);
	if (temp != NULL) {
		if (current_info->users.times) {
			free(current_info->users.times);
			current_info->users.times = 0;
		}
		current_info->users.times = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.times, temp, text_buffer_size);
	} else {
		if (current_info->users.times) {
			free(current_info->users.times);
			current_info->users.times = 0;
		}
		current_info->users.times = (char*)malloc(text_buffer_size);
		strncpy(current_info->users.times, "broken", text_buffer_size);
	}
}

void print_user_names(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.users.names);
}

void print_user_terms(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.users.terms);
}

void print_user_times(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.users.times);
}

void print_user_time(struct text_object *obj, char *p, int p_max_size)
{
	update_user_time(obj->data.s);
	snprintf(p, p_max_size, "%s", info.users.ctime);
}

void print_user_number(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%d", info.users.number);
}

void free_user_names(struct text_object *obj)
{
	(void)obj;
	if (info.users.names) {
		free(info.users.names);
		info.users.names = 0;
	}
}

void free_user_terms(struct text_object *obj)
{
	(void)obj;
	if (info.users.terms) {
		free(info.users.terms);
		info.users.terms = 0;
	}
}

void free_user_times(struct text_object *obj)
{
	(void)obj;
	if (info.users.times) {
		free(info.users.times);
		info.users.times = 0;
	}
}

void free_user_time(struct text_object *obj)
{
	if (info.users.ctime) {
		free(info.users.ctime);
		info.users.ctime = 0;
	}
	if (obj->data.s)
		free(obj->data.s);
}
