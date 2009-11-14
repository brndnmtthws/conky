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
#include <ctype.h>
#include <dirent.h>

char* readfile(char* filename, int* total_read, char showerror) {
	FILE* file;
	char* buf = NULL;
	int bytes_read;

	*total_read = 0;
	file = fopen(filename, "r");
	if(file) {
		do {
			buf = realloc(buf, *total_read + READSIZE + 1);
			bytes_read = fread(buf + *total_read, 1, READSIZE, file);
			*total_read += bytes_read;
			buf[*total_read] = 0;
		}while(bytes_read != 0);
		fclose(file);
	} else if(showerror != 0) {
		NORM_ERR(READERR, filename);
	}
	return buf;
}

void pid_readlink(char *file, char *p, int p_max_size)
{
	char buf[p_max_size];

	memset(buf, 0, p_max_size);
	if(readlink(file, buf, p_max_size) >= 0) {
		snprintf(p, p_max_size, "%s", buf);
	} else {
		NORM_ERR(READERR, file);
	}
}

struct ll_string {
	char *string;
	struct ll_string* next;
};

struct ll_string* addnode(struct ll_string* end, char* string) {
	struct ll_string* current = malloc(sizeof(struct ll_string));
	current->string = strdup(string);
	current->next = NULL;
	if(end != NULL) end->next = current;
	return current;
}

void freelist(struct ll_string* front) {
	if(front != NULL) {
		free(front->string);
		if(front->next != NULL) {
			freelist(front->next);
		}
		free(front);
	}
}

int inlist(struct ll_string* front, char* string) {
	struct ll_string* current;

	for(current = front; current != NULL; current = current->next) {
		if(strcmp(current->string, string) == 0) {
			return 1;
		}
	}
	return 0;
}

void print_pid_chroot(struct text_object *obj, char *p, int p_max_size) {
	char *buffer;

	asprintf(&buffer, PROCDIR "/%s/root", obj->data.s);
	pid_readlink(buffer, p, p_max_size);
	free(buffer);
}

void print_pid_cmdline(struct text_object *obj, char *p, int p_max_size)
{
	char* buf;
	int i, bytes_read;

	asprintf(&buf, PROCDIR "/%s/cmdline", obj->data.s);
	strcpy(obj->data.s, buf);
	free(buf);
	buf = readfile(obj->data.s, &bytes_read, 1);
	if(buf != NULL) {
		for(i = 0; i < bytes_read-1; i++) {
			if(buf[i] == 0) {
				buf[i] = ' ';
			}
		}
		snprintf(p, p_max_size, "%s", buf);
		free(buf);
	}
}

void print_pid_cwd(struct text_object *obj, char *p, int p_max_size)
{
	char buf[p_max_size];
	int bytes_read;

	sprintf(buf, PROCDIR "/%s/cwd", obj->data.s);
	strcpy(obj->data.s, buf);
	bytes_read = readlink(obj->data.s, buf, p_max_size);
	if(bytes_read != -1) {
		buf[bytes_read] = 0;
		snprintf(p, p_max_size, "%s", buf);
	} else {
		NORM_ERR(READERR, obj->data.s);
	}
}

void print_pid_environ(struct text_object *obj, char *p, int p_max_size)
{
	int i, total_read;
	pid_t pid;
	char *buf, *file, *var=strdup(obj->data.s);;

	if(sscanf(obj->data.s, "%d %s", &pid, var) == 2) {
		for(i = 0; var[i] != 0; i++) {
			var[i] = toupper(var[i]);
		}
		asprintf(&file, PROCDIR "/%d/environ", pid);
		buf = readfile(file, &total_read, 1);
		free(file);
		if(buf != NULL) {
			for(i = 0; i < total_read; i += strlen(buf + i) + 1) {
				if(strncmp(buf + i, var, strlen(var)) == 0 && *(buf + i + strlen(var)) == '=') {
					snprintf(p, p_max_size, "%s", buf + i + strlen(var) + 1);
					free(buf);
					free(var);
					return;
				}
			}
			free(buf);
		}
		free(var);
		*p = 0;
	}
}

void print_pid_environ_list(struct text_object *obj, char *p, int p_max_size)
{
	char *buf = NULL;
	char *buf2;
	int bytes_read, total_read;
	int i = 0;

	asprintf(&buf, PROCDIR "/%s/environ", obj->data.s);
	strcpy(obj->data.s, buf);
	free(buf);
	buf = readfile(obj->data.s, &total_read, 1);
	if(buf != NULL) {
		for(bytes_read = 0; bytes_read < total_read; buf[i-1] = ';') {
			buf2 = strdup(buf+bytes_read);
			bytes_read += strlen(buf2)+1;
			sscanf(buf2, "%[^=]", buf+i);
			free(buf2);
			i = strlen(buf) + 1;
		}
		buf[i-1] = 0;
		snprintf(p, p_max_size, "%s", buf);
		free(buf);
	}
}

void print_pid_exe(struct text_object *obj, char *p, int p_max_size) {
	char *buffer;

	asprintf(&buffer, PROCDIR "/%s/exe", obj->data.s);
	pid_readlink(buffer, p, p_max_size);
	free(buffer);
}

void print_pid_openfiles(struct text_object *obj, char *p, int p_max_size) {
	DIR* dir;
	struct dirent *entry;
	char buf[p_max_size];
	int length, totallength = 0;
	struct ll_string* files_front = NULL;
	struct ll_string* files_back = NULL;

	dir = opendir(obj->data.s);
	if(dir != NULL) {
		while ((entry = readdir(dir))) {
			if(entry->d_name[0] != '.') {
				snprintf(buf, p_max_size, "%s/%s", obj->data.s, entry->d_name);
				length = readlink(buf, buf, p_max_size);
				buf[length] = 0;
				if(inlist(files_front, buf) == 0) {
					files_back = addnode(files_back, buf);
					snprintf(p + totallength, p_max_size - totallength, "%s; " , buf);
					totallength += length + strlen("; ");
				}
				if(files_front == NULL) {
					files_front = files_back;
				}
			}
		}
		closedir(dir);
		freelist(files_front);
		p[totallength - strlen("; ")] = 0;
	} else {
		p[0] = 0;
	}
}

void print_pid_state(struct text_object *obj, char *p, int p_max_size) {
	char *begin, *end, *buf = NULL;
	int bytes_read;

	asprintf(&buf, PROCDIR "/%s/status", obj->data.s);
	strcpy(obj->data.s, buf);
	free(buf);
	buf = readfile(obj->data.s, &bytes_read, 1);
	if(buf != NULL) {
		begin = strstr(buf, STATE_ENTRY);
		if(begin != NULL) {
			begin += strlen(STATE_ENTRY) + 3;	// +3 will strip the char representing the short state and the space and '(' that follow
			end = strchr(begin, '\n');
			if(end != NULL) {
				*(end-1) = 0;
			}
			snprintf(p, p_max_size, "%s", begin);
		} else {
			NORM_ERR(STATENOTFOUND, obj->data.s);
		}
		free(buf);
	}
}

void print_pid_state_short(struct text_object *obj, char *p, int p_max_size) {
	char *begin, *buf = NULL;
	int bytes_read;

	asprintf(&buf, PROCDIR "/%s/status", obj->data.s);
	strcpy(obj->data.s, buf);
	free(buf);
	buf = readfile(obj->data.s, &bytes_read, 1);
	if(buf != NULL) {
		begin = strstr(buf, STATE_ENTRY);
		if(begin != NULL) {
			snprintf(p, p_max_size, "%c", *begin);
		} else {
			NORM_ERR(STATENOTFOUND, obj->data.s);
		}
		free(buf);
	}
}

void print_pid_stderr(struct text_object *obj, char *p, int p_max_size) {
	char *buffer;

	asprintf(&buffer, PROCDIR "/%s/fd/2", obj->data.s);
	pid_readlink(buffer, p, p_max_size);
	free(buffer);
}

void print_pid_stdin(struct text_object *obj, char *p, int p_max_size) {
	char *buffer;

	asprintf(&buffer, PROCDIR "/%s/fd/0", obj->data.s);
	pid_readlink(buffer, p, p_max_size);
	free(buffer);
}

void print_pid_stdout(struct text_object *obj, char *p, int p_max_size) {
	char *buffer;

	asprintf(&buffer, PROCDIR "/%s/fd/1", obj->data.s);
	pid_readlink(buffer, p, p_max_size);
	free(buffer);
}

void scan_cmdline_to_pid_arg(struct text_object *obj, const char *arg, void* free_at_crash) {
	unsigned int i;

	if(strlen(arg) > 0) {
		obj->data.s = strdup(arg);
		for(i = 0; obj->data.s[i] != 0; i++) {
			while(obj->data.s[i] == ' ' && obj->data.s[i + 1] == ' ') {
				memmove(obj->data.s + i, obj->data.s + i + 1, strlen(obj->data.s + i + 1) + 1);
			}
		}
		if(obj->data.s[i - 1] == ' ') {
			obj->data.s[i - 1] = 0;
		}
	} else {
		CRIT_ERR(obj, free_at_crash, "${cmdline_to_pid commandline}");
	}
}

void print_cmdline_to_pid(struct text_object *obj, char *p, int p_max_size) {
	DIR* dir;
	struct dirent *entry;
	char *filename, *buf;
	int bytes_read, i;

	dir = opendir(PROCDIR);
	if(dir != NULL) {
		while ((entry = readdir(dir))) {
			asprintf(&filename, PROCDIR "/%s/cmdline", entry->d_name);
			buf = readfile(filename, &bytes_read, 0);
			free(filename);
			if(buf != NULL) {
				for(i = 0; i < bytes_read - 1; i++) {
					if(buf[i] == 0) buf[i] = ' ';
				}
				if(strstr(buf, obj->data.s) != NULL) {
					snprintf(p, p_max_size, "%s", entry->d_name);
					free(buf);
					closedir(dir);
					return;
				}
				free(buf);
			}
		}
		closedir(dir);
	} else {
		NORM_ERR(READERR, PROCDIR);
	}
}
