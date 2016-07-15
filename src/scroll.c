/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
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

struct scroll_data {
	char *text;
	unsigned int show;
	unsigned int step;
	unsigned int start;
	long resetcolor;
};

static int scroll_character_length(char c) {
	unsigned char uc = (unsigned char) c;
	int len = 0;

	if (!utf8_mode)
		return 1;

	if (c == -1)
		return 1;

	if ((uc & 0x80) == 0)
		return 1;

	for (len = 0; len < 7; ++len)
	{
		if ((uc & (0x80 >> len)) == 0) {
			break;
		}
	}

	return len;
}

/* should the byte be skipped if found in
   the begining of string to display? */
static int is_scroll_skip_byte(char c) {
	unsigned char uc = (unsigned char) c;

	// in single-byte encodings no bytes should be skipped
	if (!utf8_mode)
		return 0;

	return (uc & 0xc0) == 0x80;
}

void parse_scroll_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	struct scroll_data *sd;
	int n1 = 0, n2 = 0;

	sd = malloc(sizeof(struct scroll_data));
	memset(sd, 0, sizeof(struct scroll_data));

	sd->resetcolor = get_current_text_color();
	sd->step = 1;
	if (!arg || sscanf(arg, "%u %n", &sd->show, &n1) <= 0)
		CRIT_ERR(obj, free_at_crash, "scroll needs arguments: <length> [<step>] <text>");

	sscanf(arg + n1, "%u %n", &sd->step, &n2);
	if (*(arg + n1 + n2)) {
		n1 += n2;
	} else {
		sd->step = 1;
	}
	sd->text = malloc(strlen(arg + n1) + sd->show + 1);

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
	obj->sub = malloc(sizeof(struct text_object));
	extract_variable_text_internal(obj->sub, sd->text);

	obj->data.opaque = sd;
}

void print_scroll(struct text_object *obj, char *p, int p_max_size, struct information *cur)
{
	struct scroll_data *sd = obj->data.opaque;
	unsigned int j, k, colorchanges = 0, frontcolorchanges = 0, strend;
	unsigned int visibcolorchanges = 0;
	unsigned int visibleChars = 0;
	char *pwithcolors;
	char buf[max_user_text];
	size_t bufLength;
	char c;

	if (!sd)
		return;

	generate_text_internal(buf, max_user_text, *obj->sub, cur);
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

	bufLength = strnlen(buf, max_user_text);

	//no scrolling necessary if the length of the text to scroll is too short
	if (bufLength - colorchanges <= sd->show) {
		snprintf(p, p_max_size, "%s", buf);
		return;
	}

	//if length of text changed to shorter so the (sd->start) is already
	//outside of actual text then reset (sd->start)
	if (sd->start >= bufLength) {
		sd->start = 0;
	}

	//make sure a colorchange at the front is not part of the string we are going to show
	while(buf[sd->start] == SPECIAL_CHAR) {
		sd->start++;
	}

	while(is_scroll_skip_byte(*(buf + sd->start))) {
		sd->start++;
	}

	j = 0;
	while (visibleChars < sd->show) {
		c = p[j] = buf[sd->start + j];
		++j;
		if (0 == c) {
			// if end of string reached - fill remaining place with (k) spaces
			k = sd->show - visibleChars;

			// return back to '\0' in (p)
			--j;

			// overwrite '\0' and (k-1) following bytes
			do {
				p[j++] = ' ';
			} while (--k);

			// done!
			break;
		} else if (SPECIAL_CHAR == c) {
			++visibcolorchanges;
		} else {
			// get length of the character
			k = scroll_character_length(c);

			// copy whole character
			while (--k) {
				p[j] = buf[sd->start + j];
				++j;
			}

			++visibleChars;
		}
	}

	p[j] = 0;

	//count colorchanges in front of the visible part and place that many colorchanges in front of the visible part
	for(j = 0; j < sd->start; j++) {
		if(buf[j] == SPECIAL_CHAR) frontcolorchanges++;
	}
	pwithcolors=malloc(strlen(p) + 1 + colorchanges - visibcolorchanges);
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
	for (j = 0; j < sd->step; ++j) {
		sd->start += scroll_character_length(*(buf + sd->start));
	}
	if(buf[sd->start] == 0 || sd->start > bufLength){
		sd->start = 0;
	}
#ifdef X11
	//reset color when scroll is finished
	new_fg(p + strlen(p), sd->resetcolor);
#endif
}

void free_scroll(struct text_object *obj)
{
	struct scroll_data *sd = obj->data.opaque;

	if (!sd)
		return;

	if (sd->text)
		free(sd->text);
	if (obj->sub) {
		free_text_objects(obj->sub, 1);
		free(obj->sub);
		obj->sub = NULL;
	}
	free(obj->data.opaque);
	obj->data.opaque = NULL;
}
