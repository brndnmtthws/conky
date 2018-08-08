/*
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

/* forward declare to make gcc happy */
struct text_object;

int tcp_portmon_init(struct text_object *, const char *);
void tcp_portmon_action(struct text_object *, char *, unsigned int);
int tcp_portmon_update(void);
int tcp_portmon_clear(void);
void tcp_portmon_free(struct text_object *);

#endif /* _TCP_PORTMON_H */
