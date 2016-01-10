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
#ifndef _TEXT_OBJECT_H
#define _TEXT_OBJECT_H

#include <stdint.h>		/* uint8_t */
#include "config.h"		/* for the defines */
#include "specials.h"		/* enum special_types */
#include "update-cb.hh"
#include "exec.h"

/* text object callbacks */
struct obj_cb {
	/* text object: print obj's output to p */
	void (*print)(struct text_object *obj, char *p, int p_max_size);

	/* ifblock object: return zero to trigger jumping */
	int (*iftest)(struct text_object *obj);

	/* meter objects:
	 * The following functions return the current meter-type value
	 * in a range between 0 and the value passed to the appropriate
	 * scan_* function. Or, if named function has been called with
	 * a value of 0, make use of auto-scaling (i.e., scaling to the
	 * maximum value seen so far). */
	double (*barval)(struct text_object *obj);
	double (*gaugeval)(struct text_object *obj);
	double (*graphval)(struct text_object *obj);

	/* percentage object: return value in range [0, 100] */
	uint8_t (*percentage)(struct text_object *obj);

	/* free obj's data */
	void (*free)(struct text_object *obj);
};

/* generic free opaque callback
 * can be used to simply free obj->data.opaque or obj->data.s */
void gen_free_opaque(struct text_object *);

/* generic iftest returning false (i.e. trigger jumping)
 * used for the else object */
int gen_false_iftest(struct text_object *);

/* generic nothing printer callback printing nothing
 * used for the endif object */
void gen_print_nothing(struct text_object *, char *, int);

/* generic obj->data.s printer
 * used by the $text object */
void gen_print_obj_data_s(struct text_object *, char *, int);

class legacy_cb: public conky::callback<void *, int (*)()> {
    typedef conky::callback<void *, int (*)()> Base;

protected:
    virtual void work()
    { std::get<0>(tuple)(); }

public:
    legacy_cb(uint32_t period, int (*fn)())
        : Base(period, true, Base::Tuple(fn))
    {}
};

typedef conky::callback_handle<legacy_cb> legacy_cb_handle;
typedef conky::callback_handle<exec_cb> exec_cb_handle;

/**
 * This is where Conky collects information on the conky.text objects in your config
 *
 * During startup and reload, objects are parsed and callbacks are set. Note that
 * there are currently two types of callback: obj_cb (old style) and
 * conky::callback (new style). On each update interval, generate_text_internal()
 * in conky.cc traverses the list of text_objects and calls the old callbacks.
 * The new style callbacks are run separately by conky::run_all_callbacks().
 */
struct text_object {
	struct text_object *next, *prev;	/* doubly linked list of text objects */
	struct text_object *sub;		/* for objects parsing text into objects */
	struct text_object *ifblock_next;	/* jump target for ifblock objects */

	union {
		void *opaque;		/* new style generic per object data */
		char *s;		/* some string */
		int i;			/* some integer */
		long l;			/* some long integer */
	} data;

	void *special_data;
	long line;
	bool parse;	/* if true then data.s should still be parsed */
	bool thread;	/* if true then data.s should be set by a seperate thread */

	struct obj_cb callbacks;

	/* Each _cb_handle is a std::shared_ptr with very tight restrictions on
	 * construction. For now, it is necessary to store them here as regular
	 * pointers so we can instantiate them later. */
	exec_cb_handle *exec_handle;
	legacy_cb_handle *cb_handle;
};

/* text object list helpers */
int append_object(struct text_object *root, struct text_object *obj);

/* ifblock helpers
 *
 * Opaque is a pointer to the address of the ifblock stack's top object.
 * Calling clients should pass the address of a defined void pointer which
 * was initialised to NULL (empty stack).
 * */
int obj_be_ifblock_if(void **opaque, struct text_object *);
int obj_be_ifblock_else(void **opaque, struct text_object *);
int obj_be_ifblock_endif(void **opaque, struct text_object *);
int ifblock_stack_empty(void **opaque);

/* make the given object be a plain text object printing given string */
void obj_be_plain_text(struct text_object *, const char *);

#endif /* _TEXT_OBJECT_H */
