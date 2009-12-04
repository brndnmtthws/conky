/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * apcupsd.c:  conky module for APC UPS daemon monitoring
 *
 * Copyright (C) 2009 Jaromir Smrcek <jaromir.smrcek@zoner.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#include "conky.h"
#include "apcupsd.h"
#include "logging.h"
#include "text_object.h"

#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>

//
// encapsulated recv()
//
static int net_recv_ex(int sock, void *buf, int size, struct timeval *tv)
{
	
	fd_set	fds;
	int		res;

	// wait for some data to be read
	do {
		errno = 0;
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		res = select(sock + 1, &fds, NULL, NULL, tv);
	} while (res < 0 && errno == EINTR);
	if (res < 0) return 0;
	if (res == 0) {
		// timeout
		errno = ETIMEDOUT;  // select was succesfull, errno is now 0
		return 0;
	}

	// socket ready, read the data
	do {
		errno = 0;
		res = recv(sock, (char*)buf, size, 0);
	} while (res < 0 && errno == EINTR);
	if (res < 0) return 0;
	if (res == 0) {
		// orderly shutdown
		errno = ENOTCONN;
		return 0;
	}

	return res;
}

//
// read whole buffer or fail
//
static int net_recv(int sock, void* buf, int size) {

	int todo = size;
	int off = 0;
	int len;
	struct timeval tv = { 0, 250000 };

	while (todo) {
		len = net_recv_ex(sock, (char*)buf + off, todo, &tv);
		if (!len) return 0;
		todo -= len;
		off  += len;
	}
	return 1;
}

//
// get one response line
//
static int get_line(int sock, char line[], short linesize) {

	// get the line length
	short sz;
	if (!net_recv(sock, &sz, sizeof(sz))) return -1;
	sz = ntohs(sz);
	if (!sz) return 0;

	// get the line
	while (sz > linesize) {
		// this is just a hack (being lazy), this should not happen anyway
		net_recv(sock, line, linesize);
		sz -= sizeof(line);
	}
	if (!net_recv(sock, line, sz)) return 0;
	line[sz] = 0;
	return sz;
}

#define FILL(NAME,FIELD,FIRST)														\
	if (!strncmp(NAME, line, sizeof(NAME)-1)) {										\
		strncpy(apc->items[FIELD], line+11, APCUPSD_MAXSTR);						\
		/* remove trailing newline and assure termination */						\
		apc->items[FIELD][len-11 > APCUPSD_MAXSTR ? APCUPSD_MAXSTR : len-12] = 0;	\
		if (FIRST) {																\
			char* c;																\
			for (c = apc->items[FIELD]; *c; ++c)									\
				if (*c == ' ' && c > apc->items[FIELD]+2) {							\
					*c = 0;															\
					break;															\
				}																	\
		}																			\
	}

//
// fills in the data received from a socket
//
static int fill_items(int sock, PAPCUPSD_S apc) {

	char line[512];
	int len;
	while ((len = get_line(sock, line, sizeof(line)))) {
		// fill the right types in
		FILL("UPSNAME",		APCUPSD_NAME,		FALSE);
		FILL("MODEL",		APCUPSD_MODEL,		FALSE);
		FILL("UPSMODE",		APCUPSD_UPSMODE,	FALSE);
		FILL("CABLE",		APCUPSD_CABLE,		FALSE);
		FILL("STATUS",		APCUPSD_STATUS,		FALSE);
		FILL("LINEV",		APCUPSD_LINEV,		TRUE);
		FILL("LOADPCT",		APCUPSD_LOAD,		TRUE);
		FILL("BCHARGE",		APCUPSD_CHARGE,		TRUE);
		FILL("TIMELEFT",	APCUPSD_TIMELEFT,	TRUE);
		FILL("ITEMP",		APCUPSD_TEMP,		TRUE);
		FILL("LASTXFER",	APCUPSD_LASTXFER,	FALSE);
	}
	
	return len == 0;
}

//
// Conky update function for apcupsd data
//
void update_apcupsd(void) {

	int i;
	APCUPSD_S apc;
	int sock;

	for (i = 0; i < _APCUPSD_COUNT; ++i)
		memcpy(apc.items[i], "N/A", 4); // including \0

	do {
		struct hostent* he = 0;
		struct sockaddr_in addr;
		short sz = 0;
#ifdef HAVE_GETHOSTBYNAME_R
		struct hostent he_mem;
		int he_errno;
		char hostbuff[2048];
#endif
		//
		// connect to apcupsd daemon
		//
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			perror("socket");
			break;
		}
#ifdef HAVE_GETHOSTBYNAME_R
		if (gethostbyname_r(info.apcupsd.host, &he_mem, hostbuff, sizeof(hostbuff), &he, &he_errno) || !he ) {
			NORM_ERR("APCUPSD gethostbyname_r: %s", hstrerror(h_errno));
			break;
		}
#else /* HAVE_GETHOSTBYNAME_R */
		he = gethostbyname(info.apcupsd.host);
		if (!he) {
			herror("gethostbyname");
			break;
		}
#endif /* HAVE_GETHOSTBYNAME_R */
		
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = info.apcupsd.port;
		memcpy(&addr.sin_addr, he->h_addr, he->h_length);
		if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0) {
			// no error reporting, the daemon is probably not running
			break;
		}
	
		//
		// send status request - "status" - 6B
		//
		sz = htons(6);
		// no waiting to become writeable is really needed
		if (send(sock, &sz, sizeof(sz), 0) != sizeof(sz) || send(sock, "status", 6, 0) != 6) {
			perror("send");
			break;
		}
	
		//
		// read the lines of output and put them into the info structure
		//
		if (!fill_items(sock, &apc)) break;

	} while (0);

	close(sock);

	//
	// "atomically" copy the data into working set
	//
	memcpy(info.apcupsd.items, apc.items, sizeof(info.apcupsd.items));
	return;
}

double apcupsd_loadbarval(struct text_object *obj)
{
	(void)obj;

	return atof(info.apcupsd.items[APCUPSD_LOAD]);
}

#define APCUPSD_PRINT_GENERATOR(name, idx)                                  \
void print_apcupsd_##name(struct text_object *obj, char *p, int p_max_size) \
{                                                                           \
	(void)obj;                                                              \
	snprintf(p, p_max_size, "%s", info.apcupsd.items[APCUPSD_##idx]);       \
}

APCUPSD_PRINT_GENERATOR(name, NAME)
APCUPSD_PRINT_GENERATOR(model, MODEL)
APCUPSD_PRINT_GENERATOR(upsmode, UPSMODE)
APCUPSD_PRINT_GENERATOR(cable, CABLE)
APCUPSD_PRINT_GENERATOR(status, STATUS)
APCUPSD_PRINT_GENERATOR(linev, LINEV)
APCUPSD_PRINT_GENERATOR(load, LOAD)
APCUPSD_PRINT_GENERATOR(charge, CHARGE)
APCUPSD_PRINT_GENERATOR(timeleft, TIMELEFT)
APCUPSD_PRINT_GENERATOR(temp, TEMP)
APCUPSD_PRINT_GENERATOR(lastxfer, LASTXFER)

#undef APCUPSD_PRINT_GENERATOR
