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

#include "config.h"
#include "common.h"
#include "text_object.h"
#include "logging.h"
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define MAX_HEADTAIL_LINES 30
#define DEFAULT_MAX_HEADTAIL_USES 2

struct headtail {
	int wantedlines;
	char *logfile;
	char *buffer;
	int current_use;
	int max_uses;
	int reported;
};

static void tailstring(char *string, int endofstring, int wantedlines) {
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

void free_tailhead(struct text_object *obj)
{
	struct headtail *ht = obj->data.opaque;
	if (!ht)
		return;
	if (ht->logfile)
		free(ht->logfile);
	if (ht->buffer)
		free(ht->buffer);
	free(obj->data.opaque);
	obj->data.opaque = NULL;
}

void init_tailhead(const char* type, const char* arg, struct text_object *obj, void* free_at_crash) {
	unsigned int args;
	struct headtail *ht;

	ht = malloc(sizeof(struct headtail));
	memset(ht, 0, sizeof(struct headtail));

	ht->logfile = malloc(DEFAULT_TEXT_BUFFER_SIZE);
	memset(ht->logfile, 0, DEFAULT_TEXT_BUFFER_SIZE);

	ht->max_uses = DEFAULT_MAX_HEADTAIL_USES;

	args = sscanf(arg, "%s %d %d", ht->logfile, &ht->wantedlines, &ht->max_uses);
	if (args < 2 || args > 3) {
		free_tailhead(obj);
		CRIT_ERR(obj, free_at_crash, "%s needs a file as 1st and a number of lines as 2nd argument", type);
	}
	if (ht->max_uses < 1) {
		free_tailhead(obj);
		CRIT_ERR(obj, free_at_crash, "invalid arg for %s, next_check must be larger than 0", type);
	}
	if (ht->wantedlines > 0 && ht->wantedlines <= MAX_HEADTAIL_LINES) {
		to_real_path(ht->logfile, ht->logfile);
		ht->buffer = NULL;
		ht->current_use = 0;
	} else {
		free_tailhead(obj);
		CRIT_ERR(obj, free_at_crash, "invalid arg for %s, number of lines must be between 1 and %d", type, MAX_HEADTAIL_LINES);
	}
	obj->data.opaque = ht;
}

static void print_tailhead(const char* type, struct text_object *obj, char *p, int p_max_size) {
	int fd, i, endofstring = 0, linescounted = 0;
	FILE *fp;
	struct stat st;
	struct headtail *ht = obj->data.opaque;

	if (!ht)
		return;

	//empty the buffer and reset the counter if we used it the max number of times
	if(ht->buffer && ht->current_use >= ht->max_uses - 1) {
		free(ht->buffer);
		ht->buffer = NULL;
		ht->current_use = 0;
	}
	//use the buffer if possible
	if(ht->buffer) {
		strcpy(p, ht->buffer);
		ht->current_use++;
	}else{	//otherwise find the needed data
		if(stat(ht->logfile, &st) == 0) {
			if (S_ISFIFO(st.st_mode)) {
				fd = open_fifo(ht->logfile, &ht->reported);
				if(fd != -1) {
					if(strcmp(type, "head") == 0) {
						for(i = 0; linescounted < ht->wantedlines; i++) {
							read(fd, p + i, 1);
							if(p[i] == '\n') {
								linescounted++;
							}
						}
						p[i] = 0;
					} else if(strcmp(type, "tail") == 0) {
						i = read(fd, p, p_max_size - 1);
						tailstring(p, i, ht->wantedlines);
					} else {
						CRIT_ERR(NULL, NULL, "If you are seeing this then there is a bug in the code, report it !");
					}
				}
				close(fd);
			} else {
				fp = open_file(ht->logfile, &ht->reported);
				if(fp != NULL) {
					if(strcmp(type, "head") == 0) {
						for(i = 0; i < ht->wantedlines; i++) {
							fgets(p + endofstring, p_max_size - endofstring, fp);
							endofstring = strlen(p);
						}
					} else if(strcmp(type, "tail") == 0) {
						fseek(fp, - p_max_size, SEEK_END);
						i = fread(p, 1, p_max_size - 1, fp);
						tailstring(p, i, ht->wantedlines);
					} else {
						CRIT_ERR(NULL, NULL, "If you are seeing this then there is a bug in the code, report it !");
					}
					fclose(fp);
				}
			}
			ht->buffer = strdup(p);
		} else {
			CRIT_ERR(NULL, NULL, "$%s can't find information about %s", type, ht->logfile);
		}
	}
	return;
}

void print_head(struct text_object *obj, char *p, int p_max_size)
{
	print_tailhead("head", obj, p, p_max_size);
}

void print_tail(struct text_object *obj, char *p, int p_max_size)
{
	print_tailhead("tail", obj, p, p_max_size);
}

/* FIXME: use something more general (see also tail.c, head.c */
#define BUFSZ 0x1000

void print_lines(struct text_object *obj, char *p, int p_max_size)
{
	static int rep = 0;
	FILE *fp = open_file(obj->data.s, &rep);
	char buf[BUFSZ];
	int j, lines;

	if (!fp) {
		snprintf(p, p_max_size, "File Unreadable");
		return;
	}

	lines = 0;
	while(fgets(buf, BUFSZ, fp) != NULL){
		for(j = 0; buf[j] != 0; j++) {
			if(buf[j] == '\n') {
				lines++;
			}
		}
	}
	snprintf(p, p_max_size, "%d", lines);
	fclose(fp);
}

void print_words(struct text_object *obj, char *p, int p_max_size)
{
	static int rep = 0;
	FILE *fp = open_file(obj->data.s, &rep);
	char buf[BUFSZ];
	int j, words;
	char inword = 0;

	if(!fp) {
		snprintf(p, p_max_size, "File Unreadable");
		return;
	}

	words = 0;
	while(fgets(buf, BUFSZ, fp) != NULL){
		for(j = 0; buf[j] != 0; j++) {
			if(!isspace(buf[j])) {
				if(!inword) {
					words++;
					inword = 1;
				}
			} else {
				inword = 0;
			}
		}
	}
	snprintf(p, p_max_size, "%d", words);
	fclose(fp);
}
