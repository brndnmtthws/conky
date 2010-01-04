/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2010 Brenden Matthews, et. al.  (see AUTHORS)
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

#ifndef _c_funcs_h_
#define _c_funcs_h_

#ifdef __cplusplus
extern "C" {
#endif

/* these functions provide a workaround for a problem in g++ claiming to not
 * support long longs with these functions */

int sscanf_c(const char *__restrict s, const char *__restrict format, ...);

int snprintf_c(char *__restrict s, size_t maxlen, const char *__restrict format,
		...);

#ifdef __cplusplus
}
#endif

#endif /* _c_funcs_h_ */

