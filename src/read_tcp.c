/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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

#include "logging.h"
#include "text_object.h"

void parse_read_tcp_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	obj->data.read_tcp.host = malloc(text_buffer_size);
	sscanf(arg, "%s", obj->data.read_tcp.host);
	sscanf(arg+strlen(obj->data.read_tcp.host), "%u", &(obj->data.read_tcp.port));
	if(obj->data.read_tcp.port == 0) {
		obj->data.read_tcp.port = atoi(obj->data.read_tcp.host);
		strcpy(obj->data.read_tcp.host,"localhost");
	}
	if(obj->data.read_tcp.port < 1 || obj->data.read_tcp.port > 65535)
		CRIT_ERR(obj, free_at_crash, "read_tcp: Needs \"(host) port\" as argument(s)");

	obj->data.read_tcp.port = htons(obj->data.read_tcp.port);
}

void print_read_tcp(struct text_object *obj, char *p, int p_max_size)
{
	int sock, received;
	struct sockaddr_in addr;
	struct hostent* he;
	fd_set readfds;
	struct timeval tv;

	if (!(he = gethostbyname(obj->data.read_tcp.host))) {
		NORM_ERR("read_tcp: Problem with resolving the hostname");
		return;
	}
	if ((sock = socket(he->h_addrtype, SOCK_STREAM, 0)) == -1) {
		NORM_ERR("read_tcp: Couldn't create a socket");
		return;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = obj->data.read_tcp.port;
	memcpy(&addr.sin_addr, he->h_addr, he->h_length);
	if (!connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr))) {
		NORM_ERR("read_tcp: Couldn't create a connection");
		return;
	}
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if(select(sock + 1, &readfds, NULL, NULL, &tv) > 0){
		received = recv(sock, p, p_max_size, 0);
		p[received] = 0;
	}
	close(sock);
}

void free_read_tcp(struct text_object *obj)
{
	if (obj->data.read_tcp.host)
		free(obj->data.read_tcp.host);
}
