/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
 * $Id$ */

#include "conky.h"
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

int scan_hddtemp(const char *arg, char **dev, char **addr, int *port, char** temp)
{
	char buf1[32], buf2[64];
	int n, ret;

	ret = sscanf(arg, "%31s %63s %d", buf1, buf2, &n);

	if (ret < 1) {
		return -1;
	}

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

	*temp = malloc(text_buffer_size);
	memset(*temp, 0, text_buffer_size);

	return 0;
}

char *get_hddtemp_info(char *dev, char *hostaddr, int port, char *unit)
{
	int sockfd = 0;
	struct hostent he, *he_res = 0;
	int he_errno;
	char hostbuff[2048];
	struct sockaddr_in addr;
	struct timeval tv;
	fd_set rfds;
	int len, i, devlen = strlen(dev);
	char sep;
	char *p, *out, *r = NULL;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return NULL;
	}

	do {
#ifdef HAVE_GETHOSTBYNAME_R
		if (gethostbyname_r(hostaddr, &he, hostbuff, sizeof(hostbuff), &he_res, &he_errno)) {	// get the host info
			ERR("hddtemp gethostbyname_r: %s", hstrerror(h_errno));
			break;
		}
#else /* HAVE_GETHOSTBYNAME_R */
		he_res = gethostbyname(hostaddr);
		if (!he_res) {
			perror("gethostbyname");
			break;
		}
#endif /* HAVE_GETHOSTBYNAME_R */

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr = *((struct in_addr *) he_res->h_addr);
		memset(&(addr.sin_zero), 0, 8);

		if (connect(sockfd, (struct sockaddr *) &addr,
					sizeof(struct sockaddr)) == -1) {
			perror("connect");
			break;
		}

		FD_ZERO(&rfds);
		FD_SET(sockfd, &rfds);

		/* We're going to wait up to a quarter a second to see whether there's
		 * any data available. Polling with timeout set to 0 doesn't seem to work
		 * with hddtemp. */
		tv.tv_sec = 0;
		tv.tv_usec = 250000;

		i = select(sockfd + 1, &rfds, NULL, NULL, &tv);
		if (i == -1) {
			if (errno == EINTR) {	/* silently ignore interrupted system call */
				break;
			} else {
				perror("select");
			}
		}

		/* No data available */
		if (i <= 0) {
			break;
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
			break;
		}

		buf[len] = 0;

		/* The first character read is the separator. */
		sep = buf[0];
		p = buf + 1;

		while (*p) {
			if (!strncmp(p, dev, devlen)) {
				p += devlen + 1;
				p = strchr(p, sep);
				if (!p) {
					break;
				}
				p++;
				out = p;
				p = strchr(p, sep);
				if (!p) {
					break;
				}
				*p = '\0';
				p++;
				*unit = *p;
				if (!strncmp(out, "NA", 2)) {
					strncpy(buf, "N/A", BUFLEN);
					r = buf;
				} else {
					r = out;
				}
				break;
			} else {
				for (i = 0; i < 5; i++) {
					p = strchr(p, sep);
					if (!p) {
						break;
					}
					p++;
				}
				if (!p && i < 5) {
					break;
				}
			}
		}
	} while (0);
	close(sockfd);
	return r;
}
