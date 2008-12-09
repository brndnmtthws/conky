/* libtcp-portmon.h:  tcp port monitoring library.
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

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <math.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

/* connection deleted if unseen again after this # of refreshes */
#define TCP_CONNECTION_STARTING_AGE 1
#define TCP_CONNECTION_HASH_KEY_SIZE 28
#define TCP_PORT_MONITOR_HASH_KEY_SIZE 12
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

/* ------------------------------------------------------------------------
 * A single tcp connection
 *
 * The age variable provides the mechanism for removing connections if they
 * are not seen again in subsequent update cycles.
 * ------------------------------------------------------------------------ */
typedef struct _tcp_connection_t {
	/* connection's key in monitor hash */
	gchar key[TCP_CONNECTION_HASH_KEY_SIZE];
	in_addr_t local_addr;
	in_port_t local_port;
	in_addr_t remote_addr;
	in_port_t remote_port;
	int age;
} tcp_connection_t;

/* ----------------------------------
 * Copy a connection
 *
 * Returns 0 on success, -1 otherwise
 * ---------------------------------- */
int copy_tcp_connection(tcp_connection_t *p_dest_connection,
	const tcp_connection_t *p_source_connection);

/* -------------------------------------------------------------------
 * A tcp connection node/list
 *
 * Connections within each monitor are stored in a double-linked list.
 * ------------------------------------------------------------------- */
typedef struct _tcp_connection_node_t {
	tcp_connection_t connection;
	struct _tcp_connection_node_t *p_prev;
	struct _tcp_connection_node_t *p_next;
} tcp_connection_node_t;

typedef struct _tcp_connection_list_t {
	tcp_connection_node_t *p_head;
	tcp_connection_node_t *p_tail;
} tcp_connection_list_t;

/* --------------
 * A port monitor
 * -------------- */
typedef struct _tcp_port_monitor_t {
	/* monitor's key in collection hash */
	gchar key[TCP_PORT_MONITOR_HASH_KEY_SIZE];
	/* start of monitor port range */
	in_port_t port_range_begin;
	/* begin = end to monitor a single port */
	in_port_t port_range_end;
	/* list of connections for this monitor */
	tcp_connection_list_t connection_list;
	/* hash table of pointers into connection list */
	GHashTable *hash;
	/* array of connection pointers for O(1) peeking */
	tcp_connection_t **p_peek;
	/* max number of connections */
	unsigned int max_port_monitor_connections;
} tcp_port_monitor_t;

/* ------------------------
 * A port monitor node/list
 * ------------------------ */
typedef struct _tcp_port_monitor_node_t {
	tcp_port_monitor_t *p_monitor;
	struct _tcp_port_monitor_node_t *p_next;
} tcp_port_monitor_node_t;

typedef struct __tcp_port_monitor_list_t {
	tcp_port_monitor_node_t *p_head;
	tcp_port_monitor_node_t *p_tail;
} tcp_port_monitor_list_t;

/* ---------------------------------------
 * A port monitor utility function typedef
 * --------------------------------------- */
typedef void (*tcp_port_monitor_function_ptr_t)(tcp_port_monitor_t *p_monitor,
	void *p_void);

/* -------------------------------------------
 * Port monitor utility functions implementing
 * tcp_port_monitor_function_ptr_t
 * ------------------------------------------- */
void destroy_tcp_port_monitor(tcp_port_monitor_t *p_monitor,
	void *p_void /* (use NULL for this function) */);

void age_tcp_port_monitor(tcp_port_monitor_t *p_monitor,
	void *p_void /* (use NULL for this function) */);

void rebuild_tcp_port_monitor_peek_table(tcp_port_monitor_t *p_monitor,
	void *p_void /* (use NULL for this function) */);

void show_connection_to_tcp_port_monitor(tcp_port_monitor_t *p_monitor,
	void *p_connection /* (client should cast) */);

/* -----------------------------
 * A tcp port monitor collection
 * ----------------------------- */
typedef struct _tcp_port_monitor_collection_t {
	/* list of monitors for this collection */
	tcp_port_monitor_list_t monitor_list;
	/* hash table of pointers into collection's monitor list */
	GHashTable *hash;
} tcp_port_monitor_collection_t;

/* --------------------------------------------------------
 * Apply a tcp_port_monitor_function_ptr_t function to each
 * port monitor in the collection.
 * -------------------------------------------------------- */
void for_each_tcp_port_monitor_in_collection(
	tcp_port_monitor_collection_t *p_collection,
	tcp_port_monitor_function_ptr_t p_function,
	void *p_function_args /* (for user arguments) */);

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

/* Clients should first try to "find_tcp_port_monitor" before creating one,
 * so that there are no redundant monitors. */
tcp_port_monitor_t *create_tcp_port_monitor(in_port_t port_range_begin,
	in_port_t port_range_end, tcp_port_monitor_args_t *p_creation_args);

/* Clients use this function to get connection data from
 * the indicated port monitor.
 * The requested monitor value is copied into a client-supplied char buffer.
 * Returns 0 on success, -1 otherwise. */
int peek_tcp_port_monitor(const tcp_port_monitor_t *p_monitor,
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

/* Updates the tcp statitics for all monitors within a collection */
void update_tcp_port_monitor_collection(
	tcp_port_monitor_collection_t *p_collection);

/* After clients create a monitor, use this to add it to the collection.
 * Returns 0 on success, -1 otherwise. */
int insert_tcp_port_monitor_into_collection(
	tcp_port_monitor_collection_t *p_collection, tcp_port_monitor_t *p_monitor);

/* Clients need a way to find monitors */
tcp_port_monitor_t *find_tcp_port_monitor(
	const tcp_port_monitor_collection_t *p_collection,
	in_port_t port_range_begin, in_port_t port_range_end);

#endif
