/* -------------------------------------------------------------------------
 * libtcp-portmon.h:  tcp port monitoring library.               
 *
 * Copyright (C) 2005  Philip Kovacs kovacsp3@comcast.net
 *
 * $Id$
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * --------------------------------------------------------------------------- */

#ifndef LIBTCP_PORTMON_H
#define LIBTCP_PORTMON_H

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "hash.h"

/* ------------------------------------------------------------------------------------------------
 * Each port monitor contains a connection hash whose contents changes dynamically as the monitor 
 * is presented with connections on each update cycle.   This implementation maintains the health
 * of this hash by enforcing several rules.  First, the hash cannot contain more items than the
 * TCP_CONNECTION_HASH_MAX_LOAD_PCT permits.  For example, a 256 element hash with a max load of 
 * 0.5 cannot contain more than 128 connections.  Additional connections are ignored by the monitor.
 * The load factor of 0.5 is low enough to keep the hash running at near O(1) performanace at all 
 * times.  As elements are removed from the hash, the hash slots are tagged vacated, as required 
 * by open address hashing.  The vacated tags are essential as they enable the hash to find elements
 * for which there were collisions during insert (requiring additional probing for an open slot).
 * The problem with vacated slots (even though they are reused) is that, as they increase in number,
 * esp. past about 1/4 of all slots, the average number of probes the hash has to perform increases
 * from O(1) on average to O(n) worst case. To keep the hash healthy, we simply rebuild it when the
 * percentage of vacated slots gets too high (above TCP_CONNECTION_HASH_MAX_VACATED_PCT).  Rebuilding
 * the hash takes O(n) on the number of elements, but it well worth it as it keeps the hash running
 * at an average access time of O(1).
 * ------------------------------------------------------------------------------------------------*/

#define TCP_CONNECTION_HASH_SIZE 256 			/* connection hash size -- must be a power of two */
#define TCP_CONNECTION_HASH_MAX_LOAD_PCT 0.5		/* disallow inserts after this % load is exceeded */
#define TCP_CONNECIION_HASH_MAX_VACATED_PCT 0.25 	/* rebalance hash after this % of vacated slots is exceeded */ 
#define TCP_CONNECIION_STARTING_AGE 1			/* connection deleted if unseen again after this # of refreshes */

/* ----------------------------------------------------------------------------------------
 * The tcp port monitor collection also contains a hash to track the monitors it contains.
 * This hash, unlike the connection hash describes above, is not very dynamic.  Clients of
 * this library typically create a fixed number of monitors and let them run until program 
 * termination.  For this reason, I haven't included any load governors or hash rebuilding
 * steps as is done above.  You may store up to TCP_MONITOR_HASH_SIZE monitors in this hash,
 * but you _should_ remember that keeping the load low (e.g. max of 0.5) keeps the monitor
 * lookups at O(1).  
 * ----------------------------------------------------------------------------------------*/

/* TODO: Make TCP_CONNECTION_HASH_SIZE and TCP_MONITOR_HASH_SIZE variables the client can supply */

#define TCP_MONITOR_HASH_SIZE 64			/* monitor hash size -- must be a power of two */

/* -------------------------------------------------------------------
 * IMPLEMENTATION INTERFACE
 *
 * Implementation-specific interface begins here.  Clients should not 
 * manipulate these structures directly, nor call the defined helper 
 * functions.  Use the "Client interface" functions defined at bottom.
 * ------------------------------------------------------------------- */

/* The inventory of peekable items within the port monitor. */
enum tcp_port_monitor_peekables { COUNT=0, REMOTEIP, REMOTEHOST, REMOTEPORT, LOCALIP, LOCALHOST, LOCALPORT, LOCALSERVICE };

/* ------------------------------------------------------------------------
 * A single tcp connection 
 *
 * The age variable provides the mechanism for removing connections if they
 * are not seen again in subsequent update cycles.
 * ------------------------------------------------------------------------ */
typedef struct _tcp_connection_t {
        in_addr_t local_addr;
        in_port_t local_port;
        in_addr_t remote_addr;
        in_port_t remote_port;
        unsigned int uid;
        unsigned int inode;
	int age;
} tcp_connection_t;

/* ------------------------------------------------------------------------
 * A tcp connection node/list
 *
 * Connections within each monitor are stored in a double-linked list.
 * ------------------------------------------------------------------------ */
typedef struct _tcp_connection_node_t {
	tcp_connection_t connection;
	struct _tcp_connection_node_t * p_prev;
	struct _tcp_connection_node_t * p_next;
} tcp_connection_node_t;

typedef struct _tcp_connection_list_t {
	tcp_connection_node_t * p_head;
	tcp_connection_node_t * p_tail;
} tcp_connection_list_t;

/* --------------
 * A port monitor 
 * -------------- */
typedef struct _tcp_port_monitor_t {
        in_port_t port_range_begin;
        in_port_t port_range_end;   		/* begin = end to monitor a single port */
	tcp_connection_list_t connection_list;	/* list of connections for this monitor */
	hash_table_t hash;                      /* hash table contains pointers into monitor's connection list */
	tcp_connection_t **p_peek;		/* array of connection pointers for O(1) peeking by index */ 
} tcp_port_monitor_t;

/* -----------------------------------------------------------------------------
 * Open-addressed hash implementation requires that we supply two hash functions
 * and a match function to compare two hash elements for identity.
 * ----------------------------------------------------------------------------- */

/* --------------------------------------------------
 * Functions to hash the connections within a monitor
 * --------------------------------------------------*/

/* First connection hash function */
int connection_hash_function_1( const void * /* p_data */ );

/* Second connection hash function */
int connection_hash_function_2( const void * /* p_data */ );

/* Connection match function returns non-zero if hash elements are identical. */
int connection_match_function( const void * /* p_data1 */, const void * /* p_data2 */ );

/* --------------------------------------------------
 * Functions to hash the monitors within a collection
 * --------------------------------------------------*/

/* First monitor hash function */
int monitor_hash_function_1( const void * /* p_data */ );

/* Second monitor hash function */
int monitor_hash_function_2( const void * /* p_data */ );

/* Monitor match function returns non-zero if hash elements are identical. */
int monitor_match_function( const void * /* p_data1 */, const void * /* p_data2 */ );

/* ------------------------
 * A port monitor node/list 
 * ------------------------ */
typedef struct _tcp_port_monitor_node_t {
        tcp_port_monitor_t * p_monitor;
        struct _tcp_port_monitor_node_t *p_next;
} tcp_port_monitor_node_t;

typedef struct __tcp_port_monitor_list_t {
	tcp_port_monitor_node_t * p_head;
	tcp_port_monitor_node_t * p_tail;
} tcp_port_monitor_list_t;

/* ---------------------------------------
 * A port monitor utility function typedef
 * ---------------------------------------*/ 
typedef void (*tcp_port_monitor_function_ptr_t)( tcp_port_monitor_t * /* p_monitor */, void * /* p_void */ );

/* ---------------------------------------------------------------------------
 * Port monitor utility functions implementing tcp_port_monitor_function_ptr_t
 * ---------------------------------------------------------------------------*/
void destroy_tcp_port_monitor(
        tcp_port_monitor_t *                    /* p_monitor */,
	void *					/* p_void (use NULL for this function) */
	);

void age_tcp_port_monitor(
	tcp_port_monitor_t *                    /* p_monitor */,
	void *					/* p_void (use NULL for this function) */
	);

void maintain_tcp_port_monitor_hash(
	tcp_port_monitor_t *                    /* p_monitor */,
	void *                                  /* p_void (use NULL for this function) */
	);

void rebuild_tcp_port_monitor_peek_table(
	tcp_port_monitor_t * 			/* p_monitor */,
	void *					/* p_void (use NULL for this function) */
	);

void show_connection_to_tcp_port_monitor(
	tcp_port_monitor_t *                    /* p_monitor */,
	void *					/* p_connection (client should cast) */
	);

/* -----------------------------
 * A tcp port monitor collection
 * -----------------------------*/
typedef struct _tcp_port_monitor_collection_t {
	tcp_port_monitor_list_t monitor_list;	/* list of monitors for this collection */
	hash_table_t hash;			/* hash table contains pointers into collection's monitor list */
} tcp_port_monitor_collection_t;

/* ---------------------------------------------------------------------------------------
 * Apply a tcp_port_monitor_function_ptr_t function to each port monitor in the collection. 
 * ---------------------------------------------------------------------------------------*/
void for_each_tcp_port_monitor_in_collection(
 	tcp_port_monitor_collection_t *         /* p_collection */,
  	tcp_port_monitor_function_ptr_t		/* p_function */,
	void *					/* p_function_args (for user arguments) */
	);


/* ----------------------------------------------------------------------
 * CLIENT INTERFACE 
 *
 * Clients should call only those functions below this line.
 * ---------------------------------------------------------------------- */

/* ----------------------------------
 * Client operations on port monitors
 * ---------------------------------- */

/* Clients should first try to "find_tcp_port_monitor" before creating one
   so that there are no redundant monitors. */
tcp_port_monitor_t * create_tcp_port_monitor(
	in_port_t 				/* port_range_begin */, 
	in_port_t 				/* port_range_end */ 
	);

/* Clients use this function to get connection data from the indicated port monitor.
   The requested monitor value is copied into a client-supplied char buffer. 
   Returns 0 on success, -1 otherwise. */
int peek_tcp_port_monitor(
	tcp_port_monitor_t * 			/* p_monitor */,
	int					/* item, ( item of interest, from tcp_port_monitor_peekables enum ) */,
	int					/* connection_index, ( 0 to number of connections in monitor - 1 )*/,
	char *					/* p_buffer, buffer to receive requested value */,
	size_t					/* buffer_size, size of p_buffer */
	);

/* --------------------------------
 * Client operations on collections
 * -------------------------------- */

/* Create a monitor collection.  Do this one first. */
tcp_port_monitor_collection_t * create_tcp_port_monitor_collection( void );

/* Destroy the monitor collection (and everything it contains).  Do this one last. */
void destroy_tcp_port_monitor_collection( 
	tcp_port_monitor_collection_t * 	/* p_collection */ 
	);

/* Updates the tcp statitics for all monitors within a collection */
void update_tcp_port_monitor_collection(
	tcp_port_monitor_collection_t *         /* p_collection */
	);

/* After clients create a monitor, use this to add it to the collection. 
   Returns 0 on success, -1 otherwise. */
int insert_tcp_port_monitor_into_collection( 
	tcp_port_monitor_collection_t * 	/* p_collection */, 
	tcp_port_monitor_t * 			/* p_monitor */ 
	);

/* Clients need a way to find monitors */
tcp_port_monitor_t * find_tcp_port_monitor( 
	tcp_port_monitor_collection_t * 	/* p_collection */, 
	in_port_t 				/* port_range_begin */, 
	in_port_t 				/* port_range_end */ 
	);

#endif
