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
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cmath>
#include <mutex>
#include "update-cb.hh"

namespace {
	class exec_cb: public conky::callback<std::string, std::string> {
		typedef conky::callback<std::string, std::string> Base;

	protected:
		virtual void work();

	public:
		exec_cb(uint32_t period, bool wait, const std::string &cmd)
			: Base(period, wait, Base::Tuple(cmd))
		{}
	};
}

struct execi_data {
	float interval;
	char *cmd;
	execi_data() : interval(0), cmd(0) {}
};

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
		close(parentend);

		//by dupping childend, the returned fd will have close-on-exec turned off
		if (dup(childend) == -1)
			perror("dup()");
		close(childend);

		execl("/bin/sh", "sh", "-c", command, (char *) NULL);
		_exit(EXIT_FAILURE); //child should die here, (normally execl will take care of this but it can fail)
	}
	return fdopen(parentend, mode);
}

void exec_cb::work()
{
	pid_t childpid;
	std::string buf;
	std::shared_ptr<FILE> fp;
	char b[0x1000];

	if(FILE *t = pid_popen(std::get<0>(tuple).c_str(), "r", &childpid))
		fp.reset(t, fclose);
	else
		return;

	while(!feof(fp.get()) && !ferror(fp.get())) {
		int length = fread(b, 1, sizeof b, fp.get());
		buf.append(b, length);
	}

	if(*buf.rbegin() == '\n')
		buf.resize(buf.size()-1);

	std::lock_guard<std::mutex> l(result_mutex);
	result = buf;
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

static inline double get_barnum(const char *buf)
{
	double barnum;

	if (sscanf(buf, "%lf", &barnum) != 1) {
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

void scan_exec_arg(struct text_object *obj, const char *arg)
{
	/* XXX: do real bar parsing here */
	scan_bar(obj, "", 100);
	obj->data.s = strndup(arg ? arg : "", text_buffer_size.get(*state));
}

void scan_execi_arg(struct text_object *obj, const char *arg)
{
	struct execi_data *ed;
	int n;

	ed = new execi_data;

	if (sscanf(arg, "%f %n", &ed->interval, &n) <= 0) {
		NORM_ERR("${execi* <interval> command}");
		delete ed;
		return;
	}
	ed->cmd = strndup(arg + n, text_buffer_size.get(*state));
	obj->data.opaque = ed;
}

void scan_execi_bar_arg(struct text_object *obj, const char *arg)
{
	/* XXX: do real bar parsing here */
	scan_bar(obj, "", 100);
	scan_execi_arg(obj, arg);
}

#ifdef BUILD_X11
void scan_execi_gauge_arg(struct text_object *obj, const char *arg)
{
	/* XXX: do real gauge parsing here */
	scan_gauge(obj, "", 100);
	scan_execi_arg(obj, arg);
}

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

void fill_p(const char *buffer, struct text_object *obj, char *p, int p_max_size) {
	if(obj->parse == true) {
		evaluate(buffer, p, p_max_size);
	} else snprintf(p, p_max_size, "%s", buffer);
	remove_deleted_chars(p);
}

void print_exec(struct text_object *obj, char *p, int p_max_size)
{
	auto cb = conky::register_cb<exec_cb>(1, true, obj->data.s);
	fill_p(cb->get_result_copy().c_str(), obj, p, p_max_size);
}

void print_execi(struct text_object *obj, char *p, int p_max_size)
{
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;

	if (!ed)
		return;

	uint32_t period = std::max(lround(ed->interval/active_update_interval()), 1l);

	auto cb = conky::register_cb<exec_cb>(period, !obj->thread, ed->cmd);

	fill_p(cb->get_result_copy().c_str(), obj, p, p_max_size);
}

double execbarval(struct text_object *obj)
{
	auto cb = conky::register_cb<exec_cb>(1, true, obj->data.s);
	return get_barnum(cb->get_result_copy().c_str());
}

double execi_barval(struct text_object *obj)
{
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;

	if (!ed)
		return 0;

	uint32_t period = std::max(lround(ed->interval/active_update_interval()), 1l);

	auto cb = conky::register_cb<exec_cb>(period, !obj->thread, ed->cmd);

	return get_barnum(cb->get_result_copy().c_str());
}

void free_exec(struct text_object *obj)
{
	free_and_zero(obj->data.s);
}

void free_execi(struct text_object *obj)
{
	struct execi_data *ed = (struct execi_data *)obj->data.opaque;

	if (!ed)
		return;

	free_and_zero(ed->cmd);
	delete ed;
	obj->data.opaque = NULL;
}
