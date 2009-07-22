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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_HEADTAIL_LINES 30
#define DEFAULT_MAX_HEADTAIL_USES 2

void tailstring(char *string, int endofstring, int wantedlines) {
	int i, linescounted = 0;

	string[endofstring] = 0;
	if(endofstring > 0) {
		if(string[endofstring-1] == '\n') {	//work with or without \n at end of file
			string[endofstring-1] = 0;
		}
		for(i = endofstring - 1; i >= 0 && linescounted < wantedlines; i--) {
			if(string[i] == '\n') {
				linescounted++;
			}
		}
		if(i > 0) {
			strfold(string, i+2);
		}
	}
}

void init_tailhead(const char* type, const char* arg, struct text_object *obj, void* free_at_crash) {
	unsigned int args;

	if(arg) {
		obj->data.headtail.logfile=malloc(DEFAULT_TEXT_BUFFER_SIZE);
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
	int fd, i, endofstring = 0, linescounted = 0;
	FILE *fp;
	struct stat st;

	//empty the buffer and reset the counter if we used it the max number of times
	if(obj->data.headtail.buffer && obj->data.headtail.current_use >= obj->data.headtail.max_uses - 1) {
		free(obj->data.headtail.buffer);
		obj->data.headtail.buffer = NULL;
		obj->data.headtail.current_use = 0;
	}
	//use the buffer if possible
	if(obj->data.headtail.buffer) {
		strcpy(p, obj->data.headtail.buffer);
		obj->data.headtail.current_use++;
	}else{	//otherwise find the needed data
		if(stat(obj->data.headtail.logfile, &st) == 0) {
			if (S_ISFIFO(st.st_mode)) {
				fd = open_fifo(obj->data.headtail.logfile, &obj->a);
				if(fd != -1) {
					if(strcmp(type, "head") == 0) {
						for(i = 0; linescounted < obj->data.headtail.wantedlines; i++) {
							read(fd, p + i, 1);
							if(p[i] == '\n') {
								linescounted++;
							}
						}
						p[i] = 0;
					} else if(strcmp(type, "tail") == 0) {
						i = read(fd, p, p_max_size - 1);
						tailstring(p, i, obj->data.headtail.wantedlines);
					} else {
						CRIT_ERR(NULL, NULL, "If you are seeing this then there is a bug in the code, report it !");
					}
				}
				close(fd);
			} else {
				fp = open_file(obj->data.headtail.logfile, &obj->a);
				if(fp != NULL) {
					if(strcmp(type, "head") == 0) {
						for(i = 0; i < obj->data.headtail.wantedlines; i++) {
							fgets(p + endofstring, p_max_size - endofstring, fp);
							endofstring = strlen(p);
						}
					} else if(strcmp(type, "tail") == 0) {
						fseek(fp, - p_max_size, SEEK_END);
						i = fread(p, 1, p_max_size - 1, fp);
						tailstring(p, i, obj->data.headtail.wantedlines);
					} else {
						CRIT_ERR(NULL, NULL, "If you are seeing this then there is a bug in the code, report it !");
					}
					fclose(fp);
				}
			}
			obj->data.headtail.buffer = strdup(p);
		} else {
			CRIT_ERR(NULL, NULL, "$%s can't find information about %s", type, obj->data.headtail.logfile);
		}
	}
	return;
}
