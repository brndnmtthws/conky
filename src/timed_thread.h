/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * timed_thread.h: Abstraction layer for timed threads
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
 * USA. */

#ifndef _TIMED_THREAD_H_
#define _TIMED_THREAD_H_

#include <stdlib.h>
#include <functional>
#include <memory>

/* 10000 microseconds = 10 ms =  0.01 sec */
const unsigned int MINIMUM_INTERVAL_USECS = 10000;

/* private data */
typedef struct _timed_thread _timed_thread;

class timed_thread;
typedef std::shared_ptr<timed_thread> timed_thread_ptr;

namespace std { class mutex; }

class thread_handle {
	/* this class is passed to threaded functions allowing them to control
	 * specific thread aspects */
	public:
		virtual ~thread_handle() {}
		int test(int override_wait_time);
		std::mutex &mutex();
		void lock(void);
		void unlock(void);
		int readfd(void) const;
	private:
		thread_handle(timed_thread *thread) : thread(thread) {}
		friend class timed_thread;
		timed_thread *thread;
};

class timed_thread {
	public:
		/* create a timed thread (object creation only) */
		static timed_thread_ptr create(const std::function<void(thread_handle &)> &start_routine, const unsigned int
				interval_usecs, bool register_for_destruction = true) {
			timed_thread_ptr ptr(new timed_thread(std::cref(start_routine), interval_usecs));
			if (register_for_destruction) {
				register_(ptr);
			}
			return ptr;
		}

		virtual ~timed_thread(void) {
			destroy();
		}

		/* run a timed thread (drop the thread and run it) */
		int run(void);

		/* lock a timed thread for critical section activity */
		void lock(void);

		/* unlock a timed thread after critical section activity */
		void unlock(void);

		/* returns the critical section mutex */
		std::mutex &mutex();

		/* destroy all registered timed threads */
		static void destroy_registered_threads(void);

		/* returns read file descriptor for thread pipe */
		int readfd(void) const;

	private:
		/* create a timed thread (object creation only) */
		timed_thread(const std::function<void(thread_handle &)> &start_routine, unsigned int
				interval_usecs);

		/* waits required interval (unless override_wait_time is non-zero) for
		 * termination signal returns 1 if received, 0 otherwise.  should also return 1
		 * if the thread has been asked kindly to die.  */
		int test(int override_wait_time);

		/* destroy a timed thread */
		void destroy(void);

		/* register a timed thread for destruction */
		static int register_(const timed_thread_ptr &timed_thread);
		
		/* de-register a timed thread for destruction */
		static void deregister(const timed_thread *timed_thread);

		/* private internal data */
		std::auto_ptr<_timed_thread> p_timed_thread;
		thread_handle p_thread_handle;
		unsigned int interval_usecs;
		bool running;
		friend class thread_handle;
};

#endif /* #ifdef _TIMED_THREAD_H_ */
