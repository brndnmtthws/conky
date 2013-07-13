/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
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

#ifndef UPDATE_CB_HH
#define UPDATE_CB_HH

#include <cstdint>
#include <memory>
#include <thread>
// the following probably requires a is-gcc-4.7.0 check
#include <mutex>
#include <tuple>
#include <unordered_set>


#include <assert.h>

#include "c++wrap.hh"
#include "semaphore.hh"

namespace conky {
	// forward declarations
	template<typename Callback>
	class callback_handle;
	void run_all_callbacks();
	template<typename Callback, typename... Params>
	callback_handle<Callback> register_cb(uint32_t period, Params&&... params);

	namespace priv {
		class callback_base {
			typedef callback_handle<callback_base> handle;
			typedef std::unordered_set<handle, size_t (*)(const handle &),
									   bool (*)(const handle &, const handle &)>
			Callbacks;


			semaphore sem_start;
			std::thread *thread;
			const size_t hash;
			uint32_t period;
			uint32_t remaining;
			std::pair<int, int> pipefd;
			const bool wait;
			bool done;
			uint8_t unused;

			callback_base(const callback_base &) = delete;
			callback_base& operator=(const callback_base &) = delete;

			virtual bool operator==(const callback_base &) = 0;

			void run();
			void start_routine();
			void stop();

			static void deleter(callback_base *ptr)
			{
				ptr->stop();
				delete ptr;
			}

			// a list of registered callbacks
			static Callbacks callbacks;

			// used by the callbacks list
			static inline size_t get_hash(const handle &h);
			static inline bool is_equal(const handle &a, const handle &b);

			static handle do_register_cb(const handle &h);

			template<typename Callback, typename... Params>
			friend callback_handle<Callback>
			conky::register_cb(uint32_t period, Params&&... params);

			friend void conky::run_all_callbacks();

			template<typename Callback>
			friend class conky::callback_handle;

		protected:
			callback_base(size_t hash_, uint32_t period_, bool wait_, bool use_pipe)
				: thread(NULL), hash(hash_), period(period_), remaining(0),
				  pipefd(use_pipe ? pipe2(O_CLOEXEC) : std::pair<int, int>(-1, -1)),
				  wait(wait_), done(false), unused(0)
			{}

			int donefd()
			{ return pipefd.first; }

			bool is_done()
			{ return done; }

			// to be implemented by descendant classes
			virtual void work() = 0;

			// called when two registered objects evaulate as equal, the latter is removed
			// afterwards
			virtual void merge(callback_base &&);


		public:
			std::mutex result_mutex;

			virtual ~callback_base();
		};

	}

	template<typename Callback>
	class callback_handle: private std::shared_ptr<Callback> {
		typedef std::shared_ptr<Callback> Base;
	
		callback_handle(Callback *ptr)
			: Base(ptr, &priv::callback_base::deleter)
		{}

		callback_handle(Base &&ptr)
			: Base(std::move(ptr))
		{}

	public:

		using Base::operator->;
		using Base::operator*;

		friend void conky::run_all_callbacks();
		template<typename Callback_, typename... Params>
		friend callback_handle<Callback_> register_cb(uint32_t period, Params&&... params);
	};
		

	template<typename Callback, typename... Params>
	callback_handle<Callback> register_cb(uint32_t period, Params&&... params)
	{
		return std::dynamic_pointer_cast<Callback>(priv::callback_base::do_register_cb(
					priv::callback_base::handle(
						new Callback(period, std::forward<Params>(params)...)
					)
				));
	}

	namespace priv {
		template<size_t pos, typename... Elements>
		struct hash_tuple {
			typedef std::tuple<Elements...>                         Tuple;
			typedef typename std::tuple_element<pos-1, Tuple>::type Element;

			static inline size_t hash(const Tuple &tuple)
			{
				return std::hash<Element>()(std::get<pos-1>(tuple))
						+ 47 * hash_tuple<pos-1, Elements...>::hash(tuple);
			}
		};

		template<typename... Elements>
		struct hash_tuple<0, Elements...> {
			static inline size_t hash(const std::tuple<Elements...> &)
			{ return 0; }
		};
	}

	/*
	 * To create a callback, inherit from this class. The Result template parameter should be the
	 * type of your output, so that your users can retrieve it with the get_result* functions.
	 *
	 * get_result() returns a reference to the internal variable. It can be used without locking
	 * if the object has wait set to true (wait=true means that the run_all_callbacks() waits for
	 * the callback to finish work()ing before returning). If object has wait=false then the user
	 * must first lock the result_mutex.
	 *
	 * get_result_copy() returns a copy of the result object and it handles the necessary
	 * locking. Don't call it if you hold a lock on the result_mutex.
	 *
	 * You should implement the work() function to do the actual updating and store the result in
	 * the result variable (lock the mutex while you are doing it, especially if you have
	 * wait=false).
	 *
	 * The Keys... template parameters are parameters for your work function. E.g., a curl
	 * callback can have one parameter - the url to retrieve,. hddtemp may have two - host and
	 * port number of the hddtemp server, etc. The register_cb() function make sure that there
	 * exists only one object (of the same type) with the same values for all the keys.
	 *
	 * Callbacks are registered with the register_cb() function. You pass the class name as the
	 * template parameter, and any additional parameters to the constructor as function
	 * parameters. The period parameter specifies how often the callback will run. It should be
	 * left for the user to decide that. register_cb() returns a pointer to the newly created
	 * object. As long as someone holds a pointer to the object, the callback will be run.
	 *
	 * run_all_callbacks() runs the registered callbacks (with the specified periodicity). It
	 * should be called from somewhere inside the main loop, according to the update_interval
	 * setting. It waits for the callbacks which have wait=true. It leaves the rest to run in
	 * background.
	 */
	template<typename Result, typename... Keys>
	class callback: public priv::callback_base {
		virtual bool operator==(const callback_base &other)
		{ return tuple == dynamic_cast<const callback &>(other).tuple; }

	public:
		typedef std::tuple<Keys...> Tuple;

	protected:
		const Tuple tuple;
		Result result;

		template<size_t i>
		typename std::add_lvalue_reference<
					const typename std::tuple_element<i, Tuple>::type
				>::type
		get()
		{ return std::get<i>(tuple); }

	public:
		callback(uint32_t period_, bool wait_, const Tuple &tuple_, bool use_pipe = false)
			: callback_base(priv::hash_tuple<sizeof...(Keys), Keys...>::hash(tuple_),
						period_, wait_, use_pipe),
			  tuple(tuple_)
		{}

		const Result& get_result()
		{ return result; }

		Result get_result_copy()
		{
			std::lock_guard<std::mutex> l(result_mutex);
			return result;
		}
	};
}

#endif /* UPDATE_CB_HH */
