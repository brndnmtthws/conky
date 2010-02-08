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
#include "logging.h"
#include <ctype.h>
#include <stdlib.h>
#include <string>
/* The templates defined by the user.
 *
 * This is a 1 to 1 mapping from templateN config option to template[N] field. */
static char *_template[MAX_TEMPLATES];	//this is named _template because template is a reserved word in c++

/* free all templates
 *
 * On first invocation, just memset all pointers to zero, so this function can
 * be used when initialising data upon startup. */
void free_templates(void)
{
	int i;
	static int initialised = 0;

	if (!initialised) {
		memset(_template, 0, MAX_TEMPLATES * sizeof(char *));
		initialised = 1;
		return;
	}

	for (i = 0; i < MAX_TEMPLATES; i++) {
		if (_template[i]) {
			free(_template[i]);
			_template[i] = NULL;
		}
	}
}

/* set the value of template at index n
 *
 * Returns non-zero on illegal arguments passed, zero otherwise. */
int set_template(int n, const char *val)
{
	if (n < 0 || n >= MAX_TEMPLATES || !val)
		return 1;
	if (_template[n])
		free(_template[n]);
	_template[n] = strdup(val);
	return 0;
}

/* backslash_escape - do the actual substitution task for template objects
 *
 * The field templates is used for substituting the \N occurences. Set it to
 * NULL to leave them as they are.
 */
static char *backslash_escape(const char *src, char **templates, unsigned int template_count)
{
	char *src_dup;
	const char *p;
	unsigned int dup_idx = 0, dup_len;

	dup_len = strlen(src) + 1;
	src_dup = (char*) malloc(dup_len * sizeof(char));

	p = src;
	while (*p) {
		switch (*p) {
		case '\\':
			if (!*(p + 1))
				break;
			if (*(p + 1) == '\\') {
				src_dup[dup_idx++] = '\\';
				p++;
			} else if (*(p + 1) == ' ') {
				src_dup[dup_idx++] = ' ';
				p++;
			} else if (*(p + 1) == 'n') {
				src_dup[dup_idx++] = '\n';
				p++;
			} else if (templates) {
				unsigned int tmpl_num;
				int digits;
				if ((sscanf(p + 1, "%u%n", &tmpl_num, &digits) <= 0) ||
				    (tmpl_num > template_count))
					break;
				dup_len += strlen(templates[tmpl_num - 1]);
				src_dup = (char*) realloc(src_dup, dup_len * sizeof(char));
				sprintf(src_dup + dup_idx, "%s", templates[tmpl_num - 1]);
				dup_idx += strlen(templates[tmpl_num - 1]);
				p += digits;
			}
			break;
		default:
			src_dup[dup_idx++] = *p;
			break;
		}
		p++;
	}
	src_dup[dup_idx] = '\0';
	src_dup = (char*) realloc(src_dup, (strlen(src_dup) + 1) * sizeof(char));
	return src_dup;
}

/* handle_template_object - core logic of the template object
 *
 * use config variables like this:
 * template1 = "$\1\2"
 * template2 = "\1: ${fs_bar 4,100 \2} ${fs_used \2} / ${fs_size \2}"
 *
 * and use them like this:
 * ${template1 node name}
 * ${template2 root /}
 * ${template2 cdrom /mnt/cdrom}
 */
static char *handle_template(const char *tmpl, const char *args)
{
	char *args_dup = NULL;
	char *p, *p_old;
	char **argsp = NULL;
	unsigned int argcnt = 0, template_idx, i;
	char *eval_text;

	if ((sscanf(tmpl, "template%u", &template_idx) != 1) ||
	    (template_idx >= MAX_TEMPLATES))
		return NULL;

	if(args) {
		args_dup = strdup(args);
		p = args_dup;
		while (*p) {
			while (*p && (*p == ' ' && (p == args_dup || *(p - 1) != '\\')))
				p++;
			if (p > args_dup && *(p - 1) == '\\')
				p--;
			p_old = p;
			while (*p && (*p != ' ' || (p > args_dup && *(p - 1) == '\\')))
				p++;
			if (*p) {
				(*p) = '\0';
				p++;
			}
			argsp = (char**) realloc(argsp, ++argcnt * sizeof(char *));
			argsp[argcnt - 1] = p_old;
		}
		for (i = 0; i < argcnt; i++) {
			char *tmp;
			tmp = backslash_escape(argsp[i], NULL, 0);
			DBGP2("%s: substituted arg '%s' to '%s'", tmpl, argsp[i], tmp);
			argsp[i] = tmp;
		}
	}

	eval_text = backslash_escape(_template[template_idx], argsp, argcnt);
	DBGP("substituted %s, output is '%s'", tmpl, eval_text);
	free(args_dup);
	for (i = 0; i < argcnt; i++)
		free(argsp[i]);
	free(argsp);
	return eval_text;
}

/* Search inbuf and replace all found template object references
 * with the substituted value. */
char *find_and_replace_templates(const char *inbuf)
{
	char *outbuf, *indup, *p, *o, *templ, *args, *tmpl_out;
	int stack, outlen;

	outlen = strlen(inbuf) + 1;
	o = outbuf = (char*) calloc(outlen, sizeof(char));
	memset(outbuf, 0, outlen * sizeof(char));

	p = indup = strdup(inbuf);
	while (*p) {
		while (*p && *p != '$')
			*(o++) = *(p++);

		if (!(*p))
			break;

		if (strncmp(p, "$template", strlen("$template")) && strncmp(p, "${template", strlen("${template"))) {
			*(o++) = *(p++);
			continue;
		}

		if (*(p + 1) == '{') {
			p += 2;
			templ = p;
			while (*p && !isspace(*p) && *p != '{' && *p != '}')
				p++;
			if (*p == '}')
				args = NULL;
			else
				args = p;

			stack = 1;
			while (*p && stack > 0) {
				if (*p == '{')
					stack++;
				else if (*p == '}')
					stack--;
				p++;
			}
			if (stack == 0) {
				// stack is empty. that means the previous char was }, so we zero it
				*(p - 1) = '\0';
			} else {
				// we ran into the end of string without finding a closing }, bark
				CRIT_ERR(NULL, NULL, "cannot find a closing '}' in template expansion");
			}
		} else {
			templ = p + 1;
			p += strlen("$template");
			while (*p && isdigit(*p))
				p++;
			args = NULL;
		}
		tmpl_out = handle_template(templ, args);
		if (tmpl_out) {
			outlen += strlen(tmpl_out);
			*o = '\0';
			outbuf = (char*) realloc(outbuf, outlen * sizeof(char));
			strcat (outbuf, tmpl_out);
			free(tmpl_out);
			o = outbuf + strlen(outbuf);
		} else {
			NORM_ERR("failed to handle template '%s' with args '%s'", templ, args);
		}
	}
	*o = '\0';
	outbuf = (char*) realloc(outbuf, (strlen(outbuf) + 1) * sizeof(char));
	free(indup);
	return outbuf;
}

/* check text for any template object references */
int text_contains_templates(const char *text)
{
	if (strcasestr(text, "${template") != NULL)
		return 1;
	if (strcasestr(text, "$template") != NULL)
		return 1;
	return 0;
}

