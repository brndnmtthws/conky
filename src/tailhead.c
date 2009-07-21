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
 */

#include "text_object.h"
#include "logging.h"

#define MAX_HEADTAIL_LINES 30
#define DEFAULT_MAX_HEADTAIL_USES 2

void init_tailhead(const char* type, const char* arg, struct text_object *obj, void* free_at_crash) {
	unsigned int args;

	if(arg) {
		obj->data.headtail.logfile=malloc(strlen(arg));
		obj->data.headtail.max_uses = DEFAULT_MAX_HEADTAIL_USES;
		args = sscanf(arg, "%s %d %d", obj->data.headtail.logfile, &obj->data.headtail.wantedlines, &obj->data.headtail.max_uses);
		if(args == 2 || args == 3) {
			if(obj->data.headtail.max_uses < 1) {
				free(obj->data.headtail.logfile);
				CRIT_ERR(obj, free_at_crash, "invalid arg for %s, next_check must be larger than 0", type);
			}
			if (obj->data.headtail.wantedlines > 0 && obj->data.headtail.wantedlines <= MAX_HEADTAIL_LINES) {
				to_real_path(obj->data.headtail.logfile, obj->data.headtail.logfile);
				obj->data.headtail.buffer = NULL;
				obj->data.headtail.current_use = 0;
			}else{
				free(obj->data.headtail.logfile);
				CRIT_ERR(obj, free_at_crash, "invalid arg for %s, number of lines must be between 1 and %d", type, MAX_HEADTAIL_LINES);
			}
		} else {
			free(obj->data.headtail.logfile);
			CRIT_ERR(obj, free_at_crash, "%s needs a file as 1st and a number of lines as 2nd argument", type);
		}
	} else {
		CRIT_ERR(obj, free_at_crash, "%s needs arguments", type);
	}
}

void print_tailhead(const char* type, struct text_object *obj, char *p, int p_max_size) {
	int i, endofstring = 0, linescounted = 0;
	FILE *fp;

	if(obj->data.headtail.buffer && obj->data.headtail.current_use >= obj->data.headtail.max_uses - 1) {
		free(obj->data.headtail.buffer);
		obj->data.headtail.buffer = NULL;
		obj->data.headtail.current_use = 0;
	}
	if(obj->data.headtail.buffer) {
		strcpy(p, obj->data.headtail.buffer);
		obj->data.headtail.current_use++;
	}else{
		fp = open_file(obj->data.headtail.logfile, &obj->a);
		if(fp != NULL) {
			if(strcmp(type,"head") == 0) {
				for(i = 0; i < obj->data.headtail.wantedlines; i++) {
					fgets(p + endofstring, p_max_size - endofstring, fp);
					endofstring = strlen(p);
				}
			} else if(strcmp(type,"tail") == 0) {
				fseek(fp, - p_max_size, SEEK_END);
				fread(p, 1, p_max_size, fp);
				p[p_max_size - 1] = 0;
				if(p[strlen(p)-1] == '\n') {	//work with or without \n at end of file
					p[strlen(p)-1] = 0;
				}
				for(i = strlen(p); i >= 0 && linescounted < obj->data.headtail.wantedlines; i--) {
					if(p[i] == '\n') {
						linescounted++;
					}
				}
				if(i > 0) {
					strfold(p, i+2);
				}
			} else {
				CRIT_ERR(NULL, NULL, "If you are seeing this then there is a bug in the code, report it !");
			}
			fclose(fp);
			obj->data.headtail.buffer = strdup(p);
		}
	}
	return;
}
