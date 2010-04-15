/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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

#include "conky.h"
#include "logging.h"
#include "text_object.h"
#include <netdb.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <netinet/in.h>

struct read_tcp_data {
	char *host;
	unsigned int port;
};

void parse_read_tcp_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	struct read_tcp_data *rtd;

	rtd = (struct read_tcp_data *) malloc(sizeof(struct read_tcp_data));
	memset(rtd, 0, sizeof(struct read_tcp_data));

	rtd->host = (char *) malloc(text_buffer_size);
	sscanf(arg, "%s", rtd->host);
	sscanf(arg+strlen(rtd->host), "%u", &(rtd->port));
	if(rtd->port == 0) {
		rtd->port = atoi(rtd->host);
		strcpy(rtd->host,"localhost");
	}
	if(rtd->port < 1 || rtd->port > 65535)
		CRIT_ERR(obj, free_at_crash, "read_tcp: Needs \"(host) port\" as argument(s)");

	rtd->port = htons(rtd->port);
	obj->data.opaque = rtd;
}

void print_read_tcp(struct text_object *obj, char *p, int p_max_size)
{
	int sock, received;
	struct sockaddr_in addr;
	struct hostent* he;
	fd_set readfds;
	struct timeval tv;
	struct read_tcp_data *rtd = (struct read_tcp_data *) obj->data.opaque;

	if (!rtd)
		return;

	if (!(he = gethostbyname(rtd->host))) {
		NORM_ERR("read_tcp: Problem with resolving the hostname");
		return;
	}
	if ((sock = socket(he->h_addrtype, SOCK_STREAM, 0)) == -1) {
		NORM_ERR("read_tcp: Couldn't create a socket");
		return;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = rtd->port;
	memcpy(&addr.sin_addr, he->h_addr, he->h_length);
	if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)) != 0) {
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
	struct read_tcp_data *rtd = (struct read_tcp_data *) obj->data.opaque;

	if (!rtd)
		return;

	free_and_zero(rtd->host);
	free_and_zero(obj->data.opaque);
}
