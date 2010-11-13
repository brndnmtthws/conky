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
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <cinttypes>

struct read_tcpip_data {
	char *host;
	unsigned int port;
};

void parse_read_tcpip_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
	struct read_tcpip_data *rtd;

	rtd = (struct read_tcpip_data *) malloc(sizeof(struct read_tcpip_data));
	memset(rtd, 0, sizeof(struct read_tcpip_data));

	rtd->host = (char *) malloc(text_buffer_size.get(*state));
	sscanf(arg, "%s", rtd->host);
	sscanf(arg+strlen(rtd->host), "%u", &(rtd->port));
	if(rtd->port == 0) {
		rtd->port = atoi(rtd->host);
		strcpy(rtd->host,"localhost");
	}
	if(rtd->port < 1 || rtd->port > 65535)
		CRIT_ERR(obj, free_at_crash, "read_tcp and read_udp need a port from 1 to 65535 as argument");

	rtd->port = htons(rtd->port);
	obj->data.opaque = rtd;
}

void parse_tcp_ping_arg(struct text_object *obj, const char *arg, void *free_at_crash)
{
#define DEFAULT_TCP_PING_PORT 80
	struct sockaddr_in *addr;
	char *hostname;
	struct hostent* he;

	addr = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	obj->data.opaque = addr;
	memset(addr, 0, sizeof(struct sockaddr_in));
	hostname = (char *) malloc(strlen(arg)+1);
	switch( sscanf(arg, "%s %"SCNu16, hostname, &(addr->sin_port)) ) {
	case 1:
		addr->sin_port = DEFAULT_TCP_PING_PORT;
		break;
	case 2:
		break;
	default:	//this point should never be reached
		free(hostname);
		CRIT_ERR(obj, free_at_crash, "tcp_ping: Reading arguments failed");
	}
	if(!(he = gethostbyname(hostname))) {
		NORM_ERR("tcp_ping: Problem with resolving '%s', using 'localhost' instead", hostname);
		if(!(he = gethostbyname("localhost"))) {
			free(hostname);
			CRIT_ERR(obj, free_at_crash, "tcp_ping: Resolving 'localhost' also failed");
		}
	}
	free(hostname);
	addr->sin_port = htons(addr->sin_port);
	addr->sin_family = he->h_addrtype;
	memcpy(&(addr->sin_addr), he->h_addr, he->h_length);
}

void print_tcp_ping(struct text_object *obj, char *p, int p_max_size)
{
	struct timeval tv1, tv2, timeout;
	struct sockaddr_in *addr = (struct sockaddr_in *) obj->data.opaque;
	int addrlen = sizeof(struct sockaddr);
	int sock = socket(addr->sin_family, SOCK_STREAM, IPPROTO_TCP);
	unsigned long long usecdiff;
	fd_set writefds;

	if(sock != -1) {
		fcntl(sock, F_SETFL, O_NONBLOCK | fcntl(sock, F_GETFL));

		FD_ZERO(&writefds);
		FD_SET(sock, &writefds);
#define TCP_PING_TIMEOUT 10
		timeout.tv_sec = (int) TCP_PING_TIMEOUT;
		timeout.tv_usec = (TCP_PING_TIMEOUT - timeout.tv_sec) * 1000000;
		connect(sock, (struct sockaddr*) addr, addrlen);	//this will "fail" because sock is non-blocking
		if(errno == EINPROGRESS) {	//but EINPROGRESS is only a "false fail"
			gettimeofday(&tv1, 0);
			if(select(sock+1, NULL, &writefds, NULL, &timeout) != -1) {
				gettimeofday(&tv2, 0);
				usecdiff = ((tv2.tv_sec - tv1.tv_sec) * 1000000) + tv2.tv_usec - tv1.tv_usec;
				if(usecdiff <= TCP_PING_TIMEOUT * 1000000) {
					snprintf(p, p_max_size, "%llu", usecdiff);
				} else {
#define TCP_PING_FAILED "down"
					snprintf(p, p_max_size, TCP_PING_FAILED);
				}
			} else {
				NORM_ERR("tcp_ping: Couldn't wait on the 'pong'");
			}
		} else {
			NORM_ERR("tcp_ping: Couldn't start connection");
		}
		close(sock);
	} else {
		NORM_ERR("tcp_ping: Couldn't create socket");
	}
}

void print_read_tcpip(struct text_object *obj, char *p, int p_max_size, int protocol)
{
	int sock, received;
	fd_set readfds;
	struct timeval tv;
	struct read_tcpip_data *rtd = (struct read_tcpip_data *) obj->data.opaque;
	struct addrinfo hints;
	struct addrinfo* airesult, *rp;
	char portbuf[8];

	if (!rtd)
		return;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = protocol==IPPROTO_TCP ? SOCK_STREAM : SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = protocol;
	snprintf(portbuf, 8, "%d", rtd->port);
	if (getaddrinfo(rtd->host, portbuf, &hints, &airesult)) {
		NORM_ERR("%s: Problem with resolving the hostname", protocol == IPPROTO_TCP ? "read_tcp" : "read_udp");
		return;
	}
	for (rp = airesult; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1) {
			continue;
		}
		if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sock);
		return;
	}
	freeaddrinfo(airesult);
	if (rp == NULL) {
		if(protocol == IPPROTO_TCP) {
			NORM_ERR("read_tcp: Couldn't create a connection");
		} else {
			NORM_ERR("read_udp: Couldn't listen"); //other error because udp is connectionless
		}
		return;
	}
	if(protocol == IPPROTO_UDP) {
		//when using udp send a zero-length packet to let the other end know of our existence
		if(write(sock, NULL, 0) < 0) {
			NORM_ERR("read_udp: Couldn't create a empty package");
		}
	}
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if(select(sock + 1, &readfds, NULL, NULL, &tv) > 0){
		received = recv(sock, p, p_max_size, 0);
		if(received != -1) p[received] = 0; else p[0] = 0;
	}
	close(sock);
}

void print_read_tcp(struct text_object *obj, char *p, int p_max_size)
{
	print_read_tcpip(obj, p, p_max_size, IPPROTO_TCP);
}

void print_read_udp(struct text_object *obj, char *p, int p_max_size)
{
	print_read_tcpip(obj, p, p_max_size, IPPROTO_UDP);
}

void free_read_tcpip(struct text_object *obj)
{
	struct read_tcpip_data *rtd = (struct read_tcpip_data *) obj->data.opaque;

	if (!rtd)
		return;

	free_and_zero(rtd->host);
	free_and_zero(obj->data.opaque);
}

void free_tcp_ping(struct text_object *obj)
{
	struct sockaddr_in *addr = (struct sockaddr_in *) obj->data.opaque;

	if (!addr)
		return;

	free_and_zero(obj->data.opaque);
}
