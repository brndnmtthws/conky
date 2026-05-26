/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _LINUX_H
#define _LINUX_H

#include <filesystem>

#include "config.h"

const std::filesystem::path process_directory{"/proc"};

bool is_conky_already_running(void);

int update_gateway_info(void);

enum { PB_BATT_STATUS, PB_BATT_PERCENT, PB_BATT_TIME };

int update_stat(void);

extern char e_iface[64];
extern char interfaces_arr[MAX_NET_INTERFACES][64];

#endif /* _LINUX_H */
