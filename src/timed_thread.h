/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * timed_thread.h: Abstraction layer for timed threads
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
 * USA. */

#ifndef _TIMED_THREAD_H_
#define _TIMED_THREAD_H_

#include <stdlib.h>

/* 10000 microseconds = 10 ms =  0.01 sec */
#define MINIMUM_INTERVAL_USECS 10000

#ifdef __cplusplus
extern "C" {
#endif

/* opaque structure for clients */
typedef struct _timed_thread timed_thread;

/* create a timed thread (object creation only) */
timed_thread *timed_thread_create(void *start_routine(void *), void *arg,
	unsigned int interval_usecs);

/* run a timed thread (drop the thread and run it) */
int timed_thread_run(timed_thread *p_timed_thread);

/* destroy a timed thread */
void timed_thread_destroy(timed_thread *p_timed_thread,
	timed_thread **addr_of_p_timed_thread);

/* lock a timed thread for critical section activity */
int timed_thread_lock(timed_thread *p_timed_thread);

/* unlock a timed thread after critical section activity */
int timed_thread_unlock(timed_thread *p_timed_thread);

/* waits required interval (unless override_wait_time is non-zero) for
 * termination signal returns 1 if received, 0 otherwise.  should also return 1
 * if the thread has been asked kindly to die.  */
int timed_thread_test(timed_thread *p_timed_thread, int override_wait_time);

/* exit a timed thread */
void timed_thread_exit(timed_thread *p_timed_thread) __attribute__((noreturn));

/* register a timed thread for future destruction via
 * timed_thread_destroy_registered_threads() */
int timed_thread_register(timed_thread *p_timed_thread,
	timed_thread **addr_of_p_timed_thread);

/* destroy all registered timed threads */
void timed_thread_destroy_registered_threads(void);

/* returns read file descriptor for thread pipe */
int timed_thread_readfd(timed_thread *p_timed_thread);

#ifdef __cplusplus
}
#endif
#endif /* #ifdef _TIMED_THREAD_H_ */
