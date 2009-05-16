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
#include "conky.h"
#include "colours.h"
#ifdef X11
#include "fonts.h"
#endif
#include "logging.h"
#include "specials.h"
#include <math.h>

/* maximum number of special things, e.g. fonts, offsets, aligns, etc. */
unsigned int max_specials = MAX_SPECIALS_DEFAULT;

/* create specials array on heap instead of stack with introduction of
 * max_specials */
struct special_t *specials = NULL;

unsigned int special_count;

#ifdef X11
int default_bar_width = 0, default_bar_height = 6;
int default_graph_width = 0, default_graph_height = 25;
int default_gauge_width = 40, default_gauge_height = 25;
#endif

/*
 * Scanning arguments to various special text objects
 */

#ifdef X11
const char *scan_gauge(const char *args, int *w, int *h)
{
	/*width and height*/
	*w = default_gauge_width;
	*h = default_gauge_height;

	/* gauge's argument is either height or height,width */
	if (args) {
		int n = 0;

		if (sscanf(args, "%d,%d %n", h, w, &n) <= 1) {
			if (sscanf(args, "%d %n", h, &n) == 2) {
				*w = *h; /*square gauge*/
			}
		}
		args += n;
	}

	return args;
}

const char *scan_bar(const char *args, int *w, int *h)
{
	/* zero width means all space that is available */
	*w = default_bar_width;
	*h = default_bar_height;
	/* bar's argument is either height or height,width */
	if (args) {
		int n = 0;

		if (sscanf(args, "%d,%d %n", h, w, &n) <= 1) {
			sscanf(args, "%d %n", h, &n);
		}
		args += n;
	}

	return args;
}

char *scan_font(const char *args)
{
	if (args && *args) {
		return strndup(args, DEFAULT_TEXT_BUFFER_SIZE);
	}

	return NULL;
}

char *scan_graph(const char *args, int *w, int *h,
                 unsigned int *first_colour, unsigned int *last_colour,
                 unsigned int *scale, char *showaslog)
{
	const char *nographtype;
	char buf[64];
	buf[0] = 0;

	/* zero width means all space that is available */
	*w = default_graph_width;
	*h = default_graph_height;
	*first_colour = 0;
	*last_colour = 0;
	*scale = 0;
	if (args) {
		//set showaslog and place the rest of the args in nographtype
		if(strcasecmp(args, LOGGRAPH) == EQUAL) {
			*showaslog = TRUE;
			return NULL;
		}else if(strcasecmp(args, NORMGRAPH) == EQUAL) {
			*showaslog = FALSE;
			return NULL;
		}else if(strncasecmp(args, LOGGRAPH" ", strlen(LOGGRAPH) + 1 ) == EQUAL) {
			*showaslog = TRUE;
			nographtype = &args[strlen(LOGGRAPH) + 1];
		}else if(strncasecmp(args, NORMGRAPH" ", strlen(NORMGRAPH) + 1 ) == EQUAL) {
			*showaslog = FALSE;
			nographtype = &args[strlen(NORMGRAPH) + 1];
		}else{
			*showaslog = FALSE;
			nographtype = args;
		}
		DBGP("printing graph as %s, other args are: %s", (*showaslog ? "log" : "normal"), nographtype);
		//check the rest of the args
		if (sscanf(nographtype, "%d,%d %x %x %u", h, w, first_colour, last_colour, scale) == 5) {
			return NULL;
		}
		*scale = 0;
		if (sscanf(nographtype, "%d,%d %x %x", h, w, first_colour, last_colour) == 4) {
			return NULL;
		}
		if (sscanf(nographtype, "%63s %d,%d %x %x %u", buf, h, w, first_colour, last_colour, scale) == 6) {
			return strndup(buf, text_buffer_size);
		}
		*scale = 0;
		if (sscanf(nographtype, "%63s %d,%d %x %x", buf, h, w, first_colour, last_colour) == 5) {
			return strndup(buf, text_buffer_size);
		}
		buf[0] = '\0';
		*h = 25;
		*w = 0;
		if (sscanf(nographtype, "%x %x %u", first_colour, last_colour, scale) == 3) {
			return NULL;
		}
		*scale = 0;
		if (sscanf(nographtype, "%x %x", first_colour, last_colour) == 2) {
			return NULL;
		}
		if (sscanf(nographtype, "%63s %x %x %u", buf, first_colour, last_colour, scale) == 4) {
			return strndup(buf, text_buffer_size);
		}
		*scale = 0;
		if (sscanf(nographtype, "%63s %x %x", buf, first_colour, last_colour) == 3) {
			return strndup(buf, text_buffer_size);
		}
		buf[0] = '\0';
		*first_colour = 0;
		*last_colour = 0;
		if (sscanf(nographtype, "%d,%d %u", h, w, scale) == 3) {
			return NULL;
		}
		*scale = 0;
		if (sscanf(nographtype, "%d,%d", h, w) == 2) {
			return NULL;
		}
		if (sscanf(nographtype, "%63s %d,%d %u", buf, h, w, scale) < 4) {
			*scale = 0;
			//TODO: check the return value and throw an error?
			sscanf(nographtype, "%63s %d,%d", buf, h, w);
		}

		return strndup(buf, text_buffer_size);
	}

	if (buf[0] == '\0') {
		return NULL;
	} else {
		return strndup(buf, text_buffer_size);
	}
}
#endif

/*
 * Printing various special text objects
 */

static struct special_t *new_special(char *buf, enum special_types t)
{
	if (special_count >= max_specials) {
		CRIT_ERR("too many special things in text");
	}

	buf[0] = SPECIAL_CHAR;
	buf[1] = '\0';
	specials[special_count].type = t;
	return &specials[special_count++];
}

#ifdef X11
void new_gauge(char *buf, int w, int h, int usage)
{
	struct special_t *s = 0;
	if ((output_methods & TO_X) == 0)
		return;

	s = new_special(buf, GAUGE);

	s->arg = (usage > 255) ? 255 : ((usage < 0) ? 0 : usage);
	s->width = w;
	s->height = h;
}

void new_bar(char *buf, int w, int h, int usage)
{
	struct special_t *s = 0;

	if ((output_methods & TO_X) == 0)
		return;

	s = new_special(buf, BAR);

	s->arg = (usage > 255) ? 255 : ((usage < 0) ? 0 : usage);
	s->width = w;
	s->height = h;
}

void new_font(char *buf, char *args)
{
	if ((output_methods & TO_X) == 0)
		return;

	if (args) {
		struct special_t *s = new_special(buf, FONT);

		if (s->font_added > font_count || !s->font_added || (strncmp(args, fonts[s->font_added].name, DEFAULT_TEXT_BUFFER_SIZE) != EQUAL) ) {
			int tmp = selected_font;

			selected_font = s->font_added = addfont(args);
			load_fonts();
			selected_font = tmp;
		}
	} else {
		struct special_t *s = new_special(buf, FONT);
		int tmp = selected_font;

		selected_font = s->font_added = 0;
		selected_font = tmp;
	}
}

static void graph_append(struct special_t *graph, double f, char showaslog)
{
	int i;

	if (showaslog) {
#ifdef MATH
		f = log10(f + 1);
#endif
	}
	
	if (!graph->scaled && f > graph->graph_scale) {
		f = graph->graph_scale;
	}

/* Already happens in new_graph
	if (graph->scaled) {
		graph->graph_scale = 1;
	}
*/
	graph->graph[0] = f;	/* add new data */
	/* shift all the data by 1 */
	for (i = graph->graph_width - 1; i > 0; i--) {
		graph->graph[i] = graph->graph[i - 1];
		if (graph->scaled && graph->graph[i] > graph->graph_scale) {
			/* check if we need to update the scale */
			graph->graph_scale = graph->graph[i];
		}
	}
}

void new_graph(char *buf, int w, int h, unsigned int first_colour,
		unsigned int second_colour, double i, int scale, int append, char showaslog)
{
	struct special_t *s = 0;

	if ((output_methods & TO_X) == 0)
		return;

	s = new_special(buf, GRAPH);

	s->width = w;
	if (s->graph == NULL) {
		if (s->width > 0 && s->width < MAX_GRAPH_DEPTH) {
			// subtract 2 for the box
			s->graph_width = s->width /* - 2 */;
		} else {
			s->graph_width = MAX_GRAPH_DEPTH - 2;
		}
		s->graph = malloc(s->graph_width * sizeof(double));
		memset(s->graph, 0, s->graph_width * sizeof(double));
		s->graph_scale = 100;
	}
	s->height = h;
	s->first_colour = adjust_colors(first_colour);
	s->last_colour = adjust_colors(second_colour);
	if (scale != 0) {
		s->scaled = 0;
		s->graph_scale = scale;
		s->show_scale = 0;
	} else {
		s->scaled = 1;
		s->graph_scale = 1;
		s->show_scale = 1;
	}
	/* if (s->width) {
		s->graph_width = s->width - 2;	// subtract 2 for rectangle around
	} */
	if (showaslog) {
#ifdef MATH
		s->graph_scale = log10(s->graph_scale + 1);
#endif
	}
	if (append) {
		graph_append(s, i, showaslog);
	}
}

void new_hr(char *buf, int a)
{
	if ((output_methods & TO_X) == 0)
		return;

	new_special(buf, HORIZONTAL_LINE)->height = a;
}

void new_stippled_hr(char *buf, int a, int b)
{
	struct special_t *s = 0;

	if ((output_methods & TO_X) == 0)
		return;

	s = new_special(buf, STIPPLED_HR);

	s->height = b;
	s->arg = a;
}

void new_fg(char *buf, long c)
{
	if ((output_methods & TO_X) == 0)
		return;

	new_special(buf, FG)->arg = c;
}

void new_bg(char *buf, long c)
{
	if ((output_methods & TO_X) == 0)
		return;

	new_special(buf, BG)->arg = c;
}
#endif

void new_outline(char *buf, long c)
{
	new_special(buf, OUTLINE)->arg = c;
}

void new_offset(char *buf, long c)
{
	new_special(buf, OFFSET)->arg = c;
}

void new_voffset(char *buf, long c)
{
	new_special(buf, VOFFSET)->arg = c;
}

void new_alignr(char *buf, long c)
{
	new_special(buf, ALIGNR)->arg = c;
}

// A postive offset pushes the text further left
void new_alignc(char *buf, long c)
{
	new_special(buf, ALIGNC)->arg = c;
}

void new_goto(char *buf, long c)
{
	new_special(buf, GOTO)->arg = c;
}

void new_tab(char *buf, int a, int b)
{
	struct special_t *s = new_special(buf, TAB);

	s->width = a;
	s->arg = b;
}

