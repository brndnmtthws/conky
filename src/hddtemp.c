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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFLEN 512
#define DEFAULT_PORT "7634"
#define DEFAULT_HOST "127.0.0.1"

static char *hddtemp_host = NULL;
static char *hddtemp_port = NULL;

static struct hdd_info {
	struct hdd_info *next;
	char *dev;
	short temp;
	char unit;
} hdd_info_head = {
	.next = NULL,
};

void set_hddtemp_host(const char *host)
{
	if (hddtemp_host)
		free(hddtemp_host);
	hddtemp_host = strdup(host);
}

void set_hddtemp_port(const char *port)
{
	if (hddtemp_port)
		free(hddtemp_port);
	hddtemp_port = strdup(port);
}

static void __free_hddtemp_info(struct hdd_info *hdi)
{
	if (hdi->next)
		__free_hddtemp_info(hdi->next);
	free(hdi->dev);
	free(hdi);
}

static void free_hddtemp_info(void)
{
	DBGP("free_hddtemp_info() called");
	if (!hdd_info_head.next)
		return;
	__free_hddtemp_info(hdd_info_head.next);
	hdd_info_head.next = NULL;
}

static void add_hddtemp_info(char *dev, short temp, char unit)
{
	struct hdd_info *hdi = &hdd_info_head;

	DBGP("add_hddtemp_info(%s, %d, %c) being called", dev, temp, unit);
	while (hdi->next)
		hdi = hdi->next;

	hdi->next = malloc(sizeof(struct hdd_info));
	memset(hdi->next, 0, sizeof(struct hdd_info));
	hdi->next->dev = strdup(dev);
	hdi->next->temp = temp;
	hdi->next->unit = unit;
}

static char *fetch_hddtemp_output(void)
{
	int sockfd;
	const char *dst_host, *dst_port;
	char *buf = NULL;
	int buflen, offset = 0, rlen;
	struct addrinfo hints, *result, *rp;
	int i;

	dst_host = hddtemp_host ? hddtemp_host : DEFAULT_HOST;
	dst_port = hddtemp_port ? hddtemp_port : DEFAULT_PORT;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	/* XXX: hddtemp has no ipv6 support (yet?) */
	hints.ai_socktype = SOCK_STREAM;

	if ((i = getaddrinfo(dst_host, dst_port, &hints, &result))) {
		NORM_ERR("getaddrinfo(): %s", gai_strerror(i));
		return NULL;
	}

	for (rp = result; rp; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1)
			continue;
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
		close(sockfd);
	}
	if (!rp) {
		NORM_ERR("could not connect to hddtemp host");
		goto GET_OUT;
	}

	buflen = 1024;
	buf = malloc(buflen);
	while ((rlen = recv(sockfd, buf + offset, buflen - offset - 1, 0)) > 0) {
		offset += rlen;
		if (buflen - offset < 1) {
			buflen += 1024;
			buf = realloc(buf, buflen);
		}
	}
	if (rlen < 0)
		perror("recv");

	buf[offset] = '\0';

	close(sockfd);
GET_OUT:
	freeaddrinfo(result);
	return buf;
}

/* this is an iterator:
 * set line to NULL in consecutive calls to get the next field
 * note that exhausing iteration is assumed - otherwise *saveptr
 * is not being freed!
 */
static int read_hdd_val(const char *line, char **dev, short *val, char *unit,
		char **saveptr)
{
	char *line_s, *cval, *endptr;
	static char *p = 0;

	if (line) {
		*saveptr = strdup(line);
		p = *saveptr;
	}
	line_s = *saveptr;

again:
	if(!*p)
		goto out_fail;
	/* read the device */
	*dev = ++p;
	if (!(p = strchr(p, line_s[0])))
		goto out_fail;
	*(p++) = '\0';
	/* jump over the devname */
	if (!(p = strchr(p, line_s[0])))
		goto out_fail;
	/* read the value */
	cval = ++p;
	if (!(p = strchr(p, line_s[0])))
		goto out_fail;
	*(p++) = '\0';
	*unit = *(p++);
	*val = strtol(cval, &endptr, 10);
	if (*endptr) {
		if (!(p = strchr(p, line_s[0])))
			goto out_fail;

		p++;
		goto again;
	}
	
	/* preset p for next call */
	p++;

	return 0;
out_fail:
	free(*saveptr);
	return 1;
}

void update_hddtemp(void) {
	char *data, *dev, unit, *saveptr;
	short val;
	static double last_hddtemp_update = 0.0;

	/* limit tcp connection overhead */
	if (current_update_time - last_hddtemp_update < 5)
		return;
	last_hddtemp_update = current_update_time;

	free_hddtemp_info();

	if (!(data = fetch_hddtemp_output()))
		return;

	if (read_hdd_val(data, &dev, &val, &unit, &saveptr)) {
		free(data);
		return;
	}
	do {
		add_hddtemp_info(dev, val, unit);
	} while (!read_hdd_val(NULL, &dev, &val, &unit, &saveptr));
	free(data);
}

void free_hddtemp(void)
{
	free_hddtemp_info();
	if (hddtemp_host) {
		free(hddtemp_host);
		hddtemp_host = NULL;
	}
	if (hddtemp_port) {
		free(hddtemp_port);
		hddtemp_port = NULL;
	}
}

int get_hddtemp_info(const char *dev, short *val, char *unit)
{
	struct hdd_info *hdi = hdd_info_head.next;

	/* if no dev is given, just use hdd_info_head->next */
	while(dev && hdi) {
		if (!strcmp(dev, hdi->dev))
			break;
		hdi = hdi->next;
	}
	if (!hdi)
		return 1;
	
	*val = hdi->temp;
	*unit = hdi->unit;
	return 0;
}
