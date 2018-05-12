/*
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
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "conky.h"
#include "logging.h"
#include "temphelper.h"
#include "text_object.h"

#define BUFLEN 512

static conky::simple_config_setting<std::string> hddtemp_host("hddtemp_host",
                                                              "localhost",
                                                              false);
static conky::simple_config_setting<std::string> hddtemp_port("hddtemp_port",
                                                              "7634", false);

struct hdd_info {
  hdd_info() : next(0) {}
  struct hdd_info *next;
  char *dev;
  short temp;
  char unit;
};

struct hdd_info hdd_info_head;

static void __free_hddtemp_info(struct hdd_info *hdi) {
  if (hdi->next) __free_hddtemp_info(hdi->next);
  free(hdi->dev);
  delete hdi;
}

static void free_hddtemp_info(void) {
  DBGP("free_hddtemp_info() called");
  if (!hdd_info_head.next) return;
  __free_hddtemp_info(hdd_info_head.next);
  hdd_info_head.next = NULL;
}

static void add_hddtemp_info(char *dev, short temp, char unit) {
  struct hdd_info *hdi = &hdd_info_head;

  DBGP("add_hddtemp_info(%s, %d, %c) being called", dev, temp, unit);
  while (hdi->next) hdi = hdi->next;

  hdi->next = new hdd_info;
  memset(hdi->next, 0, sizeof(struct hdd_info));
  hdi->next->dev = strdup(dev);
  hdi->next->temp = temp;
  hdi->next->unit = unit;
}

static char *fetch_hddtemp_output(void) {
  int sockfd;
  char *buf = NULL;
  int buflen, offset = 0, rlen;
  struct addrinfo hints, *result, *rp;
  int i;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; /* XXX: hddtemp has no ipv6 support (yet?) */
  hints.ai_socktype = SOCK_STREAM;

  if ((i = getaddrinfo(hddtemp_host.get(*state).c_str(),
                       hddtemp_port.get(*state).c_str(), &hints, &result))) {
    NORM_ERR("getaddrinfo(): %s", gai_strerror(i));
    return NULL;
  }

  for (rp = result; rp; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd == -1) continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;
    close(sockfd);
  }
  if (!rp) {
    NORM_ERR("could not connect to hddtemp host");
    goto GET_OUT;
  }

  buflen = 1024;
  buf = (char *)malloc(buflen);
  while ((rlen = recv(sockfd, buf + offset, buflen - offset - 1, 0)) > 0) {
    offset += rlen;
    if (buflen - offset < 1) {
      buflen += 1024;
      buf = (char *)realloc(buf, buflen);
    }
  }
  if (rlen < 0) perror("recv");

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
                        char **saveptr) {
  char *line_s, *cval, *endptr;
  static char *p = 0;

  if (line) {
    *saveptr = strdup(line);
    p = *saveptr;
  }
  line_s = *saveptr;

again:
  if (!*p) goto out_fail;
  /* read the device */
  *dev = ++p;
  if (!(p = strchr(p, line_s[0]))) goto out_fail;
  *(p++) = '\0';
  /* jump over the devname */
  if (!(p = strchr(p, line_s[0]))) goto out_fail;
  /* read the value */
  cval = ++p;
  if (!(p = strchr(p, line_s[0]))) goto out_fail;
  *(p++) = '\0';
  *unit = *(p++);
  *val = strtol(cval, &endptr, 10);
  if (*endptr) {
    if (!(p = strchr(p, line_s[0]))) goto out_fail;

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

int update_hddtemp(void) {
  char *data, *dev, unit, *saveptr;
  short val;
  static double last_hddtemp_update = 0.0;

  /* limit tcp connection overhead */
  if (current_update_time - last_hddtemp_update < 5) return 0;
  last_hddtemp_update = current_update_time;

  free_hddtemp_info();

  if (!(data = fetch_hddtemp_output())) return 0;

  if (read_hdd_val(data, &dev, &val, &unit, &saveptr)) {
    free(data);
    return 0;
  }
  do {
    add_hddtemp_info(dev, val, unit);
  } while (!read_hdd_val(NULL, &dev, &val, &unit, &saveptr));
  free(data);
  return 0;
}

void free_hddtemp(struct text_object *obj) {
  free_hddtemp_info();
  free_and_zero(obj->data.s);
}

static int get_hddtemp_info(const char *dev, short *val, char *unit) {
  struct hdd_info *hdi = hdd_info_head.next;

  /* if no dev is given, just use hdd_info_head->next */
  while (dev && hdi) {
    if (!strcmp(dev, hdi->dev)) break;
    hdi = hdi->next;
  }
  if (!hdi) return 1;

  *val = hdi->temp;
  *unit = hdi->unit;
  return 0;
}

void print_hddtemp(struct text_object *obj, char *p, int p_max_size) {
  short val;
  char unit;

  if (get_hddtemp_info(obj->data.s, &val, &unit)) {
    snprintf(p, p_max_size, "N/A");
  } else {
    temp_print(p, p_max_size, (double)val,
               (unit == 'C' ? TEMP_CELSIUS : TEMP_FAHRENHEIT));
  }
}
