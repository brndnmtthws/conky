/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#include <libical/ical.h>
#include "conky.h"
#include "logging.h"

struct ical_event {
	icaltimetype start;
	icalcomponent *event;
	ical_event *next, *prev;
};

struct obj_ical {
	struct ical_event *list;
	icalcomponent *comps;
	icalparser *parser;
	unsigned int num;
};

char* read_stream(char *s, size_t size, void *d) {
	return fgets(s, size, (FILE*) d);
}

struct ical_event *add_event(struct ical_event *listend, icalcomponent *new_ev) {
	struct ical_event *ev_new, *ev_cur;
	icaltimetype start;

	start = icalcomponent_get_dtstart(new_ev);
	if(icaltime_compare(start, icaltime_from_timet(time(NULL), 0)) <= 0) {
		icalproperty *rrule = icalcomponent_get_first_property(new_ev, ICAL_RRULE_PROPERTY);
		if(rrule) {
			icalrecur_iterator* ritr = icalrecur_iterator_new(icalproperty_get_rrule(rrule), start);
			icaltimetype nexttime = icalrecur_iterator_next(ritr);
			while (!icaltime_is_null_time(nexttime)) {
				if(icaltime_compare(nexttime, icaltime_from_timet(time(NULL), 0)) > 0) {
					start = nexttime;
					break;
				}
				nexttime = icalrecur_iterator_next(ritr);
			}
			icalrecur_iterator_free(ritr);
		} else return NULL;
	}
	ev_new = (struct ical_event *) malloc(sizeof(struct ical_event));
	memset(ev_new, 0, sizeof(struct ical_event));
	ev_new->event = new_ev;
	ev_new->start = start;
	if(listend) {	//list already contains events
		ev_cur = listend;
		while(icaltime_compare(ev_new->start, ev_cur->start) <= 0) {
			if( ! ev_cur->prev) {	//ev_new starts first
				ev_new->next = ev_cur;
				ev_cur->prev = ev_new;
				return listend;
			}
			ev_cur = ev_cur->prev;
		}
		if(ev_cur == listend) {	//ev_new starts last
			ev_cur->next = ev_new;
			ev_new->prev = ev_cur;
			return ev_new;
		}
		//ev_new somewhere in the middle
		ev_new->prev = ev_cur;
		ev_new->next = ev_cur->next;
		ev_cur->next->prev = ev_new;
		ev_cur->next = ev_new;
		return listend;
	}
	return ev_new;
}

void parse_ical_args(struct text_object *obj, const char* arg, void *free_at_crash, void *free_at_crash2) {
	char *filename = strdup(arg);
	FILE *file;
	icalparser *parser;
	icalcomponent *allc, *curc;
	struct ical_event *ll_start, *ll_end, *ll_new;
	struct obj_ical *opaque;
	unsigned int num;

	if(sscanf(arg , "%d %s", &num, filename) != 2) {
		free(filename);
		free(obj);
		CRIT_ERR(free_at_crash, free_at_crash2, "wrong number of arguments for $ical");
	}
	file = fopen(filename, "r");
	if( ! file) {
		free(obj);
		free(free_at_crash);
		CRIT_ERR(filename, free_at_crash2, "Can't read file %s", filename);
		return;
	}
	free(filename);
	parser = icalparser_new();
	icalparser_set_gen_data(parser, file);
	allc = icalparser_parse(parser, read_stream);
	fclose(file);
	curc = icalcomponent_get_first_component(allc, ICAL_VEVENT_COMPONENT);
	if(!curc) {
		icalparser_free(parser);
		icalcomponent_free(allc);
		NORM_ERR("No ical events available");
		return;
	}
	ll_start = add_event(NULL, curc);
	ll_end = ll_start;
	while(1) {
		curc = icalcomponent_get_next_component(allc, ICAL_VEVENT_COMPONENT);
		if(!curc) break;
		ll_new = add_event(ll_end, curc);
		if( ! ll_start) {	//first component was not added
			ll_start = ll_new;
			ll_end = ll_new;
		}else if( ll_start->prev ) {
			ll_start = ll_start->prev;
		}else if( ll_end->next ) {
			ll_end = ll_end->next;
		}
	}
	opaque = (struct obj_ical *) malloc(sizeof(struct obj_ical));
	opaque->list = ll_start;
	opaque->parser = parser;
	opaque->comps = allc;
	opaque->num = num;
	obj->data.opaque = opaque;
}

void print_ical(struct text_object *obj, char *p, int p_max_size) {
	struct obj_ical *ical_obj = (struct obj_ical *) obj->data.opaque;
	struct ical_event *ll_current;

	if( ! ical_obj) return;
	ll_current = ical_obj->list;
	unsigned int i=1;
	while(1) {
		if( ! ll_current) return;
		if(i > ical_obj->num) return;
		if(i == ical_obj->num) break;
		if(i < ical_obj->num) {
			ll_current = ll_current->next;
			i++;
		}
	}
	snprintf(p, p_max_size, "%s", icalproperty_get_summary(icalcomponent_get_first_property(ll_current->event, ICAL_SUMMARY_PROPERTY)));
}

void free_ical(struct text_object *obj) {
	struct obj_ical *ical_free_me = (struct obj_ical *) obj->data.opaque;

	if( ! ical_free_me) return;
	icalcomponent_free(ical_free_me->comps);
	icalparser_free(ical_free_me->parser);
	while(ical_free_me->list) {
		if(ical_free_me->list->next) {
			ical_free_me->list = ical_free_me->list->next;
			free_and_zero(ical_free_me->list->prev);
		} else free_and_zero(ical_free_me->list);
	}
	free(obj->data.opaque);
}
