/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
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
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
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

#ifndef _LOGGING_H
#define _LOGGING_H

#include <cstdio>
#include <stdexcept>
#include "i18n.h"

class fork_throw : public std::runtime_error {
public:
	fork_throw() : std::runtime_error("Fork happened") {}
	fork_throw(const std::string &msg) : std::runtime_error(msg) {}
};

class unknown_arg_throw : public std::runtime_error {
public:
	unknown_arg_throw() : std::runtime_error("Unknown argumunt given") {}
	unknown_arg_throw(const std::string &msg) : std::runtime_error(msg) {}
};

class combine_needs_2_args_error : public std::runtime_error {
public:
	combine_needs_2_args_error() : std::runtime_error("combine needs arguments: <text1> <text2>") {}
	combine_needs_2_args_error(const std::string &msg) : std::runtime_error(msg) {}
};

class obj_create_error : public std::runtime_error {
public:
	obj_create_error() : std::runtime_error("Failed to create object") {}
	obj_create_error(const std::string &msg) : std::runtime_error(msg) {}
};

void clean_up(void *memtofree1, void* memtofree2);
void clean_up_without_threads(void *memtofree1, void* memtofree2);

template<typename... Args>
void gettextize_format(const char *format, Args&&... args)
{ fprintf(stderr, _(format), args...); }

// explicit specialization for no arguments to avoid the 
// "format not a string literal and no format arguments" warning
inline void gettextize_format(const char *format)
{ fputs(_(format), stderr); }

#define NORM_ERR(...) do { \
	fprintf(stderr, PACKAGE_NAME": "); \
        gettextize_format(__VA_ARGS__); \
	fputs("\n", stderr); \
} while(0)

/* critical error */
#define CRIT_ERR(memtofree1, memtofree2, ...) \
	{ NORM_ERR(__VA_ARGS__); clean_up(memtofree1, memtofree2); exit(EXIT_FAILURE); }

#define THREAD_CRIT_ERR(memtofree1, memtofree2, ...) \
	{ NORM_ERR(__VA_ARGS__); clean_up_without_threads(memtofree1, memtofree2); return; }

namespace conky {
    class error : public std::runtime_error {
    public:
        error(const std::string &msg)
            : std::runtime_error(msg)
        {}
    };
}

/* debugging output */
extern int global_debug_level;
#define __DBGP(level, ...) do {\
	if (global_debug_level > level) { \
		fprintf(stderr, "DEBUG(%d) [" __FILE__ ":%d]: ", level, __LINE__); \
                gettextize_format(__VA_ARGS__); \
                fputs("\n", stderr); \
	} \
} while(0)
#define DBGP(...) __DBGP(0, __VA_ARGS__)
#define DBGP2(...) __DBGP(1, __VA_ARGS__)

#endif /* _LOGGING_H */
