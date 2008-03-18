/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al.
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
 * $Id$
 *
 */

#include "conky.h"
#include <utmp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void user_name(char **ptr) {
	const struct utmp *usr;
	char buf[512];

	setutent();
	while((usr=getutent())!=NULL) {
		if (usr->ut_type==USER_PROCESS) {
			strncat(buf, usr->ut_name, 9); strcat(buf, "\n");
		}
	}
	*ptr = buf;
}
static void user_num(int *ptr) {
	const struct utmp *usr;
	int users_num = 0;

	setutent();
	while ((usr=getutent())!=NULL) {
		if (usr->ut_type==USER_PROCESS) {
			++users_num;
		}
	}
	*ptr = users_num;
}
static void user_term(char **ptr) {
	const struct utmp *usr;
	char buf[512];

	setutent();
	while((usr=getutent())!=NULL) {
		if (usr->ut_type==USER_PROCESS) {
			strncat(buf, usr->ut_line, 13); strncat(buf, "\n", 3);
		}
	}
	*ptr = buf;
}
static void user_time(char **ptr) {
	const struct utmp *usr;
	time_t login, real, diff;
	struct tm *dtime;
	char buf[512] = "";
	char output[512] = "";

	setutent();
	while ((usr=getutent())!=NULL) {
		if (usr->ut_type==USER_PROCESS) {
			login=usr->ut_time;
			time(&real);
			diff = difftime(real, login);
			dtime = localtime(&diff);
			dtime->tm_year = dtime->tm_year-70;
			dtime->tm_mon = dtime->tm_mon-1;
			dtime->tm_mday = dtime->tm_mday-1;
			if(dtime->tm_year>0){strftime(buf,512,"%yy %mm %dd %Hh %Mm\n", dtime); goto end;}
			else if(dtime->tm_mon>0){strftime(buf,512,"%mm %dd %Hh %Mm\n", dtime); goto end;}
			else if(dtime->tm_mday>0){strftime(buf,512,"%dd %Hh %Mm\n", dtime); goto end;}
			else if(dtime->tm_hour>0){strftime(buf,512,"%Hh %Mm\n", dtime); goto end;}
			else if(dtime->tm_min>0){strftime(buf,512,"%Mm\n", dtime); goto end;}
end:
			strncat(output, buf, 512);
		}
	}
	*ptr = output;
}

static void users_alloc(struct information *ptr) {
	if (ptr->users.names == NULL) {
		ptr->users.names = malloc(TEXT_BUFFER_SIZE);

	}
	if (ptr->users.terms == NULL) {
		ptr->users.terms = malloc(TEXT_BUFFER_SIZE);
	}
	if (ptr->users.times == NULL) {
		ptr->users.times = malloc(TEXT_BUFFER_SIZE);
	}
}

void update_users() {
	struct information * current_info = &info;	
	char *temp;
	int t;
	users_alloc(current_info);
	user_name(&temp);
	if (temp!=NULL) {
		if (current_info->users.names) {
			free(current_info->users.names); current_info->users.names = 0;
		}
		current_info->users.names = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->users.names, temp, TEXT_BUFFER_SIZE);
	} else {
		if (current_info->users.names) {
			free(current_info->users.names); current_info->users.names = 0;
		}
		current_info->users.names = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->users.names, "broken", TEXT_BUFFER_SIZE);
	}
	user_num(&t);
	if (t!=0) {
		if (current_info->users.number) {
			current_info->users.number = 0;
		}
		current_info->users.number = t;
	} else {
		current_info->users.number = 0;
	}
	temp = "\0";
	user_term(&temp);
	if (temp!=NULL) {
		if (current_info->users.terms) {
			free(current_info->users.terms); current_info->users.terms = 0;
		}
		current_info->users.terms = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->users.terms, temp, TEXT_BUFFER_SIZE);
	} else {
		if (current_info->users.terms) {
			free(current_info->users.terms); current_info->users.terms = 0;
		}
		current_info->users.terms = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->users.terms, "broken", TEXT_BUFFER_SIZE);
	}
	user_time(&temp);
	if (temp!=NULL) {
		if (current_info->users.times) {
			free(current_info->users.times); current_info->users.times = 0;
		}
		current_info->users.times = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->users.times, temp, TEXT_BUFFER_SIZE);
	} else {
		if (current_info->users.times) {
			free(current_info->users.times); current_info->users.times = 0;
		}
		current_info->users.times = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->users.times, "broken", TEXT_BUFFER_SIZE);
	}
}
