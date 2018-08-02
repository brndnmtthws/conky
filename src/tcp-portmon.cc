/*
 *
 * tcp-portmon.c - libtcp-portmon hooks
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
#include "conky.h"
#include "libtcp-portmon.h"
#include "logging.h"
#include "tcp-portmon.h"
#include "text_object.h"

static tcp_port_monitor_collection_t *pmc = nullptr;

static conky::range_config_setting<int> max_port_monitor_connections(
    "max_port_monitor_connections", 0, std::numeric_limits<int>::max(),
    MAX_PORT_MONITOR_CONNECTIONS_DEFAULT, false);

int tcp_portmon_init(struct text_object *obj, const char *arg) {
  int item = -1;
  int argc;
  int port_begin;
  int port_end;
  int connection_index;
  char itembuf[32];
  struct tcp_port_monitor_data *pmd;

  memset(itembuf, 0, sizeof(itembuf));
  connection_index = 0;
  /* massive argument checking */
  argc = sscanf(arg, "%d %d %31s %d", &port_begin, &port_end, itembuf,
                &connection_index);
  if ((argc != 3) && (argc != 4)) {
    CRIT_ERR(nullptr, NULL, "tcp_portmon: requires 3 or 4 arguments");
  }
  if ((port_begin < 1) || (port_begin > 65535) || (port_end < 1) ||
      (port_end > 65535)) {
    CRIT_ERR(nullptr, NULL, "tcp_portmon: port values must be from 1 to 65535");
  }
  if (port_begin > port_end) {
    CRIT_ERR(nullptr, NULL,
             "tcp_portmon: starting port must be <= ending port");
  }
  if (strncmp(itembuf, "count", 31) == EQUAL) {
    item = COUNT;
  } else if (strncmp(itembuf, "rip", 31) == EQUAL) {
    item = REMOTEIP;
  } else if (strncmp(itembuf, "rhost", 31) == EQUAL) {
    item = REMOTEHOST;
  } else if (strncmp(itembuf, "rport", 31) == EQUAL) {
    item = REMOTEPORT;
  } else if (strncmp(itembuf, "rservice", 31) == EQUAL) {
    item = REMOTESERVICE;
  } else if (strncmp(itembuf, "lip", 31) == EQUAL) {
    item = LOCALIP;
  } else if (strncmp(itembuf, "lhost", 31) == EQUAL) {
    item = LOCALHOST;
  } else if (strncmp(itembuf, "lport", 31) == EQUAL) {
    item = LOCALPORT;
  } else if (strncmp(itembuf, "lservice", 31) == EQUAL) {
    item = LOCALSERVICE;
  } else {
    CRIT_ERR(nullptr, NULL, "tcp_portmon: invalid item specified");
  }
  if ((argc == 3) && (item != COUNT)) {
    CRIT_ERR(nullptr, NULL,
             "tcp_portmon: 3 argument form valid only for \"count\" "
             "item");
  }
  if ((argc == 4) && (connection_index < 0)) {
    CRIT_ERR(nullptr, NULL,
             "tcp_portmon: connection index must be non-negative");
  }
  /* ok, args looks good. save the text object data */
  pmd = (tcp_port_monitor_data *)malloc(sizeof(struct tcp_port_monitor_data));
  memset(pmd, 0, sizeof(struct tcp_port_monitor_data));
  pmd->port_range_begin = (in_port_t)port_begin;
  pmd->port_range_end = (in_port_t)port_end;
  pmd->item = item;
  pmd->connection_index = connection_index;
  obj->data.opaque = pmd;

  /* if the port monitor collection hasn't been created,
   * we must create it */
  if (!pmc) {
    pmc = create_tcp_port_monitor_collection();
    if (!pmc) {
      CRIT_ERR(nullptr, NULL,
               "tcp_portmon: unable to create port monitor "
               "collection");
    }
  }

  /* if a port monitor for this port does not exist,
   * create one and add it to the collection */
  if (find_tcp_port_monitor(pmc, port_begin, port_end) == nullptr) {
    tcp_port_monitor_args_t pma;
    memset(&pma, 0, sizeof pma);
    pma.max_port_monitor_connections = max_port_monitor_connections.get(*state);

    /* add the newly created monitor to the collection */
    if (insert_new_tcp_port_monitor_into_collection(pmc, port_begin, port_end,
                                                    &pma) != 0) {
      CRIT_ERR(nullptr, NULL,
               "tcp_portmon: unable to add port monitor to "
               "collection");
    }
  }
  return 0;
}

void tcp_portmon_action(struct text_object *obj, char *p, int p_max_size) {
  struct tcp_port_monitor_data *pmd = (tcp_port_monitor_data *)obj->data.opaque;
  tcp_port_monitor_t *p_monitor;

  if (!pmd) return;

  /* grab a pointer to this port monitor */
  p_monitor =
      find_tcp_port_monitor(pmc, pmd->port_range_begin, pmd->port_range_end);

  if (!p_monitor) {
    snprintf(p, p_max_size, "%s", "monitor not found");
    return;
  }

  /* now grab the text of interest */
  if (peek_tcp_port_monitor(p_monitor, pmd->item, pmd->connection_index, p,
                            p_max_size) != 0) {
    snprintf(p, p_max_size, "%s", "monitor peek error");
  }
}

int tcp_portmon_update(void) {
  update_tcp_port_monitor_collection(pmc);
  return 0;
}

int tcp_portmon_clear(void) {
  destroy_tcp_port_monitor_collection(pmc);
  pmc = nullptr;
  return 0;
}

void tcp_portmon_free(struct text_object *obj) {
  free_and_zero(obj->data.opaque);
}
