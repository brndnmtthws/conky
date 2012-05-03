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
#include "core.h"
#include "logging.h"
#include "text_object.h"

struct combine_data {
	char *left;
	char *seperation;
	char *right;
};

void parse_combine_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	struct combine_data *cd;
	unsigned int i,j;
	unsigned int indenting = 0;	//vars can be used as args for other vars
	int startvar[2];
	int endvar[2];
	startvar[0] = endvar[0] = startvar[1] = endvar[1] = -1;
	j=0;
	for (i=0; arg[i] != 0 && j < 2; i++) {
		if(startvar[j] == -1) {
			if(arg[i] == '$') {
				startvar[j] = i;
			}
		}else if(endvar[j] == -1) {
			if(arg[i] == '{') {
				indenting++;
			}else if(arg[i] == '}') {
				indenting--;
			}
			if (indenting == 0 && arg[i+1] < 48) {	//<48 has 0, $, and the most used chars not used in varnames but not { or }
				endvar[j]=i+1;
				j++;
			}
		}
	}
	if(startvar[0] >= 0 && endvar[0] >= 0 && startvar[1] >= 0 && endvar[1] >= 0) {
		cd = malloc(sizeof(struct combine_data));
		memset(cd, 0, sizeof(struct combine_data));

		cd->left = malloc(endvar[0]-startvar[0] + 1);
		cd->seperation = malloc(startvar[1] - endvar[0] + 1);
		cd->right= malloc(endvar[1]-startvar[1] + 1);

		strncpy(cd->left, arg + startvar[0], endvar[0] - startvar[0]);
		cd->left[endvar[0] - startvar[0]] = 0;

		strncpy(cd->seperation, arg + endvar[0], startvar[1] - endvar[0]);
		cd->seperation[startvar[1] - endvar[0]] = 0;

		strncpy(cd->right, arg + startvar[1], endvar[1] - startvar[1]);
		cd->right[endvar[1] - startvar[1]] = 0;

		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, cd->left);
		obj->sub->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub->sub, cd->right);
		obj->data.opaque = cd;
	} else {
		CRIT_ERR(obj, free_at_crash, "combine needs arguments: <text1> <text2>");
	}
}

void print_combine(struct text_object *obj, char *p, struct information *cur)
{
	struct combine_data *cd = obj->data.opaque;
	char buf[2][max_user_text];
	int i, j;
	long longest=0;
	int nextstart;
	int nr_rows[2];
	struct llrows {
		char* row;
		struct llrows* next;
	};
	struct llrows *ll_rows[2], *current[2];
	struct text_object * objsub = obj->sub;

	if (!cd)
		return;

	p[0]=0;
	for(i=0; i<2; i++) {
		nr_rows[i] = 1;
		nextstart = 0;
		ll_rows[i] = malloc(sizeof(struct llrows));
		current[i] = ll_rows[i];
		for(j=0; j<i; j++) objsub = objsub->sub;
		generate_text_internal(buf[i], max_user_text, *objsub, cur);
		for(j=0; buf[i][j] != 0; j++) {
			if(buf[i][j] == '\t') buf[i][j] = ' ';
			if(buf[i][j] == '\n') buf[i][j] = 0;	//the vars inside combine may not have a \n at the end
			if(buf[i][j] == 2) {	// \002 is used instead of \n to separate lines inside a var
				buf[i][j] = 0;
				current[i]->row = strdup(buf[i]+nextstart);
				if(i==0 && (long)strlen(current[i]->row) > longest) longest = (long)strlen(current[i]->row);
				current[i]->next = malloc(sizeof(struct llrows));
				current[i] = current[i]->next;
				nextstart = j + 1;
				nr_rows[i]++;
			}
		}
		current[i]->row = strdup(buf[i]+nextstart);
		if(i==0 && (long)strlen(current[i]->row) > longest) longest = (long)strlen(current[i]->row);
		current[i]->next = NULL;
		current[i] = ll_rows[i];
	}
	for(j=0; j < (nr_rows[0] > nr_rows[1] ? nr_rows[0] : nr_rows[1] ); j++) {
		if(current[0]) {
			strcat(p, current[0]->row);
			i=strlen(current[0]->row);
		}else i = 0;
		while(i < longest) {
			strcat(p, " ");
			i++;
		}
		if(current[1]) {
			strcat(p, cd->seperation);
			strcat(p, current[1]->row);
		}
		strcat(p, "\n");
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
		for(i=0; i<2; i++) if(current[i]) current[i]=current[i]->next;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
	for(i=0; i<2; i++) {
		while(ll_rows[i] != NULL) {
			current[i]=ll_rows[i];
			free(current[i]->row);
			ll_rows[i]=current[i]->next;
			free(current[i]);
		}
	}
}

void free_combine(struct text_object *obj)
{
	struct combine_data *cd = obj->data.opaque;

	if (!cd)
		return;
	free(cd->left);
	free(cd->seperation);
	free(cd->right);
	free_text_objects(obj->sub->sub, 1);
	free(obj->sub->sub);
	free_text_objects(obj->sub, 1);
	free(obj->sub);
	free(obj->data.opaque);
	obj->data.opaque = NULL;
}
