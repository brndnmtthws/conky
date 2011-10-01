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

#ifndef CPPWRAP_HH
#define CPPWRAP_HH

#ifdef HAVE_O_CLOEXEC
#include <fcntl.h>
#else
enum { O_CLOEXEC = 02000000 };
#endif

#include <cerrno>
#include <stdexcept>
#include <string>
#include <utility>

std::string strerror_r(int errnum);
std::pair<int, int> pipe2(int flags);

class errno_error: public std::runtime_error {
	typedef std::runtime_error Base;

public:
	errno_error(const std::string &prefix, int err_ = errno)
		: Base(prefix + ": " + strerror_r(err_)), err(err_)
	{}

	const int err;
};

#endif /* CPPWRAP_HH */
