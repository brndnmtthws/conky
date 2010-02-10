/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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
#include "core.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"
#include "timed-thread.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <mutex>

struct execi_data {
	double last_update;
	float interval;
	char *cmd;
	char *buffer;
	double data;
	timed_thread_ptr p_timed_thread;
	float barnum;
	execi_data() : last_update(0), interval(0), cmd(0), buffer(0), data(0), barnum(0) {}
};

/* FIXME: this will probably not work, since the variable is being reused
 * between different text objects. So when a process really hangs, it's PID
 * will be overwritten at the next iteration. */
pid_t childpid = 0;

//our own implementation of popen, the difference : the value of 'childpid' will be filled with
//the pid of the running 'command'. This is useful if want to kill it when it hangs while reading
//or writing to it. We have to kill it because pclose will wait until the process dies by itself
static FILE* pid_popen(const char *command, const char *mode, pid_t *child) {
	int ends[2];
	int parentend, childend;

	//by running pipe after the strcmp's we make sure that we don't have to create a pipe
	//and close the ends if mode is something illegal
	if(strcmp(mode, "r") == 0) {
		if(pipe(ends) != 0) {
			return NULL;
		}
		parentend = ends[0];
		childend = ends[1];
	} else if(strcmp(mode, "w") == 0) {
		if(pipe(ends) != 0) {
			return NULL;
		}
		parentend = ends[1];
		childend = ends[0];
	} else {
		return NULL;
	}
	*child = fork();
	if(*child == -1) {
		close(parentend);
		close(childend);
		return NULL;
	} else if(*child > 0) {
		close(childend);
		waitpid(*child, NULL, 0);
	} else {
		//don't read from both stdin and pipe or write to both stdout and pipe
		if(childend == ends[0]) {
			close(0);
		} else {
			close(1);
		}

		//by dupping childend, the returned fd will have close-on-exec turned off
		if (dup(childend) == -1)
			perror("dup()");

		execl("/bin/sh", "sh", "-c", command, (char *) NULL);
		_exit(EXIT_FAILURE); //child should die here, (normally execl will take care of this but it can fail)
	}
	return fdopen(parentend, mode);
}

//remove backspaced chars, example: "dog^H^H^Hcat" becomes "cat"
//string has to end with \0 and it's length should fit in a int
#define BACKSPACE 8
static void remove_deleted_chars(char *string){
	int i = 0;
	while(string[i] != 0){
		if(string[i] == BACKSPACE){
			if(i != 0){
				strcpy( &(string[i-1]), &(string[i+1]) );
				i--;
			}else strcpy( &(string[i]), &(string[i+1]) ); //necessary for ^H's at the start of a string
		}else i++;
	}
}

static inline double get_barnum(char *buf)
{
	char *c = buf;
	double barnum;

	while (*c) {
		if (*c == '\001') {
			*c = ' ';
		}
		c++;
	}

	if (sscanf(buf, "%lf", &barnum) == 0) {
		NORM_ERR("reading exec value failed (perhaps it's not the "
				"correct format?)");
		return 0.0;
	}
	if (barnum > 100.0 || barnum < 0.0) {
		NORM_ERR("your exec value is not between 0 and 100, "
				"therefore it will be ignored");
		return 0.0;
	}
	return barnum;
}

static inline void read_exec(const char *data, char *buf, const int size)
{
	FILE *fp;

	memset(buf, 0, size);

	if (!data)
		return;

	alarm(update_interval);
	fp = pid_popen(data, "r", &childpid);
	if(fp) {
		int length;

		length = fread(buf, 1, size, fp);
		pclose(fp);
		buf[std::min(length, size-1)] = '\0';
		if (length > 0 && buf[length - 1] == '\n') {
			buf[length - 1] = '\0';
		}
	} else {
		buf[0] = '\0';
	}
	alarm(0);
}

static double read_exec_barnum(const char *data)
{
	char *buf = NULL;
	double barnum;

	buf = (char*)malloc(text_buffer_size);

	read_exec(data, buf, text_buffer_size);
	barnum = get_barnum(buf);
	free(buf);

	return barnum;
}

static void threaded_exec(thread_handle &handle, struct text_object *obj)
{
	char *buff, *p2;
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;

	while (1) {
		buff = (char*)malloc(text_buffer_size);
		read_exec(ed->cmd, buff, text_buffer_size);
		p2 = buff;
		while (*p2) {
			if (*p2 == '\001') {
				*p2 = ' ';
			}
			p2++;
		}

		{
			std::lock_guard<std::mutex> lock(handle.mutex());
			if (ed->buffer)
				free(ed->buffer);
			ed->buffer = buff;
		}
		if (handle.test(0)) {
			return;
		}
	}
	/* never reached */
}

/* check the execi fields and return true if the given interval has passed */
static int time_to_update(struct execi_data *ed)
{
	if (!ed->interval)
		return 0;
	if (current_update_time - ed->last_update >= ed->interval)
		return 1;
	return 0;
}

void scan_exec_arg(struct text_object *obj, const char *arg)
{
	/* XXX: do real bar parsing here */
	scan_bar(obj, "", 100);
	obj->data.s = strndup(arg ? arg : "", text_buffer_size);
}

void scan_pre_exec_arg(struct text_object *obj, const char *arg)
{
	char buf[2048];

	read_exec(arg, buf, sizeof(buf));
	obj_be_plain_text(obj, buf);
}

void scan_execi_arg(struct text_object *obj, const char *arg)
{
	struct execi_data *ed;
	int n;

	ed = new execi_data;

	if (sscanf(arg, "%f %n", &ed->interval, &n) <= 0) {
		NORM_ERR("${execi* <interval> command}");
		free(ed);
		return;
	}
	ed->cmd = strndup(arg + n, text_buffer_size);
	obj->data.opaque = ed;
}

void scan_execi_bar_arg(struct text_object *obj, const char *arg)
{
	/* XXX: do real bar parsing here */
	scan_bar(obj, "", 100);
	scan_execi_arg(obj, arg);
}

void scan_execi_gauge_arg(struct text_object *obj, const char *arg)
{
	/* XXX: do real gauge parsing here */
	scan_gauge(obj, "", 100);
	scan_execi_arg(obj, arg);
}

#ifdef BUILD_X11
void scan_execgraph_arg(struct text_object *obj, const char *arg)
{
	struct execi_data *ed;
	char *buf;

	ed = new execi_data;
	memset(ed, 0, sizeof(struct execi_data));

	buf = scan_graph(obj, arg, 100);
	if (!buf) {
		NORM_ERR("missing command argument to execgraph object");
		return;
	}
	ed->cmd = buf;
	obj->data.opaque = ed;
}
#endif /* BUILD_X11 */

void print_exec(struct text_object *obj, char *p, int p_max_size)
{
	read_exec(obj->data.s, p, p_max_size);
	remove_deleted_chars(p);
}

void fill_p(char *buffer, struct text_object *obj, char *p, int p_max_size) {
	if(obj->parse == true) {
		struct text_object subroot;
		parse_conky_vars(&subroot, buffer, p, p_max_size);
		free_text_objects(&subroot);
	} else snprintf(p, p_max_size, "%s", buffer);
}

void print_execp(struct text_object *obj, char *p, int p_max_size)
{
	char *buf;

	buf = (char*)malloc(text_buffer_size);
	memset(buf, 0, text_buffer_size);

	read_exec(obj->data.s, buf, text_buffer_size);
	fill_p(buf, obj, p, p_max_size);
	free(buf);
}

void print_execi(struct text_object *obj, char *p, int p_max_size)
{
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;
	bool fillbuffer = true;

	if (!ed)
		return;

	if(obj->thread == true) {
		if (!ed->p_timed_thread) {
			/*
			 * note that we don't register this thread with the
			 * timed_thread list, because we destroy it manually
			 */
			ed->p_timed_thread = timed_thread::create(std::bind(threaded_exec, std::placeholders::_1, obj), ed->interval * 1000000, false);
			if (!ed->p_timed_thread) {
				NORM_ERR("Error creating texeci timed thread");
			}
			fillbuffer = false;
		} else {
			std::lock_guard<std::mutex> lock(ed->p_timed_thread->mutex());
		}
	} else {
		if (time_to_update(ed)) {
			if (!ed->buffer)
				ed->buffer = (char*)malloc(text_buffer_size);
			read_exec(ed->cmd, ed->buffer, text_buffer_size);
			ed->last_update = current_update_time;
		}
	}
	if(fillbuffer) fill_p(ed->buffer, obj, p, p_max_size);
}

double execbarval(struct text_object *obj)
{
	return read_exec_barnum(obj->data.s);
}

double execi_barval(struct text_object *obj)
{
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;

	if (!ed)
		return 0;

	if (time_to_update(ed)) {
		ed->barnum = read_exec_barnum(ed->cmd);
		ed->last_update = current_update_time;
	}
	return ed->barnum;
}

void free_exec(struct text_object *obj)
{
	if (obj->data.s) {
		free(obj->data.s);
		obj->data.s = NULL;
	}
}

void free_execi(struct text_object *obj)
{
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;

	if (!ed)
		return;

	if (ed->p_timed_thread) {
		ed->p_timed_thread.reset();
	}
	if (ed->cmd)
		free(ed->cmd);
	if (ed->buffer)
		free(ed->buffer);
	delete ed;
	obj->data.opaque = NULL;
}
