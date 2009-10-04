/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * tcp-portmon.h - libtcp-portmon hooks protoypes
 *
 * Copyright (C) 2008 Phil Sutter <Phil@nwl.cc>
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
 * $Id$
 *
 */
#ifndef _TCP_PORTMON_H
#define _TCP_PORTMON_H

#include "libtcp-portmon.h"

struct tcp_port_monitor_data {
	/* starting port to monitor */
	in_port_t port_range_begin;
	/* ending port to monitor */
	in_port_t port_range_end;
	/* enum from libtcp-portmon.h, e.g. COUNT, etc. */
	int item;
	/* 0 to n-1 connections. */
	int connection_index;
};

int tcp_portmon_init(const char *, struct tcp_port_monitor_data *);
int tcp_portmon_action(char *, int, struct tcp_port_monitor_data *);
void tcp_portmon_update(void);
int tcp_portmon_clear(void);
int tcp_portmon_set_max_connections(int);

#endif /* _TCP_PORTMON_H */
