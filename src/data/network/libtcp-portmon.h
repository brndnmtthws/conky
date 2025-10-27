/*
 *
 * libtcp-portmon.h:  tcp port monitoring library.
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
 * USA. */

#ifndef LIBTCP_PORTMON_H
#define LIBTCP_PORTMON_H

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <netdb.h>

/* connection deleted if unseen again after this # of refreshes */
#define TCP_CONNECTION_STARTING_AGE 1
#define BUILD_PORT_MONITORS_HASH_KEY_SIZE 12
#define MAX_PORT_MONITOR_CONNECTIONS_DEFAULT 256

/* -------------------------------------------------------------------
 * IMPLEMENTATION INTERFACE
 *
 * Implementation-specific interface begins here.  Clients should not
 * manipulate these structures directly, nor call the defined helper
 * functions.  Use the "Client interface" functions defined at bottom.
 * ------------------------------------------------------------------- */

/* The inventory of peekable items within the port monitor. */
enum tcp_port_monitor_peekables {
  COUNT = 0,
  REMOTEIP,
  REMOTEHOST,
  REMOTEPORT,
  REMOTESERVICE,
  LOCALIP,
  LOCALHOST,
  LOCALPORT,
  LOCALSERVICE
};

/* ------------------------------------------------------------
 * A port monitor
 *
 * The definition of the struct is hidden because it contains
 * C++-specific stuff and we want to #include this from C code.
 * ------------------------------------------------------------ */
typedef struct _tcp_port_monitor_t tcp_port_monitor_t;

/* ------------------------------------------------------------
 * A tcp port monitor collection
 *
 * The definition of the struct is hidden because it contains
 * C++-specific stuff and we want to #include this from C code.
 * ------------------------------------------------------------ */
typedef struct _tcp_port_monitor_collection_t tcp_port_monitor_collection_t;

/* ----------------------------------------------------------------------
 * CLIENT INTERFACE
 *
 * Clients should call only those functions below this line.
 * ---------------------------------------------------------------------- */

/* struct to hold monitor creation arguments */
typedef struct _tcp_port_monitor_args_t {
  /* monitor supports tracking at most this many connections */
  int max_port_monitor_connections;
} tcp_port_monitor_args_t;

/* ----------------------------------
 * Client operations on port monitors
 * ---------------------------------- */

/* Clients use this function to get connection data from
 * the indicated port monitor.
 * The requested monitor value is copied into a client-supplied char buffer.
 * Returns 0 on success, -1 otherwise. */
int peek_tcp_port_monitor(
    const tcp_port_monitor_t *p_monitor,
    /* (item of interest, from tcp_port_monitor_peekables enum) */
    int item,
    /* (0 to number of connections in monitor - 1) */
    int connection_index,
    /* buffer to receive requested value */
    char *p_buffer,
    /* size of p_buffer */
    size_t buffer_size);

/* --------------------------------
 * Client operations on collections
 * -------------------------------- */

/* Create a monitor collection.  Do this one first. */
tcp_port_monitor_collection_t *create_tcp_port_monitor_collection(void);

/* Destroy the monitor collection (and everything it contains).
 * Do this one last. */
void destroy_tcp_port_monitor_collection(
    tcp_port_monitor_collection_t *p_collection);

/* Updates the tcp statistics for all monitors within a collection */
void update_tcp_port_monitor_collection(
    tcp_port_monitor_collection_t *p_collection);

/* Creation of redundant monitors is silently ignored
 * Returns 0 on success, -1 otherwise. */
int insert_new_tcp_port_monitor_into_collection(
    tcp_port_monitor_collection_t *p_collection, in_port_t port_range_begin,
    in_port_t port_range_end, tcp_port_monitor_args_t *p_creation_args);

/* Clients need a way to find monitors */
tcp_port_monitor_t *find_tcp_port_monitor(
    tcp_port_monitor_collection_t *p_collection, in_port_t port_range_begin,
    in_port_t port_range_end);

#endif
