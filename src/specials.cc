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
 * Copyright (c) 2005-2016 Brenden Matthews, Philip Kovacs, et. al.
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
#include "colours.h"
#ifdef BUILD_X11
#include "fonts.h"
#endif /* BUILD_X11 */
#include "logging.h"
#include "nc.h"
#include "specials.h"
#include <math.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#include <algorithm>
#include <sstream>

struct special_t *specials = NULL;

int special_count;

namespace {
	conky::range_config_setting<int> default_bar_width("default_bar_width", 0,
										std::numeric_limits<int>::max(), 0, false);
	conky::range_config_setting<int> default_bar_height("default_bar_height", 0,
										std::numeric_limits<int>::max(), 6, false);

#ifdef BUILD_X11
	conky::range_config_setting<int> default_graph_width("default_graph_width", 0,
										std::numeric_limits<int>::max(), 0, false);
	conky::range_config_setting<int> default_graph_height("default_graph_height", 0,
										std::numeric_limits<int>::max(), 25, false);

	conky::range_config_setting<int> default_gauge_width("default_gauge_width", 0,
										std::numeric_limits<int>::max(), 40, false);
	conky::range_config_setting<int> default_gauge_height("default_gauge_height", 0,
										std::numeric_limits<int>::max(), 25, false);
#endif /* BUILD_X11 */

	conky::simple_config_setting<std::string> console_graph_ticks("console_graph_ticks", " ,_,=,#", false);
}

/* special data types flags */
#define SF_SCALED	(1 << 0)
#define SF_SHOWLOG	(1 << 1)

/*
 * Special data typedefs
 */

struct bar {
	char flags;
	int width, height;
	double scale;
};

struct gauge {
	char flags;
	int width, height;
	double scale;
};

struct graph {
	char flags;
	int width, height;
	unsigned int first_colour, last_colour;
	double scale;
	char tempgrad;
};

struct stippled_hr {
	int height, arg;
};

struct tab {
	int width, arg;
};

/*
 * Scanning arguments to various special text objects
 */

#ifdef BUILD_X11
const char *scan_gauge(struct text_object *obj, const char *args, double scale)
{
	struct gauge *g;

	g = (struct gauge *)malloc(sizeof(struct gauge));
	memset(g, 0, sizeof(struct gauge));

	/*width and height*/
	g->width = default_gauge_width.get(*state);
	g->height = default_gauge_height.get(*state);

	if (scale)
		g->scale = scale;
	else
		g->flags |= SF_SCALED;

	/* gauge's argument is either height or height,width */
	if (args) {
		int n = 0;

		if (sscanf(args, "%d,%d %n", &g->height, &g->width, &n) <= 1) {
			if (sscanf(args, "%d %n", &g->height, &n) == 2) {
				g->width = g->height; /*square gauge*/
			}
		}
		args += n;
	}

	obj->special_data = g;
	return args;
}
#endif

const char *scan_bar(struct text_object *obj, const char *args, double scale)
{
	struct bar *b;

	b = (struct bar *)malloc(sizeof(struct bar));
	memset(b, 0, sizeof(struct bar));

	/* zero width means all space that is available */
	b->width = default_bar_width.get(*state);
	b->height = default_bar_height.get(*state);

	if (scale)
		b->scale = scale;
	else
		b->flags |= SF_SCALED;

	/* bar's argument is either height or height,width */
	if (args) {
		int n = 0;

		if (sscanf(args, "%d,%d %n", &b->height, &b->width, &n) <= 1) {
			sscanf(args, "%d %n", &b->height, &n);
		}
		args += n;
	}

	obj->special_data = b;
	return args;
}

#ifdef BUILD_X11
void scan_font(struct text_object *obj, const char *args)
{
	if (args && *args)
		obj->data.s = strndup(args, DEFAULT_TEXT_BUFFER_SIZE);
}

/**
 * parses for [height,width] [color1 color2] [scale] [-t] [-l]
 *
 * -l will set the showlog flag, enabling logarithmic graph scales
 * -t will set the tempgrad member to true, enabling temperature gradient colors
 *
 * @param[out] obj  struct in which to save width, height and other options
 * @param[in]  args argument string to parse
 * @param[in]  defscale default scale if no scale argument given
 * @return string to the command argument, NULL if argument didn't start with
 *         a string, but a number or if invalid argument string
 **/
char *scan_graph(struct text_object *obj, const char *args, double defscale)
{
	struct graph *g;
	char buf[1024];
	memset(buf, 0, 1024);

	g = (struct graph *)malloc(sizeof(struct graph));
	memset(g, 0, sizeof(struct graph));
	obj->special_data = g;

	/* zero width means all space that is available */
	g->width = default_graph_width.get(*state);
	g->height = default_graph_height.get(*state);
	g->first_colour = 0;
	g->last_colour = 0;
	g->scale = defscale;
	g->tempgrad = FALSE;
	if (args) {
		/* set tempgrad to true, if '-t' specified.
		 * It doesn#t matter where the argument is exactly. */
		if (strstr(args, " " TEMPGRAD) || strncmp(args, TEMPGRAD, strlen(TEMPGRAD)) == 0) {
			g->tempgrad = TRUE;
		}
		/* set showlog-flag, if '-l' specified
		 * It doesn#t matter where the argument is exactly. */
		if (strstr(args, " " LOGGRAPH) || strncmp(args, LOGGRAPH, strlen(LOGGRAPH)) == 0) {
			g->flags |= SF_SHOWLOG;
		}

		/* all the following functions try to interpret the beginning of a
		 * a string with different formaters. If successfuly the return from
		 * this whole function */

		/* interpret the beginning(!) of the argument string as:
		 * '[height],[width] [color1] [color2] [scale]'
		 * This means parameters like -t and -l may not be in the beginning */
		if (sscanf(args, "%d,%d %x %x %lf", &g->height, &g->width, &g->first_colour, &g->last_colour, &g->scale) == 5) {
			return NULL;
		}
		/* [height],[width] [color1] [color2] */
		g->scale = defscale;
		if (sscanf(args, "%d,%d %x %x", &g->height, &g->width, &g->first_colour, &g->last_colour) == 4) {
			return NULL;
		}
		/* [command] [height],[width] [color1] [color2] [scale] */
		if (sscanf(args, "%1023s %d,%d %x %x %lf", buf, &g->height, &g->width, &g->first_colour, &g->last_colour, &g->scale) == 6) {
			return strndup(buf, text_buffer_size.get(*state));
		}
		g->scale = defscale;
		if (sscanf(args, "%1023s %d,%d %x %x", buf, &g->height, &g->width, &g->first_colour, &g->last_colour) == 5) {
			return strndup(buf, text_buffer_size.get(*state));
		}

		buf[0] = '\0';
		g->height = 25;
		g->width = 0;
		if (sscanf(args, "%x %x %lf", &g->first_colour, &g->last_colour, &g->scale) == 3) {
			return NULL;
		}
		g->scale = defscale;
		if (sscanf(args, "%x %x", &g->first_colour, &g->last_colour) == 2) {
			return NULL;
		}
		if (sscanf(args, "%1023s %x %x %lf", buf, &g->first_colour, &g->last_colour, &g->scale) == 4) {
			return strndup(buf, text_buffer_size.get(*state));
		}
		g->scale = defscale;
		if (sscanf(args, "%1023s %x %x", buf, &g->first_colour, &g->last_colour) == 3) {
			return strndup(buf, text_buffer_size.get(*state));
		}

		buf[0] = '\0';
		g->first_colour = 0;
		g->last_colour = 0;
		if (sscanf(args, "%d,%d %lf", &g->height, &g->width, &g->scale) == 3) {
			return NULL;
		}
		g->scale = defscale;
		if (sscanf(args, "%d,%d", &g->height, &g->width) == 2) {
			return NULL;
		}
		if (sscanf(args, "%1023s %d,%d %lf", buf, &g->height, &g->width, &g->scale) < 4) {
			g->scale = defscale;
			//TODO: check the return value and throw an error?
			sscanf(args, "%1023s %d,%d", buf, &g->height, &g->width);
		}

		/* escape quotes at end in case of execgraph */
		if (*buf == '"') {
			char *_ptr;
			size_t _size;
			if ((_ptr = const_cast<char*>(strrchr(args, '"')))) {
				_size = _ptr - args - 1;
			}
			_size = _size < 1024 ? _size : 1023;
			strncpy(buf, args + 1, _size);
			buf[_size] = 0;
		}

		return strndup(buf, text_buffer_size.get(*state));
	}

	if (buf[0] == '\0') {
		return NULL;
	} else {
		return strndup(buf, text_buffer_size.get(*state));
	}
}
#endif /* BUILD_X11 */

/*
 * Printing various special text objects
 */

struct special_t *new_special_t_node()
{
	special_t *newnode = new special_t;

	memset(newnode, 0, sizeof *newnode);
	return newnode;
}

/**
 * expands the current global linked list specials to special_count elements
 *
 * increases special_count
 * @param[out] buf is set to "\x01\x00" not sure why ???
 * @param[in]  t   special type enum, e.g. alignc, alignr, fg, bg, ...
 * @return pointer to the newly inserted special of type t
 **/
struct special_t *new_special(char *buf, enum special_types t)
{
	special_t* current;

	buf[0] = SPECIAL_CHAR;
	buf[1] = '\0';
	if(!specials)
		specials = new_special_t_node();
	current = specials;
	/* allocate special_count linked list elements */
	for(int i=0; i < special_count; i++) {
		if(current->next == NULL)
			current->next = new_special_t_node();
		current = current->next;
	}
	current->type = t;
	special_count++;
	return current;
}

void new_gauge_in_shell(struct text_object *obj, char *p, int p_max_size, double usage)
{
	static const char *gaugevals[] = { "_. ", "\\. ", " | ", " ./", " ._" };
	struct gauge *g = (struct gauge *)obj->special_data;

	snprintf(p, p_max_size, "%s", gaugevals[round_to_int(usage * 4 / g->scale)]);
}

#ifdef BUILD_X11
void new_gauge_in_x11(struct text_object *obj, char *buf, double usage)
{
	struct special_t *s = 0;
	struct gauge *g = (struct gauge *)obj->special_data;

	if (not out_to_x.get(*state))
		return;

	if (!g)
		return;

	s = new_special(buf, GAUGE);

	s->arg = usage;
	s->width = g->width;
	s->height = g->height;
	s->scale = g->scale;
}
#endif /* BUILD_X11 */

void new_gauge(struct text_object *obj, char *p, int p_max_size, double usage)
{
	struct gauge *g = (struct gauge *)obj->special_data;

	if (!p_max_size || !g)
		return;

	if (g->flags & SF_SCALED)
		g->scale = MAX(g->scale, usage);
	else
		usage = MIN(g->scale, usage);

#ifdef BUILD_X11
	if (out_to_x.get(*state))
		new_gauge_in_x11(obj, p, usage);
	else
#endif /* BUILD_X11 */
		new_gauge_in_shell(obj, p, p_max_size, usage);
}

#ifdef BUILD_X11
void new_font(struct text_object *obj, char *p, int p_max_size)
{
	struct special_t *s;
	int tmp = selected_font;

	if (not out_to_x.get(*state))
		return;

	if (!p_max_size)
		return;

	s = new_special(p, FONT);

	if (obj->data.s) {
		if (s->font_added >= (int)fonts.size() || !s->font_added
								|| obj->data.s != fonts[s->font_added].name ) {
			selected_font = s->font_added = add_font(obj->data.s);
			selected_font = tmp;
		}
	} else {
		selected_font = s->font_added = 0;
		selected_font = tmp;
	}
}

/**
 * Adds value f to graph possibly truncating and scaling the graph
 **/
static void graph_append(struct special_t *graph, double f, char showaslog)
{
	int i;

	/* do nothing if we don't even have a graph yet */
	if (!graph->graph) return;

	if (showaslog) {
#ifdef BUILD_MATH
		f = log10(f + 1);
#endif
	}

	if (!graph->scaled && f > graph->scale) {
		f = graph->scale;
	}

	/* shift all the data by 1 */
	for (i = graph->graph_allocated - 1; i > 0; i--) {
		graph->graph[i] = graph->graph[i - 1];
	}
	graph->graph[0] = f;	/* add new data */

	if(graph->scaled) {
		graph->scale = *std::max_element(graph->graph + 0, graph->graph + graph->graph_width);
		if(graph->scale < 1e-47) {
			/* avoid NaN's when the graph is all-zero (e.g. before the first update)
			 * there is nothing magical about 1e-47 here */
			graph->scale = 1e-47;
		}
	}
}

void new_graph_in_shell(struct special_t *s, char *buf, int buf_max_size)
{
	// Split config string on comma to avoid the hassle of dealing with the
	// idiosyncrasies of multi-byte unicode on different platforms.
	// TODO: Parse config string once and cache result.
	const std::string ticks = console_graph_ticks.get(*state);
	std::stringstream ss(ticks);
	std::string tickitem;
	std::vector<std::string> tickitems;
	while (std::getline(ss, tickitem, ',')) {
		tickitems.push_back(tickitem);
	}

	char *p = buf;
	char *buf_max = buf + (sizeof(char) * buf_max_size);
	double scale = (tickitems.size() - 1) / s->scale;
	for (int i = s->graph_allocated -1; i >= 0; i--) {
		const unsigned int v = round_to_int(s->graph[i] * scale);
		const char *tick = tickitems[v].c_str();
		size_t itemlen = tickitems[v].size();
		for (unsigned int j = 0; j < itemlen; j++) {
			*p++ = tick[j];
			if (p == buf_max) goto graph_buf_end;
		}
	}
graph_buf_end:
	*p = '\0';
}

/**
 * Creates a visual graph and/or appends val to the graph / plot
 *
 * @param[in] obj struct containing all relevant flags like width, height, ...
 * @param[in] buf buffer for ascii art graph in console
 * @param[in] buf_max_size maximum length of buf
 * @param[in] val value to plot i.e. to add to plot
 **/
void new_graph(struct text_object *obj, char *buf, int buf_max_size, double val)
{
	struct special_t *s = 0;
	struct graph *g = (struct graph *)obj->special_data;

	if (!g || !buf_max_size)
		return;

	s = new_special(buf, GRAPH);

	/* set graph (special) width to width in obj */
	s->width = g->width;
	if (s->width) s->graph_width = s->width;

	if (s->graph_width != s->graph_allocated) {
		double *graph = static_cast<double *>(realloc(s->graph, s->graph_width * sizeof(double)));
		DBGP("reallocing graph from %d to %d", s->graph_allocated, s->graph_width);
		if (!s->graph) {
			/* initialize */
			memset(graph, 0, s->graph_width * sizeof(double));
			s->scale = 100;
		} else {
			if (s->graph_width > s->graph_allocated) {
				/* initialize the new region */
				memset(graph + (s->graph_allocated * sizeof(double)), 0,
						(s->graph_width - s->graph_allocated) *
						sizeof(double));
			}
		}
		s->graph = graph;
		s->graph_allocated = s->graph_width;
	}
	s->height = g->height;
	s->first_colour = adjust_colours(g->first_colour);
	s->last_colour = adjust_colours(g->last_colour);
	if (g->scale != 0) {
		s->scaled = 0;
		s->scale = g->scale;
		s->show_scale = 0;
	} else {
		s->scaled = 1;
		s->scale = 1;
		s->show_scale = 1;
	}
	s->tempgrad = g->tempgrad;
#ifdef BUILD_MATH
	if (g->flags & SF_SHOWLOG) {
		s->scale = log10(s->scale + 1);
	}
#endif
	graph_append(s, val, g->flags);

	if (not out_to_x.get(*state))
		new_graph_in_shell(s, buf, buf_max_size);
}

void new_hr(struct text_object *obj, char *p, int p_max_size)
{
	if (not out_to_x.get(*state))
		return;

	if (!p_max_size)
		return;

	new_special(p, HORIZONTAL_LINE)->height = obj->data.l;
}

void scan_stippled_hr(struct text_object *obj, const char *arg)
{
	struct stippled_hr *sh;

	sh = (struct stippled_hr *)malloc(sizeof(struct stippled_hr));
	memset(sh, 0, sizeof(struct stippled_hr));

	sh->arg = stippled_borders.get(*state);
	sh->height = 1;

	if (arg) {
		if (sscanf(arg, "%d %d", &sh->arg, &sh->height) != 2) {
			sscanf(arg, "%d", &sh->height);
		}
	}
	if (sh->arg <= 0) {
		sh->arg = 1;
	}
	obj->special_data = sh;
}

void new_stippled_hr(struct text_object *obj, char *p, int p_max_size)
{
	struct special_t *s = 0;
	struct stippled_hr *sh = (struct stippled_hr *)obj->special_data;

	if (not out_to_x.get(*state))
		return;

	if (!sh || !p_max_size)
		return;

	s = new_special(p, STIPPLED_HR);

	s->height = sh->height;
	s->arg = sh->arg;
}
#endif /* BUILD_X11 */

void new_fg(struct text_object *obj, char *p, int p_max_size)
{
#ifdef BUILD_X11
	if (out_to_x.get(*state))
		new_special(p, FG)->arg = obj->data.l;
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
	if (out_to_ncurses.get(*state))
		new_special(p, FG)->arg = obj->data.l;
#endif /* BUILD_NCURSES */
	UNUSED(obj);
	UNUSED(p);
	UNUSED(p_max_size);
}

#ifdef BUILD_X11
void new_bg(struct text_object *obj, char *p, int p_max_size)
{
	if (not out_to_x.get(*state))
		return;

	if (!p_max_size)
		return;

	new_special(p, BG)->arg = obj->data.l;
}
#endif /* BUILD_X11 */

static void new_bar_in_shell(struct text_object *obj, char* buffer, int buf_max_size, double usage)
{
	struct bar *b = (struct bar *)obj->special_data;
	int width, i, scaledusage;

	if (!b)
		return;

	width = b->width;
	if (!width)
		width = DEFAULT_BAR_WIDTH_NO_X;

	if (width > buf_max_size)
		width = buf_max_size;

	scaledusage = round_to_int( usage * width / b->scale);

	for (i = 0; i < scaledusage; i++)
		buffer[i] = '#';

	for (; i < width; i++)
		buffer[i] = '_';

	buffer[i] = 0;
}

#ifdef BUILD_X11
static void new_bar_in_x11(struct text_object *obj, char *buf, double usage)
{
	struct special_t *s = 0;
	struct bar *b = (struct bar *)obj->special_data;

	if (not out_to_x.get(*state))
		return;

	if (!b)
		return;

	s = new_special(buf, BAR);

	s->arg = usage;
	s->width = b->width;
	s->height = b->height;
	s->scale = b->scale;
}
#endif /* BUILD_X11 */

/* usage is in range [0,255] */
void new_bar(struct text_object *obj, char *p, int p_max_size, double usage)
{
	struct bar *b = (struct bar *)obj->special_data;

	if (!p_max_size || !b)
		return;

	if (b->flags & SF_SCALED)
		b->scale = MAX(b->scale, usage);
	else
		usage = MIN(b->scale, usage);

#ifdef BUILD_X11
	if (out_to_x.get(*state))
		new_bar_in_x11(obj, p, usage);
	else
#endif /* BUILD_X11 */
		new_bar_in_shell(obj, p, p_max_size, usage);
}

void new_outline(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;
	new_special(p, OUTLINE)->arg = obj->data.l;
}

void new_offset(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;
	new_special(p, OFFSET)->arg = obj->data.l;
}

void new_voffset(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;
	new_special(p, VOFFSET)->arg = obj->data.l;
}

void new_alignr(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;
	new_special(p, ALIGNR)->arg = obj->data.l;
}

// A postive offset pushes the text further left
void new_alignc(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;
	new_special(p, ALIGNC)->arg = obj->data.l;
}

void new_goto(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;
	new_special(p, GOTO)->arg = obj->data.l;
}

void scan_tab(struct text_object *obj, const char *arg)
{
	struct tab *t;

	t = (struct tab *)malloc(sizeof(struct tab));
	memset(t, 0, sizeof(struct tab));

	t->width = 10;
	t->arg = 0;

	if (arg) {
		if (sscanf(arg, "%d %d", &t->width, &t->arg) != 2) {
			sscanf(arg, "%d", &t->arg);
		}
	}
	if (t->width <= 0) {
		t->width = 1;
	}
	obj->special_data = t;
}

void new_tab(struct text_object *obj, char *p, int p_max_size)
{
	struct special_t *s = 0;
	struct tab *t = (struct tab *)obj->special_data;

	if (!t || !p_max_size)
		return;

	s = new_special(p, TAB);
	s->width = t->width;
	s->arg = t->arg;
}
