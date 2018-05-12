/*
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

#include "config.h"
#include "logging.h"

#include "update-cb.hh"

#include <typeinfo>
#include <unistd.h>

namespace conky {
namespace {
semaphore sem_wait;
enum { UNUSED_MAX = 5 };
}  // namespace

namespace priv {
callback_base::~callback_base() { stop(); }

void callback_base::stop() {
  if (thread != nullptr) {
    done = true;
    sem_start.post();
    if (pipefd.second >= 0) {
      if (write(pipefd.second, "X", 1) != 1) {
        NORM_ERR("can't write 'X' to pipefd %d: %s", pipefd.second,
                 strerror(errno));
      }
    }
    thread->join();
    delete thread;
    thread = nullptr;
  }
  if (pipefd.first >= 0) {
    close(pipefd.first);
    pipefd.first = -1;
  }
  if (pipefd.second >= 0) {
    close(pipefd.second);
    pipefd.second = -1;
  }
}

inline size_t callback_base::get_hash(const handle &h) { return h->hash; }

inline bool callback_base::is_equal(const handle &a, const handle &b) {
  if (a->hash != b->hash) {
    return false;
  }

  if (typeid(*a) != typeid(*b)) {
    return false;
  }

  return *a == *b;
}

/*
 * If a callback is not successfully inserted into the set, it must have
 * the same hash as an existing callback. If this is so, merge the incoming
 * callback with the one that prevented insertion. Keep the smaller of the
 * two periods.
 */
void callback_base::merge(callback_base &&other) {
  if (other.period < period) {
    period = other.period;
    remaining = 0;
  }
  assert(wait == other.wait);
  unused = 0;
}

/*
 * Register a callback (i.e. insert it into the callbacks set)
 */
callback_base::handle callback_base::do_register_cb(const handle &h) {
  const auto &p = callbacks.insert(h);

  /* insertion failed; callback already exists */
  if (not p.second) {
    (*p.first)->merge(std::move(*h));
  }

  return *p.first;
}

void callback_base::run() {
  if (thread == nullptr) {
    thread = new std::thread(&callback_base::start_routine, this);
  }

  sem_start.post();
}

void callback_base::start_routine() {
  for (;;) {
    sem_start.wait();
    if (done) {
      return;
    }

    // clear any remaining posts in case the previous iteration was very slow
    // (this should only happen if wait == false)
    while (sem_start.trywait()) {
      ;
    }

    work();
    if (wait) {
      sem_wait.post();
    }
  }
}

callback_base::Callbacks callback_base::callbacks(1, get_hash, is_equal);
}  // namespace priv

void run_all_callbacks() {
  using priv::callback_base;

  size_t wait = 0;
  for (auto i = callback_base::callbacks.begin();
       i != callback_base::callbacks.end();) {
    callback_base &cb = **i;

    /* check whether enough update intervals have elapsed (up to period) */
    if (cb.remaining-- == 0) {
      /* run the callback as long as someone holds a pointer to it;
       * if no one owns the callback, run it at most UNUSED_MAX times */
      if (!i->unique() || ++cb.unused < UNUSED_MAX) {
        cb.remaining = cb.period - 1;
        cb.run();
        if (cb.wait) {
          ++wait;
        }
      }
    }
    if (cb.unused == UNUSED_MAX) {
      auto t = i;
      ++i;
      callback_base::callbacks.erase(t);
    } else {
      ++i;
    }
  }

  while (wait-- > 0) {
    sem_wait.wait();
  }
}
}  // namespace conky
