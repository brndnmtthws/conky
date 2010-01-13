/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * timed_thread.c: Abstraction layer for timed threads
 *
 * Copyright (C) 2006-2007 Philip Kovacs pkovacs@users.sourceforge.net
 * Copyright (c) 2005-2010 Brenden Matthews, et. al. (see AUTHORS)
 * All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <thread>
#include <list>
#include <chrono>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "timed-thread.h"
#include "logging.h"

/* Abstraction layer for timed threads */

typedef struct std::chrono::system_clock clk;

/* private */
struct _timed_thread {
	std::auto_ptr<std::thread> thread;		/* thread itself */
	std::mutex cs_mutex;					/* critical section mutex */
	std::mutex runnable_mutex;				/* runnable section mutex */
	std::condition_variable runnable_cond;	/* signalled to stop the thread */
	clk::time_point last_time;				/* last time interval */
	int pipefd[2];
	int die;
	_timed_thread() : die(0) {}
};

typedef std::list<timed_thread_ptr> thread_list_t;
thread_list_t thread_list;

/* create a timed thread (object creation only) */
timed_thread::timed_thread(const std::function<void(thread_handle &)> start_routine, unsigned
		int interval_usecs) :
	p_timed_thread(new _timed_thread), p_thread_handle(this),
	interval_usecs(interval_usecs), running(false)
{

#ifdef DEBUG
	assert(interval_usecs >= MINIMUM_INTERVAL_USECS);
#endif /* DEBUG */

	/* create thread pipe (used to tell threads to die) */
	if (pipe(p_timed_thread->pipefd)) {
		throw std::runtime_error("couldn't create pipe");
	}

	/* set initialize to current time */
	p_timed_thread->last_time = clk::now();

	/* printf("interval_time.tv_sec = %li, .tv_nsec = %li\n",
	   p_timed_thread->interval_time.tv_sec,
	   p_timed_thread->interval_time.tv_nsec); */

	p_timed_thread->thread = std::auto_ptr<std::thread>(
			new std::thread(start_routine, p_thread_handle)
			);
	
	DBGP("created thread %ld", (long)p_timed_thread->thread.get());

	running = true;
}

/* destroy a timed thread. */
void timed_thread::destroy(bool deregister_this)
{
	DBGP("destroying thread %ld", (long)p_timed_thread->thread.get());
#ifdef DEBUG
	assert(running && p_timed_thread->thread->joinable());
#endif /* DEBUG */
	{
		/* signal thread to stop */
		std::lock_guard<std::mutex> l(p_timed_thread->runnable_mutex);
		p_timed_thread->runnable_cond.notify_one();
		p_timed_thread->die = 1;
	}

	if (write(p_timed_thread->pipefd[1], "die", 3) == -1)
		perror("write()");

	/* join the terminating thread */
	p_timed_thread->thread->join();

	close(p_timed_thread->pipefd[0]);
	close(p_timed_thread->pipefd[1]);
	
	running = false;

	if (deregister_this) deregister(this);

}

/* lock a timed thread for critical section activity */
void timed_thread::lock(void)
{
#ifdef DEBUG
	assert(running);
#endif /* DEBUG */
	p_timed_thread->cs_mutex.lock();
}

/* unlock a timed thread after critical section activity */
void timed_thread::unlock(void)
{
#ifdef DEBUG
	assert(running);
#endif /* DEBUG */
	p_timed_thread->cs_mutex.unlock();
}

std::mutex &timed_thread::mutex()
{
#ifdef DEBUG
	assert(running);
#endif /* DEBUG */
	return p_timed_thread->cs_mutex;
}

int timed_thread::readfd(void) const
{
#ifdef DEBUG
	assert(running);
#endif /* DEBUG */
	return p_timed_thread->pipefd[0];
}

/* thread waits interval_usecs for runnable_cond to be signaled.
 * returns 1 if signaled, -1 on error, and 0 otherwise.
 * caller should call timed_thread::exit() on any non-zero return value. */
int timed_thread::test(int override_wait_time)
{
#ifdef DEBUG
	assert(running);
#endif /* DEBUG */
	bool rc = false;
	/* determine when to wait until */
#ifdef _GLIBCXX_USE_CLOCK_REALTIME
	clk::time_point wait_time = p_timed_thread->last_time +
		clk::duration(interval_usecs * 1000);
#elif defined(_GLIBCXX_USE_GETTIMEOFDAY)
	clk::time_point wait_time = p_timed_thread->last_time +
		clk::duration(interval_usecs);
#else
	clk::time_point wait_time = p_timed_thread->last_time +
		clk::duration(interval_usecs / 1000000);
#endif

	/* acquire runnable_cond mutex */
	{
		std::unique_lock<std::mutex> lock(p_timed_thread->runnable_mutex);

		if (p_timed_thread->die) {
			/* if we were kindly asked to die, then die */
			return 1;
		}

		if (override_wait_time) {
			wait_time = clk::now();
		}

		/* release mutex and wait until future time for runnable_cond to signal */
		rc = p_timed_thread->runnable_cond.wait_until(lock, wait_time);
	}

	p_timed_thread->last_time = clk::now();
#ifdef _GLIBCXX_USE_CLOCK_REALTIME
	if (wait_time + clk::duration(interval_usecs * 1000) >
			p_timed_thread->last_time) {
		p_timed_thread->last_time = wait_time;
	}
#elif defined(_GLIBCXX_USE_GETTIMEOFDAY)
	if (wait_time + clk::duration(interval_usecs) >
			p_timed_thread->last_time) {
		p_timed_thread->last_time = wait_time;
	}
#else
	if (wait_time + clk::duration(interval_usecs / 1000000) >
			p_timed_thread->last_time) {
		p_timed_thread->last_time = wait_time;
	}
#endif
	
	/* if runnable_cond was signaled, tell caller to exit thread */
	return rc;
}

/* register a timed thread for future destruction via
 * timed_thread::destroy_registered_threads() */
int timed_thread::register_(const timed_thread_ptr &timed_thread)
{
	thread_list.push_back(timed_thread);
	return 0;
}


void timed_thread::deregister(const timed_thread *timed_thread)
{
	for (thread_list_t::iterator i = thread_list.begin(); i != thread_list.end(); i++) {
		if (i->get() == timed_thread) {
			thread_list.erase(i);
			break;
		}
	}
}

/* destroy all registered timed threads */
void timed_thread::destroy_registered_threads(void)
{
	for (thread_list_t::iterator i = thread_list.begin(); i != thread_list.end(); i++) {
//		(*i)->destroy(false /* don't deregister */);
#ifdef DEBUG
		/* if this assert is ever reached, we have an unreleased shared_ptr
		 * somewhere holding on to this instance */
		assert(i->unique()); 
#endif /* DEBUG */
	}
	thread_list.clear(); /* that was easy */
}

int thread_handle::test(int override_wait_time) {
	return thread->test(override_wait_time);
}

std::mutex &thread_handle::mutex()
{
#ifdef DEBUG
	assert(thread->running);
#endif /* DEBUG */
	return thread->p_timed_thread->cs_mutex;
}

void thread_handle::lock(void) {
	thread->lock();
}

void thread_handle::unlock(void) {
	thread->unlock();
}

int thread_handle::readfd(void) const
{
#ifdef DEBUG
	assert(thread->running);
#endif /* DEBUG */
	return thread->p_timed_thread->pipefd[0];
}

