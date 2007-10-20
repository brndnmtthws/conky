/* $Id$ */

/* 
 * timed_thread.c: Abstraction layer for timed threads
 *
 * Copyright (C) 2006-2007 Philip Kovacs pkovacs@users.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#ifndef HAVE_CLOCK_GETTIME
#include <sys/time.h>
#endif
#include "timed_thread.h"

/* Abstraction layer for timed threads */

static int now (struct timespec *);

/* private */
struct _timed_thread 
{
  pthread_t thread;               /* thread itself */
  pthread_attr_t thread_attr;     /* thread attributes */
  pthread_mutex_t cs_mutex;       /* critical section mutex */
  pthread_mutex_t runnable_mutex; /* only for the runnable_cond */
  pthread_cond_t runnable_cond;   /* signalled to stop the thread */
  void *(*start_routine)(void*);  /* thread function to run */
  void *arg;                      /* thread function argument */
  struct timespec interval_time;  /* interval_usecs as a struct timespec */
};

/* linked list of created threads */
typedef struct _timed_thread_list
{
  timed_thread *p_timed_thread;
  timed_thread **addr_of_p_timed_thread;
  struct _timed_thread_list *next;
} timed_thread_node, timed_thread_list;

static timed_thread_list *p_timed_thread_list_head = NULL;
static timed_thread_list *p_timed_thread_list_tail = NULL;

static int now (struct timespec *abstime)
{
  if (!abstime)
    return (-1);

#ifdef HAVE_CLOCK_GETTIME
  return clock_gettime (CLOCK_REALTIME, &abstime);
#else
  /* fallback to gettimeofday () */
  struct timeval tv;
  if (gettimeofday (&tv, NULL) != 0)
    return (-1);

  abstime->tv_sec = tv.tv_sec;
  abstime->tv_nsec = tv.tv_usec * 1000;
  return 0;
#endif
}


/* create a timed thread (object creation only) */
timed_thread* 
timed_thread_create (void *(*start_routine)(void*), void *arg, unsigned int interval_usecs)
{
  timed_thread *p_timed_thread;

  assert (start_routine != NULL);
  assert (interval_usecs >= MINIMUM_INTERVAL_USECS);

  if ((p_timed_thread = calloc (sizeof(timed_thread), 1)) == 0)
    return NULL;

  /* init attributes, e.g. joinable thread */
  pthread_attr_init (&p_timed_thread->thread_attr);
  pthread_attr_setdetachstate (&p_timed_thread->thread_attr, PTHREAD_CREATE_JOINABLE);
  /* init mutexes */
  pthread_mutex_init (&p_timed_thread->cs_mutex, NULL);
  pthread_mutex_init (&p_timed_thread->runnable_mutex, NULL);
  /* init cond */
  pthread_cond_init (&p_timed_thread->runnable_cond, NULL);

  p_timed_thread->start_routine = start_routine;
  p_timed_thread->arg = arg;

  /* seconds portion of the microseconds interval */
  p_timed_thread->interval_time.tv_sec = (time_t)(interval_usecs / 1000000);
  /* remaining microseconds convert to nanoseconds */
  p_timed_thread->interval_time.tv_nsec = (long)((interval_usecs % 1000000) * 1000);

  /*
  printf ("interval_time.tv_sec = %li, .tv_nsec = %li\n",
            p_timed_thread->interval_time.tv_sec,
            p_timed_thread->interval_time.tv_nsec);
  */
  return p_timed_thread;
}

/* run a timed thread (drop the thread and run it) */
int 
timed_thread_run (timed_thread* p_timed_thread)
{
  return pthread_create (&p_timed_thread->thread, &p_timed_thread->thread_attr,
                         p_timed_thread->start_routine, p_timed_thread->arg);
}

/* destroy a timed thread.
 * optional addr_of_p_timed_thread to set callers pointer to NULL as a convenience. */
void 
timed_thread_destroy (timed_thread* p_timed_thread, timed_thread** addr_of_p_timed_thread)
{
  assert (p_timed_thread != NULL);
  assert ((addr_of_p_timed_thread == NULL) || (*addr_of_p_timed_thread == p_timed_thread));

  /* signal thread to stop */
  pthread_mutex_lock (&p_timed_thread->runnable_mutex);
  pthread_cond_signal (&p_timed_thread->runnable_cond);
  pthread_mutex_unlock (&p_timed_thread->runnable_mutex);

  /* join the terminating thread */
  if (p_timed_thread->thread)
    pthread_join (p_timed_thread->thread, NULL);

  /* clean up */
  pthread_attr_destroy (&p_timed_thread->thread_attr);
  pthread_mutex_destroy (&p_timed_thread->cs_mutex);
  pthread_mutex_destroy (&p_timed_thread->runnable_mutex);
  pthread_cond_destroy (&p_timed_thread->runnable_cond);

  free (p_timed_thread);
  if (addr_of_p_timed_thread)
    *addr_of_p_timed_thread = NULL;
}


/* lock a timed thread for critical section activity */
int
timed_thread_lock (timed_thread* p_timed_thread)
{
  assert (p_timed_thread != NULL);

  return pthread_mutex_lock (&p_timed_thread->cs_mutex);
}


/* unlock a timed thread after critical section activity */
int
timed_thread_unlock (timed_thread* p_timed_thread)
{
  assert (p_timed_thread != NULL);

  return pthread_mutex_unlock (&p_timed_thread->cs_mutex);
}


/* thread waits interval_usecs for runnable_cond to be signaled.  returns 1 if signaled, 
 * -1 on error, and 0 otherwise.  caller should call timed_thread_exit() on any non-zero 
 * return value. */
int 
timed_thread_test (timed_thread* p_timed_thread)
{
  struct timespec wait_time;
  int rc;

  assert (p_timed_thread != NULL);

  if (now (&wait_time)) return (-1);
  /*printf ("PRE:wait_time.tv_secs = %li, .tv_nsecs = %li\n", wait_time.tv_sec, wait_time.tv_nsec);*/
 
  /* add in the wait interval */
  if (1000000000-wait_time.tv_nsec <= p_timed_thread->interval_time.tv_nsec)
  {
    /* perform nsec->sec carry operation */
    wait_time.tv_sec += p_timed_thread->interval_time.tv_sec + 1;
    wait_time.tv_nsec -= 1000000000-p_timed_thread->interval_time.tv_nsec;
    /*printf ("001:wait_time.tv_secs = %li, .tv_nsecs = %li\n", wait_time.tv_sec, wait_time.tv_nsec);*/
  }
  else
  {
    /* no carry needed, just add respective components */
    wait_time.tv_sec += p_timed_thread->interval_time.tv_sec;
    wait_time.tv_nsec += p_timed_thread->interval_time.tv_nsec;
    /*printf ("002:wait_time.tv_secs = %li, .tv_nsecs = %li\n", wait_time.tv_sec, wait_time.tv_nsec);*/
  }

  /* acquire runnable_cond mutex */
  if (pthread_mutex_lock (&p_timed_thread->runnable_mutex))
    return (-1);  /* could not acquire runnable_cond mutex, so tell caller to exit thread */

  /* release mutex and wait until future time for runnable_cond to signal */
  rc = pthread_cond_timedwait (&p_timed_thread->runnable_cond,
                               &p_timed_thread->runnable_mutex,
                               &wait_time);
  /* mutex re-acquired, so release it */
  pthread_mutex_unlock (&p_timed_thread->runnable_mutex);

  if (rc==0)
    return 1; /* runnable_cond was signaled, so tell caller to exit thread */

  /* tell caller not to exit yet */
  return 0;
}


/* exit a timed thread */
void 
timed_thread_exit (timed_thread* p_timed_thread)
{
  assert (p_timed_thread != NULL);

  pthread_exit (NULL);
}


/* register a timed thread for future destruction via timed_thread_destroy_registered_threads() */
int 
timed_thread_register (timed_thread* p_timed_thread, timed_thread** addr_of_p_timed_thread)
{
  timed_thread_node *p_node;

  assert ((addr_of_p_timed_thread == NULL) || (*addr_of_p_timed_thread == p_timed_thread));

  if ((p_node = calloc (sizeof (timed_thread_node), 1)) == 0)
    return 0;
    
  p_node->p_timed_thread = p_timed_thread;
  p_node->addr_of_p_timed_thread = addr_of_p_timed_thread;
  p_node->next = NULL;

  if (!p_timed_thread_list_tail)
  {
    /* first node of empty list */
    p_timed_thread_list_tail = p_node;
    p_timed_thread_list_head = p_node;
  }
  else
  {
    /* add node to tail of non-empty list */
    p_timed_thread_list_tail->next = p_node;
    p_timed_thread_list_tail = p_node;
  }

  return 0;
}


/* destroy all registered timed threads */
void 
timed_thread_destroy_registered_threads (void)
{
  timed_thread_node *p_node, *p_next;

  for (p_node=p_timed_thread_list_head;
       p_node;
       p_node=p_next)
  {
    p_next = p_node->next;
    timed_thread_destroy (p_node->p_timed_thread, p_node->addr_of_p_timed_thread);
    free (p_node);
    p_node = NULL;
  }

  p_timed_thread_list_head = NULL;
  p_timed_thread_list_tail = NULL;
}

