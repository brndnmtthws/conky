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
#include "x11.h"
#include <vector>

#define SCROLL_LEFT true
#define SCROLL_RIGHT false

struct scroll_data {
	char *text;
	unsigned int show;
	unsigned int step;
	signed int start;
	long resetcolor;
	bool direction;
};

void parse_scroll_arg(struct text_object *obj, const char *arg, void *free_at_crash, char *free_at_crash2)
{
	struct scroll_data *sd;
	int n1 = 0, n2 = 0;
	char dirarg[6];

	sd = (struct scroll_data *)malloc(sizeof(struct scroll_data));
	memset(sd, 0, sizeof(struct scroll_data));

	sd->resetcolor = get_current_text_color();
	sd->step = 1;
	sd->direction = SCROLL_LEFT;

	if (arg && sscanf(arg, "%5s %n", dirarg, &n1) == 1) {
		if (strcasecmp(dirarg, "right") == 0 || strcasecmp(dirarg, "r") == 0)
			sd->direction = SCROLL_RIGHT;
		else if ( strcasecmp(dirarg, "left") != 0 && strcasecmp(dirarg, "l") != 0)
			n1 = 0;
	}

	if (!arg || sscanf(arg + n1, "%u %n", &sd->show, &n2) <= 0) {
		free(sd);
#ifdef BUILD_X11
		free(obj->next);
#endif
		free(free_at_crash2);
		CRIT_ERR(obj, free_at_crash, "scroll needs arguments: [left|right] <length> [<step>] <text>");
	}
	n1 += n2;

	if(sscanf(arg + n1, "%u %n", &sd->step, &n2) == 1) {
		n1 += n2;
	} else {
		sd->step = 1;
	}
	sd->text = (char*)malloc(strlen(arg + n1) + sd->show + 1);

	if (strlen(arg) > sd->show) {
		for(n2 = 0; (unsigned int) n2 < sd->show; n2++) {
		    sd->text[n2] = ' ';
		}
		sd->text[n2] = 0;
	}
	else
	    sd->text[0] = 0;

	strcat(sd->text, arg + n1);
	sd->start = 0;
	obj->sub = (struct text_object *)malloc(sizeof(struct text_object));
	extract_variable_text_internal(obj->sub, sd->text);

	obj->data.opaque = sd;

#ifdef BUILD_X11
	/* add a color object right after scroll to reset any color changes */
#endif /* BUILD_X11 */
}

void print_scroll(struct text_object *obj, char *p, int p_max_size)
{
	struct scroll_data *sd = (struct scroll_data *)obj->data.opaque;
	unsigned int j, colorchanges = 0, frontcolorchanges = 0, visibcolorchanges = 0, strend;
	char *pwithcolors;
	std::vector<char> buf(max_user_text.get(*state));

	if (!sd)
		return;

	generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
	for(j = 0; buf[j] != 0; j++) {
		switch(buf[j]) {
			case '\n':	//place all the lines behind each other with LINESEPARATOR between them
#define LINESEPARATOR '|'
				buf[j]=LINESEPARATOR;
				break;
			case SPECIAL_CHAR:
				colorchanges++;
				break;
		}
	}
	//no scrolling necessary if the length of the text to scroll is too short
	if (strlen(&(buf[0])) - colorchanges <= sd->show) {
		snprintf(p, p_max_size, "%s", &(buf[0]));
		return;
	}
	//make sure a colorchange at the front is not part of the string we are going to show
	while(buf[sd->start] == SPECIAL_CHAR) {
		sd->start++;
	}
	//place all chars that should be visible in p, including colorchanges
	for(j=0; j < sd->show + visibcolorchanges; j++) {
		p[j] = buf[sd->start + j];
		if(p[j] == SPECIAL_CHAR) {
			visibcolorchanges++;
		}
		//if there is still room fill it with spaces
		if( ! p[j]) break;
	}
	for(; j < sd->show + visibcolorchanges; j++) {
		p[j] = ' ';
	}
	p[j] = 0;
	//count colorchanges in front of the visible part and place that many colorchanges in front of the visible part
	for(j = 0; j < (unsigned) sd->start; j++) {
		if(buf[j] == SPECIAL_CHAR) frontcolorchanges++;
	}
	pwithcolors=(char*)malloc(strlen(p) + 4 + colorchanges - visibcolorchanges);
	for(j = 0; j < frontcolorchanges; j++) {
		pwithcolors[j] = SPECIAL_CHAR;
	}
	pwithcolors[j] = 0;
	strcat(pwithcolors,p);
	strend = strlen(pwithcolors);
	//and place the colorchanges not in front or in the visible part behind the visible part
	for(j = 0; j < colorchanges - frontcolorchanges - visibcolorchanges; j++) {
		pwithcolors[strend + j] = SPECIAL_CHAR;
	}
	pwithcolors[strend + j] = 0;
	strcpy(p, pwithcolors);
	free(pwithcolors);
	//scroll
	if(sd->direction == SCROLL_LEFT) {
		sd->start += sd->step;
		if(buf[sd->start] == 0 || (unsigned) sd->start > strlen(&(buf[0]))) {
			sd->start = 0;
		}
	} else {
		if(sd->start < 1) {
			sd->start = strlen(&(buf[0]));
		}
		sd->start -= sd->step;
	}
#ifdef BUILD_X11
	//reset color when scroll is finished
	if (out_to_x.get(*state))
		new_special(p + strlen(p), FG)->arg = sd->resetcolor;
#endif
}

void free_scroll(struct text_object *obj)
{
	struct scroll_data *sd = (struct scroll_data *)obj->data.opaque;

	if (!sd)
		return;

	free_and_zero(sd->text);
	free_text_objects(obj->sub);
	free_and_zero(obj->sub);
	free_and_zero(obj->data.opaque);
}
