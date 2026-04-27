/*
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
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *   (see AUTHORS)
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

#ifndef _EXEC_H
#define _EXEC_H

#include "../update-cb.hh"

/**
 * A callback that executes a command and stores the output as a std::string.
 *
 * Important note: if more than one exec callback uses the same command,
 * then only ONE callback is actually stored. This saves space. However,
 * suppose we have the following ${exec} objects in our conky.text:
 *
 * ${exec ~/bin/foo.sh}
 * ${execi 10 ~/bin/foo.sh}
 *
 * To the callback system, these are identical! Furthermore, the callback
 * with the smallest period/interval is the one that is stored. So the execi
 * command will in fact run on every update interval, rather than every
 * ten seconds as one would expect.
 */
class exec_cb : public conky::callback<std::string, std::string> {
  typedef conky::callback<std::string, std::string> Base;

 protected:
  virtual void work();

 public:
  exec_cb(uint32_t period, bool wait, const std::string &cmd)
      : Base(period, wait, Base::Tuple(cmd)) {}
};

/**
 * Flags used to identify the different types of exec commands during
 * parsing by scan_exec_arg(). These can be used individually or combined.
 * For example, to parse an ${execigraph} object, we pass
 * exec_flag::interval | exec_flag::graph as the last argument to
 * scan_exec_arg().
 */
enum class exec_flag : unsigned int {
  none = 0,
  interval = (1 << 0),
  bar = (1 << 1),
  graph = (1 << 2),
  gauge = (1 << 3)
};

inline exec_flag operator|(exec_flag a, exec_flag b) {
  return static_cast<exec_flag>(static_cast<unsigned>(a) |
                                    static_cast<unsigned>(b));
}

inline bool operator&(exec_flag a, exec_flag b) {
  return (static_cast<unsigned>(a) & static_cast<unsigned>(b)) != 0;
}

void scan_exec_arg(struct text_object *, const char *,
                   exec_flag = exec_flag::none);
void register_exec(struct text_object *);
void print_exec(struct text_object *, char *, unsigned int);
double execbarval(struct text_object *);
void free_exec(struct text_object *);

#endif /* _EXEC_H */
