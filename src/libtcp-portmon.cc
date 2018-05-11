/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * libtcp-portmon.c:  tcp port monitoring library.
 *
 * Copyright (C) 2005-2007 Philip Kovacs pkovacs@users.sourceforge.net
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "libtcp-portmon.h"

#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

/* -------------------------------------------------------------------
 * IMPLEMENTATION INTERFACE
 *
 * Implementation-specific interface begins here.  Clients should not
 * manipulate these structures directly, nor call the defined helper
 * functions.  Use the "Client interface" functions defined at bottom.
 * ------------------------------------------------------------------- */

namespace {
/* ------------------------------------------------------------------------
 * A single tcp connection
 * ------------------------------------------------------------------------ */
struct tcp_connection_t {
  /* connection's key in monitor hash */
  struct in6_addr local_addr;
  struct in6_addr remote_addr;
  in_port_t local_port;
  in_port_t remote_port;
};

/* hash function for tcp connections */
struct tcp_connection_hash {
  size_t operator()(const tcp_connection_t &a) const {
    size_t hash = 0;
    size_t i;

    hash = hash * 47 + a.local_port;
    hash = hash * 47 + a.remote_port;
    for (i = 0; i < sizeof(a.local_addr.s6_addr); ++i)
      hash = hash * 47 + a.local_addr.s6_addr[i];
    for (i = 0; i < sizeof(a.remote_addr.s6_addr); ++i)
      hash = hash * 47 + a.remote_addr.s6_addr[i];

    return hash;
  }
};

/* comparison function for tcp connections */
bool operator==(const tcp_connection_t &a, const tcp_connection_t &b) {
  return a.local_port == b.local_port && a.remote_port == b.remote_port &&
         !std::memcmp(&a.local_addr, &b.local_addr, sizeof(a.local_addr)) &&
         !std::memcmp(&a.remote_addr.s6_addr, &b.remote_addr,
                      sizeof(a.remote_addr));
}

/* ------------------------------------------------------------------------
 * A hash table containing tcp connection
 *
 * The second parameter provides the mechanism for removing connections if
 * they are not seen again in subsequent update cycles.
 * ------------------------------------------------------------------------ */
typedef std::unordered_map<tcp_connection_t, int, tcp_connection_hash>
    connection_hash_t;

/* start and end of port monitor range. Set start=end to monitor a single port
 */
typedef std::pair<in_port_t, in_port_t> port_range_t;

/* hash function for port ranges */
struct port_range_hash {
  size_t operator()(const port_range_t &a) const {
    return a.first * 47 + a.second;
  }
};

typedef std::unordered_map<port_range_t, tcp_port_monitor_t, port_range_hash>
    monitor_hash_t;

}  // namespace

/* --------------
 * A port monitor
 * -------------- */
struct _tcp_port_monitor_t {
  /* hash table of pointers into connection list */
  connection_hash_t hash;
  /* array of connection pointers for O(1) peeking
   * these point into the hash table*/
  std::vector<const tcp_connection_t *> p_peek;

  _tcp_port_monitor_t(int max_connections)
      : hash(),
        p_peek(max_connections, static_cast<const tcp_connection_t *>(NULL)) {}

  _tcp_port_monitor_t(const _tcp_port_monitor_t &other)
      : hash(other.hash),
        p_peek(other.p_peek.size(),
               static_cast<const tcp_connection_t *>(NULL)) {
    // we must rebuild the peek table because the pointers are no longer valid
    rebuild_peek_table();
  }

  void rebuild_peek_table() {
    /* Run through the monitor's connections and rebuild the peek table of
     * connection pointers.  This is done so peeking into the monitor can be
     * done in O(1) time instead of O(n) time for each peek. */

    /* zero out the peek array */
    std::fill(p_peek.begin(), p_peek.end(),
              static_cast<tcp_connection_t *>(NULL));

    size_t i = 0;
    for (connection_hash_t::iterator j = hash.begin(); j != hash.end();
         ++j, ++i) {
      p_peek[i] = &j->first;
    }
  }

 private:
  // we don't need this atm
  const _tcp_port_monitor_t &operator=(const _tcp_port_monitor_t &);
};

/* -----------------------------
 * A tcp port monitor collection
 * ----------------------------- */
struct _tcp_port_monitor_collection_t {
  /* hash table of monitors */
  monitor_hash_t hash;
};

namespace {
/* ---------------------------------------
 * A port monitor utility function typedef
 * --------------------------------------- */
typedef void (*tcp_port_monitor_function_ptr_t)(
    monitor_hash_t::value_type &monitor, void *p_void);

void age_tcp_port_monitor(monitor_hash_t::value_type &monitor, void *p_void) {
  /* Run through the monitor's connections and decrement the age variable.
   * If the age goes negative, we remove the connection from the monitor.
   * Function takes O(n) time on the number of connections. */

  if (p_void) { /* p_void should be NULL in this context */
    return;
  }

  for (connection_hash_t::iterator i = monitor.second.hash.begin();
       i != monitor.second.hash.end();) {
    if (--i->second >= 0)
      ++i;
    else {
      /* connection is old.  remove connection from the hash. */
      /* erase shouldn't invalidate iterators */
      monitor.second.hash.erase(i++);
    }
  }
}

void rebuild_tcp_port_monitor_peek_table(monitor_hash_t::value_type &monitor,
                                         void *p_void) {
  if (p_void) { /* p_void should be NULL in this context */
    return;
  }

  monitor.second.rebuild_peek_table();
}

void show_connection_to_tcp_port_monitor(monitor_hash_t::value_type &monitor,
                                         void *p_void) {
  /* The monitor gets to look at each connection to see if it falls within
   * the monitor's port range of interest.  Connections of interest are first
   * looked up in the hash to see if they are already there.  If they are, we
   * reset the age of the connection so it is not deleted.  If the connection
   * is not in the hash, we add it, but only if we haven't exceeded the
   * maximum connection limit for the monitor.
   * The function takes O(1) time. */

  tcp_connection_t *p_connection;

  if (!p_void) {
    return;
  }

  /* This p_connection is on caller's stack and not the heap.
   * If we are interested, we will create a copy of the connection
   * (on the heap) and add it to our list. */
  p_connection = (tcp_connection_t *)p_void;

  /* inspect the local port number of the connection to see if we're
   * interested. */
  if ((monitor.first.first <= p_connection->local_port) &&
      (p_connection->local_port <= monitor.first.second)) {
    /* the connection is in the range of the monitor. */

    /* first check the hash to see if the connection is already there. */
    connection_hash_t::iterator i = monitor.second.hash.find(*p_connection);
    if (i != monitor.second.hash.end()) {
      /* it's already in the hash.  reset the age of the connection. */
      i->second = TCP_CONNECTION_STARTING_AGE;

      return;
    }

    /* Connection is not yet in the hash.
     * Add it if max_connections not exceeded. */
    if (monitor.second.hash.size() < monitor.second.p_peek.size()) {
      monitor.second.hash.insert(connection_hash_t::value_type(
          *p_connection, TCP_CONNECTION_STARTING_AGE));
    }
  }
}

/* ------------------------------------------------------------------------
 * Apply a tcp_port_monitor_function_ptr_t function to each port monitor in
 * the collection.
 * ------------------------------------------------------------------------ */
void for_each_tcp_port_monitor_in_collection(
    tcp_port_monitor_collection_t *p_collection,
    tcp_port_monitor_function_ptr_t p_function, void *p_function_args) {
  if (!p_collection || !p_function) {
    return;
  }

  /* for each monitor in the collection */
  for (monitor_hash_t::iterator i = p_collection->hash.begin();
       i != p_collection->hash.end(); ++i) {
    /* apply the function with the given arguments */
    p_function(*i, p_function_args);
  }
}

const unsigned char prefix_4on6[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0xff, 0xff};

union sockaddr_in46 {
  struct sockaddr_in sa4;
  struct sockaddr_in6 sa6;
  struct sockaddr sa;
};

/* checks whether the address is a IPv4-mapped IPv6 address */
bool is_4on6(const struct in6_addr *addr) {
  return !std::memcmp(&addr->s6_addr, prefix_4on6, sizeof(prefix_4on6));
}

/* converts the address to appropriate textual representation (IPv6, IPv4 or
 * fqdn) */
void print_host(char *p_buffer, size_t buffer_size, const struct in6_addr *addr,
                int fqdn) {
  union sockaddr_in46 sa;
  socklen_t slen;

  std::memset(&sa, 0, sizeof(sa));

  if (is_4on6(addr)) {
    sa.sa4.sin_family = AF_INET;
    std::memcpy(&sa.sa4.sin_addr.s_addr, &addr->s6_addr[12], 4);
    slen = sizeof(sa.sa4);
  } else {
    sa.sa6.sin6_family = AF_INET6;
    std::memcpy(&sa.sa6.sin6_addr, addr, sizeof(struct in6_addr));
    slen = sizeof(sa.sa6);
  }

  getnameinfo(&sa.sa, slen, p_buffer, buffer_size, NULL, 0,
              fqdn ? 0 : NI_NUMERICHOST);
}

/* converts the textual representation of an IPv4 or IPv6 address to struct
 * in6_addr */
void string_to_addr(struct in6_addr *addr, const char *p_buffer) {
  size_t i;

  if (std::strlen(p_buffer) < 32) {  // IPv4 address
    i = sizeof(prefix_4on6);
    std::memcpy(addr->s6_addr, prefix_4on6, i);
  } else {
    i = 0;
  }

  for (; i < sizeof(addr->s6_addr); i += 4, p_buffer += 8) {
    std::sscanf(p_buffer, "%8x", (unsigned *)&addr->s6_addr[i]);
  }
}

/* adds connections from file to the collection */
void process_file(tcp_port_monitor_collection_t *p_collection,
                  const char *file) {
  std::FILE *fp;
  char buf[256];
  char local_addr[40];
  char remote_addr[40];
  tcp_connection_t conn;
  unsigned long inode, uid, state;

  if ((fp = std::fopen(file, "r")) == NULL) {
    return;
  }

  /* ignore field name line */
  if (std::fgets(buf, 255, fp) == NULL) {
    std::fclose(fp);
    return;
  }

  /* read all tcp connections */
  while (std::fgets(buf, sizeof(buf), fp) != NULL) {
    if (std::sscanf(buf,
                    "%*d: %39[0-9a-fA-F]:%hx %39[0-9a-fA-F]:%hx %lx %*x:%*x "
                    "%*x:%*x %*x %lu %*d %lu",
                    local_addr, &conn.local_port, remote_addr,
                    &conn.remote_port, (unsigned long *)&state,
                    (unsigned long *)&uid, (unsigned long *)&inode) != 7) {
      std::fprintf(stderr, "%s: bad file format\n", file);
    }
    /** TCP_ESTABLISHED equals 1, but is not (always??) included **/
    // if ((inode == 0) || (state != TCP_ESTABLISHED)) {
    if ((inode == 0) || (state != 1)) {
      continue;
    }

    string_to_addr(&conn.local_addr, local_addr);
    string_to_addr(&conn.remote_addr, remote_addr);

    /* show the connection to each port monitor. */
    for_each_tcp_port_monitor_in_collection(
        p_collection, &show_connection_to_tcp_port_monitor, (void *)&conn);
  }

  std::fclose(fp);
}
}  // namespace

/* ----------------------------------------------------------------------
 * CLIENT INTERFACE
 *
 * Clients should call only those functions below this line.
 * ---------------------------------------------------------------------- */

/* ----------------------------------
 * Client operations on port monitors
 * ---------------------------------- */

/* Clients use this function to get connection data from the indicated
 * port monitor.
 * The requested monitor value is copied into a client-supplied char buffer.
 * Returns 0 on success, -1 otherwise. */
int peek_tcp_port_monitor(const tcp_port_monitor_t *p_monitor, int item,
                          int connection_index, char *p_buffer,
                          size_t buffer_size) {
  struct sockaddr_in sa;

  if (!p_monitor || !p_buffer || connection_index < 0) {
    return -1;
  }

  std::memset(p_buffer, 0, buffer_size);
  std::memset(&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;

  /* if the connection index is out of range, we simply return with no error,
   * having first cleared the client-supplied buffer. */
  if ((item != COUNT) &&
      (connection_index >= ssize_t(p_monitor->hash.size()))) {
    return 0;
  }

  switch (item) {
    case COUNT:

      std::snprintf(p_buffer, buffer_size, "%u",
                    unsigned(p_monitor->hash.size()));
      break;

    case REMOTEIP:

      print_host(p_buffer, buffer_size,
                 &p_monitor->p_peek[connection_index]->remote_addr, 0);
      break;

    case REMOTEHOST:

      print_host(p_buffer, buffer_size,
                 &p_monitor->p_peek[connection_index]->remote_addr, 1);
      break;

    case REMOTEPORT:

      std::snprintf(p_buffer, buffer_size, "%d",
                    p_monitor->p_peek[connection_index]->remote_port);
      break;

    case REMOTESERVICE:

      sa.sin_port = htons(p_monitor->p_peek[connection_index]->remote_port);
      getnameinfo((struct sockaddr *)&sa, sizeof(struct sockaddr_in), NULL, 0,
                  p_buffer, buffer_size, NI_NUMERICHOST);
      break;

    case LOCALIP:

      print_host(p_buffer, buffer_size,
                 &p_monitor->p_peek[connection_index]->local_addr, 0);
      break;

    case LOCALHOST:

      print_host(p_buffer, buffer_size,
                 &p_monitor->p_peek[connection_index]->local_addr, 1);
      break;

    case LOCALPORT:

      std::snprintf(p_buffer, buffer_size, "%d",
                    p_monitor->p_peek[connection_index]->local_port);
      break;

    case LOCALSERVICE:

      sa.sin_port = htons(p_monitor->p_peek[connection_index]->local_port);
      getnameinfo((struct sockaddr *)&sa, sizeof(struct sockaddr_in), NULL, 0,
                  p_buffer, buffer_size, NI_NUMERICHOST);
      break;

    default:
      return -1;
  }

  return 0;
}

/* --------------------------------
 * Client operations on collections
 * -------------------------------- */

/* Create a monitor collection.  Do this one first. */
tcp_port_monitor_collection_t *create_tcp_port_monitor_collection(void) {
  return new tcp_port_monitor_collection_t();
}

/* Destroy the monitor collection (and the monitors inside).
 * Do this one last. */
void destroy_tcp_port_monitor_collection(
    tcp_port_monitor_collection_t *p_collection) {
  delete p_collection;
}

/* Updates the tcp statistics for all monitors within a collection */
void update_tcp_port_monitor_collection(
    tcp_port_monitor_collection_t *p_collection) {
  if (!p_collection) {
    return;
  }

  process_file(p_collection, "/proc/net/tcp");
  process_file(p_collection, "/proc/net/tcp6");

  /* age the connections in all port monitors. */
  for_each_tcp_port_monitor_in_collection(p_collection, &age_tcp_port_monitor,
                                          NULL);

  /* rebuild the connection peek tables of all monitors
   * so clients can peek in O(1) time */
  for_each_tcp_port_monitor_in_collection(
      p_collection, &rebuild_tcp_port_monitor_peek_table, NULL);
}

/* Creation of reduntant monitors is silently ignored */
int insert_new_tcp_port_monitor_into_collection(
    tcp_port_monitor_collection_t *p_collection, in_port_t port_range_begin,
    in_port_t port_range_end, tcp_port_monitor_args_t *p_creation_args) {
  if (!p_collection) {
    return -1;
  }

  p_collection->hash.insert(monitor_hash_t::value_type(
      port_range_t(port_range_begin, port_range_end),
      tcp_port_monitor_t(p_creation_args->max_port_monitor_connections)));

  return 0;
}

/* Clients need a way to find monitors */
tcp_port_monitor_t *find_tcp_port_monitor(
    tcp_port_monitor_collection_t *p_collection, in_port_t port_range_begin,
    in_port_t port_range_end) {
  if (!p_collection) {
    return NULL;
  }

  /* is monitor in hash? */
  monitor_hash_t::iterator i =
      p_collection->hash.find(port_range_t(port_range_begin, port_range_end));

  return i == p_collection->hash.end() ? NULL : &i->second;
}
