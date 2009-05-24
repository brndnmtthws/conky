/* Conky, a system monitor, based on torsmo
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
#define PORT 7634

char buf[BUFLEN];

int scan_hddtemp(const char *arg, char **dev, char **addr, int *port)
{
	char buf1[32], buf2[64];
	int n, ret;

	if (!arg)
		return 1;

	if ((ret = sscanf(arg, "%31s %63s %d", buf1, buf2, &n)) < 1)
		return 1;

	if (strncmp(buf1, "/dev/", 5)) {
		strncpy(buf1 + 5, buf1, 32 - 5);
		strncpy(buf1, "/dev/", 5);
	}
	*dev = strndup(buf1, text_buffer_size);

	if (ret >= 2) {
		*addr = strndup(buf2, text_buffer_size);
	} else {
		*addr = strndup("127.0.0.1", text_buffer_size);
	}

	if (ret == 3) {
		*port = n;
	} else {
		*port = PORT;
	}

	return 0;
}

/* this is an iterator:
 * set line to NULL in consecutive calls to get the next field
 * returns "<dev><unit><val>" or NULL on error
 */
static char *read_hdd_val(const char *line)
{
	static char line_s[512] = "\0";
	static char *p = 0;
	char *dev, *val, unit;
	char *ret = NULL;

	if (line) {
		snprintf(line_s, 512, "%s", line);
		p = line_s;
	}
	if (!(*line_s))
		return ret;
	/* read the device */
	dev = ++p;
	if (!p) return ret;
	if (!(p = strchr(p, line_s[0])))
		return ret;
	*(p++) = '\0';
	/* jump over the devname */
	if (!(p = strchr(p, line_s[0])))
		return ret;
	/* read the value */
	val = ++p;
	if (!(p = strchr(p, line_s[0])))
		return ret;
	*(p++) = '\0';
	unit = *(p++);
	/* preset p for next call */
	p = strchr(p + 1, line_s[0]);

	if (dev && *dev && val && *val) {
		asprintf(&ret, "%s%c%s", dev, unit, val);
	}
	return ret;
}

/* returns <unit><val> or NULL on error or N/A */
char *get_hddtemp_info(char *dev, char *hostaddr, int port)
{
	int sockfd = 0;
	struct hostent he, *he_res = 0;
	int he_errno;
	char hostbuff[2048];
	struct sockaddr_in addr;
	struct timeval tv;
	fd_set rfds;
	int len, i;
	char *p, *r = NULL;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		goto GET_OUT;
	}

#ifdef HAVE_GETHOSTBYNAME_R
	if (gethostbyname_r(hostaddr, &he, hostbuff,
	                    sizeof(hostbuff), &he_res, &he_errno)) {
		ERR("hddtemp gethostbyname_r: %s", hstrerror(h_errno));
#else /* HAVE_GETHOSTBYNAME_R */
	if (!(he_res = gethostbyname(hostaddr))) {
		perror("gethostbyname()");
#endif /* HAVE_GETHOSTBYNAME_R */
		goto GET_OUT;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *((struct in_addr *) he_res->h_addr);
	memset(&(addr.sin_zero), 0, 8);

	if (connect(sockfd, (struct sockaddr *) &addr,
				sizeof(struct sockaddr)) == -1) {
		perror("connect");
		goto GET_OUT;
	}

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	/* We're going to wait up to a half second to see whether there's any
	 * data available. Polling with timeout set to 0 doesn't seem to work
	 * with hddtemp.
	 */
	tv.tv_sec = 0;
	tv.tv_usec = 500000;

	i = select(sockfd + 1, &rfds, NULL, NULL, &tv);
	if (i == -1) { /* select() failed */
		if (errno == EINTR) {
			/* silently ignore interrupted system call */
			goto GET_OUT;
		} else {
			perror("select");
		}
	} else if (i == 0) { /* select() timeouted */
		ERR("hddtemp had nothing for us");
		goto GET_OUT;
	}

	p = buf;
	len = 0;
	do {
		i = recv(sockfd, p, BUFLEN - (p - buf), 0);
		if (i < 0) {
			perror("recv");
			break;
		}
		len += i;
		p += i;
	} while (i > 0 && p < buf + BUFLEN - 1);

	if (len < 2) {
		ERR("hddtemp returned nada");
		goto GET_OUT;
	}

	buf[len] = 0;

	if ((p = read_hdd_val(buf)) == NULL)
		goto GET_OUT;
	do {
		if (p >= buf + BUFLEN) break;
		if (!strncmp(dev, p, strlen(dev)))
			asprintf(&r, "%s", p + strlen(dev));
		free(p);
	} while(!r && (p = read_hdd_val(NULL)) != NULL);

GET_OUT:
	close(sockfd);
	return r;
}
