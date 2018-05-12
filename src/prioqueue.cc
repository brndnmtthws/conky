/*
 *
 * prioqueue:  a simple priority queue implementation
 *
 * The queue organises it's data internally using a doubly linked
 * list, into which elements are inserted at the right position. This
 * is definitely not the best algorithm for a priority queue, but it
 * fits best for the given purpose, i.e. the top process sorting.
 * This means we have a rather little amount of total elements (~200
 * on a normal system), which are to be inserted into a queue of only
 * the few top-most elements (10 at the current state). Additionally,
 * at each update interval, the queue is drained completely and
 * refilled from scratch.
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

#include <climits> /* INT_MAX */
#include <cstdlib>
#include <cstring>
#include "conky.h"

struct prio_elem {
  struct prio_elem *next, *prev;
  void *data;
};

struct prio_queue {
  /* Compare a and b. Return:
   * <0 if a should come before b,
   * >0 if b should come before a,
   *  0 if don't care */
  int (*compare)(void *a, void *b);

  /* Free element payload. Called when dropping elements. */
  void (*free)(void *a);

  /* Maximum size of queue. The first
   * elements in the list take precedence. */
  int max_size;

  /* The pointers to the actual list. */
  struct prio_elem *head, *tail;

  /* The current number of elements in the list. */
  int cur_size;
};

/* nop callback to save us from conditional calling */
static void pq_free_nop(void *a) { (void)a; }

struct prio_queue *init_prio_queue() {
  struct prio_queue *retval;

  retval = static_cast<struct prio_queue *>(malloc(sizeof(struct prio_queue)));
  memset(retval, 0, sizeof(struct prio_queue));

  /* use pq_free_nop by default */
  retval->free = &pq_free_nop;

  /* Default to maximum possible size as restricted
   * by the used data type. This also saves us from
   * checking if caller has set this field or not. */
  retval->max_size = INT_MAX;

  return retval;
}

void pq_set_compare(struct prio_queue *queue,
                    int (*pqcompare)(void *a, void *b)) {
  if (pqcompare != nullptr) {
    queue->compare = pqcompare;
  }
}

void pq_set_free(struct prio_queue *queue, void (*pqfree)(void *a)) {
  if (pqfree != nullptr) {
    queue->free = pqfree;
  }
}

void pq_set_max_size(struct prio_queue *queue, int max_size) {
  if (max_size >= 0) {
    queue->max_size = max_size;
  }
}

int pq_get_cur_size(struct prio_queue *queue) { return queue->cur_size; }

static struct prio_elem *init_prio_elem(void *data) {
  struct prio_elem *retval;

  retval = static_cast<struct prio_elem *>(malloc(sizeof(struct prio_elem)));
  memset(retval, 0, sizeof(struct prio_elem));

  retval->data = data;
  return retval;
}

void insert_prio_elem(struct prio_queue *queue, void *data) {
  struct prio_elem *cur;

  /* queue->compare is a must-have */
  if (queue->compare == nullptr) {
    return;
  }

  /* empty queue, insert the first item */
  if (queue->cur_size == 0) {
    queue->cur_size++;
    queue->head = queue->tail = init_prio_elem(data);
    return;
  }

  /* short-cut 1: new item is lower than all others */
  if (queue->compare(queue->tail->data, data) <= 0) {
    if (queue->cur_size < queue->max_size) {
      queue->cur_size++;
      queue->tail->next = init_prio_elem(data);
      queue->tail->next->prev = queue->tail;
      queue->tail = queue->tail->next;
    } else { /* list was already full */
      (*queue->free)(data);
    }
    return;
  }

  /* short-cut 2: we have a new maximum */
  if (queue->compare(queue->head->data, data) >= 0) {
    queue->cur_size++;
    queue->head->prev = init_prio_elem(data);
    queue->head->prev->next = queue->head;
    queue->head = queue->head->prev;
    goto check_cur_size;
  }

  /* find the actual position if short-cuts failed */
  for (cur = queue->head->next; cur != nullptr; cur = cur->next) {
    if (queue->compare(cur->data, data) >= 0) {
      queue->cur_size++;
      cur->prev->next = init_prio_elem(data);
      cur->prev->next->prev = cur->prev;
      cur->prev->next->next = cur;
      cur->prev = cur->prev->next;
      break;
    }
  }

check_cur_size:
  /* drop the lowest item if queue overrun */
  if (queue->cur_size > queue->max_size) {
    queue->cur_size--;
    queue->tail = queue->tail->prev;
    (*queue->free)(queue->tail->next->data);
    free_and_zero(queue->tail->next);
  }
}

void *pop_prio_elem(struct prio_queue *queue) {
  struct prio_elem *tmp;
  void *data;

  if (queue->cur_size <= 0) {
    return nullptr;
  }

  tmp = queue->head;
  data = tmp->data;

  queue->head = queue->head->next;
  queue->cur_size--;
  if (queue->head != nullptr) {
    queue->head->prev = nullptr;
  } else { /* list is now empty */
    queue->tail = nullptr;
  }

  free(tmp);
  return data;
}

void free_prio_queue(struct prio_queue *queue) {
  void *data;
  while ((data = pop_prio_elem(queue)) != nullptr) {
    (*queue->free)(data);
  }
  free(queue);
}
