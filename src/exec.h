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
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include "text_object.h"

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
 * For example, to parse an ${execgraph} object, we pass EF_EXEC | EF_GRAPH
 * as the last argument to scan_exec_arg().
 */
enum {
  EF_EXEC = (1 << 0),
  EF_EXECI = (1 << 1),
  EF_BAR = (1 << 2),
  EF_GRAPH = (1 << 3),
  EF_GAUGE = (1 << 4)
};

void scan_exec_arg(struct text_object *, const char *, unsigned int);
void register_exec(struct text_object *);
void register_execi(struct text_object *);
void print_exec(struct text_object *, char *, int);
double execbarval(struct text_object *);
void free_exec(struct text_object *);
void free_execi(struct text_object *);

#endif /* _EXEC_H */
