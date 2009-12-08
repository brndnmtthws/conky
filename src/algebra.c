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
#include "conky.h"
#define _GNU_SOURCE
#include "algebra.h"
#include "logging.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* find the operand in the given expression
 * returns the index of the first op character or -1 on error
 */
int find_match_op(const char *expr)
{
	unsigned int idx;

	for (idx = 0; idx < strlen(expr); idx++) {
		switch (expr[idx]) {
			case '=':
			case '!':
				if (expr[idx + 1] != '=')
					return -1;
				/* fall through */
			case '<':
			case '>':
				return idx;
				break;
		}
	}
	return -1;
}

int get_match_type(const char *expr)
{
	int idx;
	const char *str;

	if ((idx = find_match_op(expr)) == -1)
		return -1;
	str = expr + idx;

	if (*str == '=' && *(str + 1) == '=')
		return OP_EQ;
	else if (*str == '!' && *(str + 1) == '=')
		return OP_NEQ;
	else if (*str == '>') {
		if (*(str + 1) == '=')
			return OP_GEQ;
		return OP_GT;
	} else if (*str == '<') {
		if (*(str + 1) == '=')
			return OP_LEQ;
		return OP_LT;
	}
	return -1;
}



/* generic compare function
 *
 * v is actually the difference of the compared values. For strings
 * this is equal to the output of str(n)cmp(). Use a macro here, as
 * it's type-independent.
 */
#define COMPARE(v, t) \
	switch (t) { \
		case OP_GT:  return (v > 0); \
		case OP_LT:  return (v < 0); \
		case OP_EQ:  return (v == 0); \
		case OP_GEQ: return (v >= 0); \
		case OP_LEQ: return (v <= 0); \
		case OP_NEQ: return (v != 0); \
	} \
	return 0

int lcompare(long a, enum match_type mtype, long b)
{
	DBGP2("comparing longs '%ld' and '%ld'", a, b);
	COMPARE((a - b), mtype);
}
int dcompare(double a, enum match_type mtype, double b)
{
	DBGP2("comparing doubles '%.lf' and '%.lf'", a, b);
	COMPARE((a - b), mtype);
}

int scompare(const char *a, enum match_type mtype, const char *b)
{
	DBGP2("comparing strings '%s' and '%s'", a, b);
	COMPARE(strcmp(a, b), mtype);
}

enum arg_type get_arg_type(const char *arg)
{
	const char *p, *e;

	p = arg;
	e = arg + strlen(arg);

	if (*(e - 1) == ' ')
		e--;
	while (*e && *e == ' ')
		e--;
	while (p != e && *p == ' ')
		p++;

	if (*p == '"' && *e == '"')
		return ARG_STRING;

	if (*p == '-')	//allow negative values
		p++;
	while (p != e) {
		if (!isdigit(*p))
			break;
		p++;
	}
	if (p == e)
		return ARG_LONG;
	if (*p == '.') {
		p++;
		while (p != e) {
			if (!isdigit(*p))
				return ARG_STRING;
			p++;
		}
		return ARG_DOUBLE;
	}
	return ARG_STRING;
}

char *arg_to_string(const char *arg)
{
	const char *start;
	int len;

	start = arg;
	len = 0;
	while (*start && *start == ' ')
			start++;
	if (!(*(start++) == '"'))
		return NULL;
	while (start[len] != '"')
		len++;
	return strndup(start, len);
}
double arg_to_double(const char *arg)
{
	double d;
	if (sscanf(arg, "%lf", &d) != 1) {
		NORM_ERR("converting '%s' to double failed", arg);
		return 0.0;
	}
	return d;
}
long arg_to_long(const char *arg)
{
	long l;
	if (sscanf(arg, "%ld", &l) != 1) {
		NORM_ERR("converting '%s' to long failed", arg);
		return 0;
	}
	return l;
}
int compare(const char *expr)
{
	char *expr_dup;
	int idx, mtype;
	enum arg_type type1, type2;

	idx = find_match_op(expr);
	mtype = get_match_type(expr);

	if (!idx || mtype == -1) {
		NORM_ERR("failed to parse compare string '%s'", expr);
		return -2;
	}

	expr_dup = strdup(expr);
	expr_dup[idx] = '\0';
	if (expr_dup[idx + 1] == '=')
		expr_dup[++idx] = '\0';

	type1 = get_arg_type(expr_dup);
	type2 = get_arg_type(expr_dup + idx + 1);
	if (type1 == ARG_LONG && type2 == ARG_DOUBLE)
		type1 = ARG_DOUBLE;
	if (type1 == ARG_DOUBLE && type2 == ARG_LONG)
		type2 = ARG_DOUBLE;
	if (type1 != type2) {
		NORM_ERR("trying to compare args '%s' and '%s' of different type",
				expr_dup, (expr_dup + idx + 1));
		return -2;
	}
	switch (type1) {
		case ARG_STRING:
			{
				char *a, *b;
				a = arg_to_string(expr_dup);
				b = arg_to_string(expr_dup + idx + 1);
				idx = scompare(a, mtype, b);
				free(a);
				free(b);
				return idx;
			}
		case ARG_LONG:
			return lcompare(arg_to_long(expr_dup), mtype,
					arg_to_long(expr_dup + idx + 1));
		case ARG_DOUBLE:
			return dcompare(arg_to_double(expr_dup), mtype,
					arg_to_double(expr_dup + idx + 1));
	}
	/* not reached */
	return -2;
}

int check_if_match(struct text_object *obj)
{
	char expression[max_user_text];
	int val;
	int result = 1;

	generate_text_internal(expression, max_user_text, *obj->sub);
	DBGP("parsed arg into '%s'", expression);

	val = compare(expression);
	if (val == -2) {
		NORM_ERR("compare failed for expression '%s'", expression);
	} else if (!val) {
		result = 0;
	}
	return result;
}
