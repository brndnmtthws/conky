/*
 *
 * prioqueue:  a simple priority queue implementation
 *
 * Copyright (C) 2009 Phil Sutter <phil@nwl.cc>
 *
 * Initially based on the former implementation of sorted processes in
 * top.c, Copyright (C) 2005 David Carter <boojit@pundo.com>
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

#ifndef _PRIOQUEUE_H
#define _PRIOQUEUE_H

/* forward-define for private data */
struct prio_queue;

/* typedef for a distinct prioqueue object */
typedef struct prio_queue *prio_queue_t;

/* initialise a prioqueue object (mandatory) */
prio_queue_t init_prio_queue(void);

/* set the compare function (mandatory)
 * (*compare) shall return:
 * <0 if a should come before b,
 * >0 if b should come before a,
 *  0 if doesn't matter */
void pq_set_compare(prio_queue_t, int (*compare)(void *a, void *b));

/* set the data free function (optional)
 * (*free) will be called when:
 * - dropping elements from the end of a limited size queue and
 * - free_prio_queue() finds leftover elements in the given queue */
void pq_set_free(prio_queue_t, void (*free)(void *));

/* set a maximum queue size (optional) (defaults to INT_MAX) */
void pq_set_max_size(prio_queue_t, int);

/* insert an element into the given queue */
void insert_prio_elem(prio_queue_t, void *);

/* return the number of elements in the queue */
int pq_get_cur_size(prio_queue_t queue);

/* pop the top-most element from the queue
 * returns NULL if queue is empty */
void *pop_prio_elem(prio_queue_t);

/* clear and free the given queue */
void free_prio_queue(prio_queue_t);

#endif /* _PRIOQUEUE_H */
