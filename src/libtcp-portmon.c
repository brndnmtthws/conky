/* $Id$ */

/* libtcp-portmon.c:  tcp port monitoring library.
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

#include <glib/gprintf.h>
#include "libtcp-portmon.h"

/* -------------------------------------------------------------------
 * IMPLEMENTATION INTERFACE
 *
 * Implementation-specific interface begins here.  Clients should not
 * manipulate these structures directly, nor call the defined helper
 * functions.  Use the "Client interface" functions defined at bottom.
 * ------------------------------------------------------------------- */

/* -----------------------------------
 * Copy a tcp_connection_t
 *
 * Returns 0 on success, -1 otherwise.
 * ----------------------------------- */
int copy_tcp_connection(tcp_connection_t *p_dest_connection,
		const tcp_connection_t *p_source_connection)
{
	if (!p_dest_connection || !p_source_connection) {
		return -1;
	}

	g_strlcpy(p_dest_connection->key, p_source_connection->key,
		sizeof(p_dest_connection->key));
	p_dest_connection->local_addr = p_source_connection->local_addr;
	p_dest_connection->local_port = p_source_connection->local_port;
	p_dest_connection->remote_addr = p_source_connection->remote_addr;
	p_dest_connection->remote_port = p_source_connection->remote_port;
	p_dest_connection->age = p_source_connection->age;

	return 0;
}

/* -------------------------------------------
 * Port monitor utility functions implementing
 * tcp_port_monitor_function_ptr_t
 * ------------------------------------------- */
void destroy_tcp_port_monitor(tcp_port_monitor_t *p_monitor, void *p_void)
{
	tcp_connection_node_t *p_node, *p_temp;

	if (!p_monitor || p_void) {	/* p_void should be NULL in this context */
		return;
	}

	/* destroy the monitor's peek array */
	free(p_monitor->p_peek);

	/* destroy the monitor's connection list */
	for (p_node = p_monitor->connection_list.p_head; p_node != NULL; ) {
		/* p_temp is for the next iteration */
		p_temp = p_node->p_next;

		free(p_node);

		p_node = p_temp;
	}

	/* destroy the monitor's hash */
	g_hash_table_destroy(p_monitor->hash);
	p_monitor->hash = NULL;

	/* destroy the monitor */
	free(p_monitor);
	p_monitor = NULL;
}

void age_tcp_port_monitor(tcp_port_monitor_t *p_monitor, void *p_void)
{
	/* Run through the monitor's connections and decrement the age variable.
	 * If the age goes negative, we remove the connection from the monitor.
	 * Function takes O(n) time on the number of connections. */

	tcp_connection_node_t *p_node, *p_temp;
	tcp_connection_t *p_conn;

	if (!p_monitor || p_void) {	/* p_void should be NULL in this context */
		return;
	}

	if (!p_monitor->p_peek) {
		return;
	}

	for (p_node = p_monitor->connection_list.p_head; p_node; ) {
		if (--p_node->connection.age >= 0) {
			p_node = p_node->p_next;
			continue;
		}

		/* connection on p_node is old.  remove connection from the hash. */
		p_conn = &p_node->connection;
#ifdef HASH_DEBUG
		fprintf(stderr, "monitor hash removal of connection [%s]", p_conn->key);
		if (!g_hash_table_remove(p_monitor->hash,
				(gconstpointer) p_conn->key)) {
			fprintf(stderr, " - ERROR NOT FOUND\n");
			return;
		}
		fprintf(stderr, " - OK\n");
#else
		if (!g_hash_table_remove(p_monitor->hash,
				(gconstpointer) p_conn->key)) {
			return;
		}
#endif

		/* splice p_node out of the connection_list */
		if (p_node->p_prev != NULL) {
			p_node->p_prev->p_next = p_node->p_next;
		}
		if (p_node->p_next != NULL) {
			p_node->p_next->p_prev = p_node->p_prev;
		}

		/* correct the list head and tail if necessary */
		if (p_monitor->connection_list.p_head == p_node) {
			p_monitor->connection_list.p_head = p_node->p_next;
		}
		if (p_monitor->connection_list.p_tail == p_node) {
			p_monitor->connection_list.p_tail = p_node->p_prev;
		}

		/* p_temp is for the next iteration */
		p_temp = p_node->p_next;

		/* destroy the node */
		free(p_node);

		p_node = p_temp;
	}
}

void rebuild_tcp_port_monitor_peek_table(tcp_port_monitor_t *p_monitor,
		void *p_void)
{
	/* Run through the monitor's connections and rebuild the peek table of
	 * connection pointers.  This is done so peeking into the monitor can be
	 * done in O(1) time instead of O(n) time for each peek. */

	tcp_connection_node_t *p_node;
	int i = 0;

	if (!p_monitor || p_void) {	/* p_void should be NULL in this context */
		return;
	}

	/* zero out the peek array */
	memset(p_monitor->p_peek, 0, p_monitor->max_port_monitor_connections *
		sizeof(tcp_connection_t *));

	for (p_node = p_monitor->connection_list.p_head; p_node != NULL;
			p_node = p_node->p_next, i++) {
		p_monitor->p_peek[i] = &p_node->connection;
	}
}

void show_connection_to_tcp_port_monitor(tcp_port_monitor_t *p_monitor,
		void *p_void)
{
	/* The monitor gets to look at each connection to see if it falls within
	 * the monitor's port range of interest.  Connections of interest are first
	 * looked up in the hash to see if they are already there.  If they are, we
	 * reset the age of the connection so it is not deleted.  If the connection
	 * is not in the hash, we add it, but only if we haven't exceeded the
	 * maximum connection limit for the monitor.
	 * The function takes O(1) time. */

	tcp_connection_node_t *p_node;
	tcp_connection_t *p_connection, *p_conn_hash;

	if (!p_monitor || !p_void) {
		return;
	}

	/* This p_connection is on caller's stack and not the heap.
	 * If we are interested, we will create a copy of the connection
	 * (on the heap) and add it to our list. */
	p_connection = (tcp_connection_t *) p_void;

	/* inspect the local port number of the connection to see if we're
	 * interested. */
	if ((p_monitor->port_range_begin <= p_connection->local_port)
			&& (p_connection->local_port <= p_monitor->port_range_end)) {
		/* the connection is in the range of the monitor. */

		/* first check the hash to see if the connection is already there. */
		if ((p_conn_hash = g_hash_table_lookup(p_monitor->hash,
				(gconstpointer) p_connection->key))) {
			/* it's already in the hash.  reset the age of the connection. */
			p_conn_hash->age = TCP_CONNECTION_STARTING_AGE;

			return;
		}

		/* Connection is not yet in the hash.
		 * Add it if max_connections not exceeded. */
		if (g_hash_table_size(p_monitor->hash)
				>= p_monitor->max_port_monitor_connections) {
			return;
		}

		/* create a new connection node */
		if ((p_node = (tcp_connection_node_t *)
				calloc(1, sizeof(tcp_connection_node_t))) == NULL) {
			return;
		}

		/* copy the connection data */
		if (copy_tcp_connection(&p_node->connection, p_connection) != 0) {
			/* error copying the connection data. deallocate p_node to
			 * avoid leaks and return. */
			free(p_node);
			return;
		}

		p_node->connection.age = TCP_CONNECTION_STARTING_AGE;
		p_node->p_next = NULL;

		/* insert it into the monitor's hash table */
#ifdef HASH_DEBUG
		fprintf(stderr, "monitor hash insert of connection [%s]\n",
			p_node->connection.key);
#endif
		g_hash_table_insert(p_monitor->hash,
			(gpointer) p_node->connection.key, (gpointer) &p_node->connection);

		/* append the node to the monitor's connection list */
		if (p_monitor->connection_list.p_tail == NULL) {
			/* assume p_head is NULL too */
			p_monitor->connection_list.p_head = p_node;
			p_monitor->connection_list.p_tail = p_node;
			p_node->p_prev = NULL;
		} else {
			p_monitor->connection_list.p_tail->p_next = p_node;
			p_node->p_prev = p_monitor->connection_list.p_tail;
			p_monitor->connection_list.p_tail = p_node;
		}
	}
}

/* ------------------------------------------------------------------------
 * Apply a tcp_port_monitor_function_ptr_t function to each port monitor in
 * the collection.
 * ------------------------------------------------------------------------ */
void for_each_tcp_port_monitor_in_collection(
		tcp_port_monitor_collection_t *p_collection,
		tcp_port_monitor_function_ptr_t p_function, void *p_function_args)
{
	tcp_port_monitor_node_t *p_current_node, *p_next_node;

	if (!p_collection || !p_function) {
		return;
	}

	/* for each monitor in the collection */
	for (p_current_node = p_collection->monitor_list.p_head;
			p_current_node != NULL; p_current_node = p_next_node) {
		p_next_node = p_current_node->p_next;	/* do this first! */

		if (p_current_node->p_monitor) {
			/* apply the function with the given arguments */
			p_function(p_current_node->p_monitor, p_function_args);
		}
	}
}

/* ----------------------------------------------------------------------
 * CLIENT INTERFACE
 *
 * Clients should call only those functions below this line.
 * ---------------------------------------------------------------------- */

/* ----------------------------------
 * Client operations on port monitors
 * ---------------------------------- */

/* Clients should first try to "find_tcp_port_monitor" before creating one
 * so that there are no redundant monitors. */
tcp_port_monitor_t *create_tcp_port_monitor(in_port_t port_range_begin,
		in_port_t port_range_end, tcp_port_monitor_args_t *p_creation_args)
{
	tcp_port_monitor_t *p_monitor;

	/* create the monitor */
	p_monitor = (tcp_port_monitor_t *) calloc(1, sizeof(tcp_port_monitor_t));
	if (!p_monitor) {
		return NULL;
	}

	p_monitor->max_port_monitor_connections =
		p_creation_args->max_port_monitor_connections;

	/* build the monitor key for the collection hash */
	g_sprintf(p_monitor->key, ":%04X :%04X", port_range_begin, port_range_end);

	/* create the monitor's connection hash */
	if ((p_monitor->hash = g_hash_table_new(g_str_hash, g_str_equal)) == NULL) {
		/* we failed to create the hash, so destroy the monitor completely
		 * so we don't leak */
		destroy_tcp_port_monitor(p_monitor, NULL);
		return NULL;
	}

	/* create the monitor's peek array */
	if ((p_monitor->p_peek = (tcp_connection_t **)
			calloc(p_monitor->max_port_monitor_connections,
			sizeof(tcp_connection_t *))) == NULL) {
		/* we failed to create the peek array,
		 * so destroy the monitor completely, again, so we don't leak */
		destroy_tcp_port_monitor(p_monitor, NULL);
		return NULL;
	}

	p_monitor->port_range_begin = port_range_begin;
	p_monitor->port_range_end = port_range_end;

	p_monitor->connection_list.p_head = NULL;
	p_monitor->connection_list.p_tail = NULL;

	return p_monitor;
}

/* Clients use this function to get connection data from the indicated
 * port monitor.
 * The requested monitor value is copied into a client-supplied char buffer.
 * Returns 0 on success, -1 otherwise. */
int peek_tcp_port_monitor(const tcp_port_monitor_t *p_monitor, int item,
		int connection_index, char *p_buffer, size_t buffer_size)
{
	struct hostent *p_hostent;
	struct servent *p_servent;
	struct in_addr net;

	if (!p_monitor || !p_buffer || connection_index < 0) {
		return -1;
	}

	memset(p_buffer, 0, buffer_size);
	memset(&net, 0, sizeof(net));

	/* if the connection index is out of range, we simply return with no error,
	 * having first cleared the client-supplied buffer. */
	if ((item != COUNT) && (connection_index
			> (int) g_hash_table_size(p_monitor->hash) - 1)) {
		return 0;
	}

	switch (item) {

		case COUNT:

			snprintf(p_buffer, buffer_size, "%d",
				g_hash_table_size(p_monitor->hash));
			break;

		case REMOTEIP:

			net.s_addr = p_monitor->p_peek[connection_index]->remote_addr;
			snprintf(p_buffer, buffer_size, "%s", inet_ntoa(net));
			break;

		case REMOTEHOST:

			p_hostent = gethostbyaddr((const void *)
				&p_monitor->p_peek[connection_index]->remote_addr,
				sizeof(in_addr_t), AF_INET);
			/* if no host name found, just use ip address. */
			if (!p_hostent || !p_hostent->h_name) {
				net.s_addr = p_monitor->p_peek[connection_index]->remote_addr;
				snprintf(p_buffer, buffer_size, "%s", inet_ntoa(net));
				break;
			}
			snprintf(p_buffer, buffer_size, "%s", p_hostent->h_name);
			break;

		case REMOTEPORT:

			snprintf(p_buffer, buffer_size, "%d",
				p_monitor->p_peek[connection_index]->remote_port);
			break;

		case REMOTESERVICE:

			p_servent = getservbyport(
				htons(p_monitor->p_peek[connection_index]->remote_port), "tcp");
			/* if no service name found for the port,
			 * just use the port number. */
			if (!p_servent || !p_servent->s_name) {
				snprintf(p_buffer, buffer_size, "%d",
					p_monitor->p_peek[connection_index]->remote_port);
			} else {
				snprintf(p_buffer, buffer_size, "%s", p_servent->s_name);
			}
			break;

		case LOCALIP:

			net.s_addr = p_monitor->p_peek[connection_index]->local_addr;
			snprintf(p_buffer, buffer_size, "%s", inet_ntoa(net));
			break;

		case LOCALHOST:

			p_hostent = gethostbyaddr((const void *)
				&p_monitor->p_peek[connection_index]->local_addr,
				sizeof(in_addr_t), AF_INET);
			/* if no host name found, just use ip address. */
			if (!p_hostent || !p_hostent->h_name) {
				net.s_addr = p_monitor->p_peek[connection_index]->local_addr;
				snprintf(p_buffer, buffer_size, "%s", inet_ntoa(net));
				break;
			}
			snprintf(p_buffer, buffer_size, "%s", p_hostent->h_name);
			break;

		case LOCALPORT:

			snprintf(p_buffer, buffer_size, "%d",
				p_monitor->p_peek[connection_index]->local_port);
			break;

		case LOCALSERVICE:

			p_servent = getservbyport(
				htons(p_monitor->p_peek[connection_index]->local_port), "tcp");
			/* if no service name found for the port,
			 * just use the port number. */
			if (!p_servent || !p_servent->s_name) {
				snprintf(p_buffer, buffer_size, "%d",
					p_monitor->p_peek[connection_index]->local_port);
				break;
			}
			snprintf(p_buffer, buffer_size, "%s", p_servent->s_name);
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
tcp_port_monitor_collection_t *create_tcp_port_monitor_collection(void)
{
	tcp_port_monitor_collection_t *p_collection;

	p_collection = (tcp_port_monitor_collection_t *)
		calloc(1, sizeof(tcp_port_monitor_collection_t));
	if (!p_collection) {
		return NULL;
	}

	/* create the collection's monitor hash */
	if ((p_collection->hash = g_hash_table_new(g_str_hash, g_str_equal))
			== NULL) {
		/* we failed to create the hash,
		 * so destroy the monitor completely so we don't leak */
		destroy_tcp_port_monitor_collection(p_collection);
		return NULL;
	}

	p_collection->monitor_list.p_head = NULL;
	p_collection->monitor_list.p_tail = NULL;

	return p_collection;
}

/* Destroy the monitor collection (and the monitors inside).
 * Do this one last. */
void destroy_tcp_port_monitor_collection(
		tcp_port_monitor_collection_t *p_collection)
{
	tcp_port_monitor_node_t *p_current_node, *p_next_node;

	if (!p_collection) {
		return;
	}

	/* destroy the monitors */
	for_each_tcp_port_monitor_in_collection(p_collection,
		&destroy_tcp_port_monitor, NULL);

	/* next destroy the empty monitor nodes */
	for (p_current_node = p_collection->monitor_list.p_head;
			p_current_node != NULL; p_current_node = p_next_node) {
		p_next_node = p_current_node->p_next;	/* do this first! */

		free(p_current_node);
	}

	/* destroy the collection's hash */
	g_hash_table_destroy(p_collection->hash);
	p_collection->hash = NULL;

	free(p_collection);
	p_collection = NULL;
}

/* Updates the tcp statistics for all monitors within a collection */
void update_tcp_port_monitor_collection(
		tcp_port_monitor_collection_t *p_collection)
{
	FILE *fp;
	char buf[256];
	tcp_connection_t conn;
	unsigned long inode, uid, state;

	if (!p_collection) {
		return;
	}

	/* age the connections in all port monitors. */
	for_each_tcp_port_monitor_in_collection(p_collection,
		&age_tcp_port_monitor, NULL);

	/* read tcp data from /proc/net/tcp */
	if ((fp = fopen("/proc/net/tcp", "r")) == NULL) {
		return;
	}

	/* ignore field name line */
	fgets(buf, 255, fp);

	/* read all tcp connections */
	while (fgets(buf, sizeof(buf), fp) != NULL) {

		if (sscanf(buf,
				"%*d: %x:%hx %x:%hx %lx %*x:%*x %*x:%*x %*x %lu %*d %lu",
				(unsigned int *) &conn.local_addr, &conn.local_port,
				(unsigned int *) &conn.remote_addr, &conn.remote_port,
				(unsigned long *) &state, (unsigned long *) &uid,
				(unsigned long *) &inode) != 7) {
			fprintf(stderr, "/proc/net/tcp: bad file format\n");
		}
		/** TCP_ESTABLISHED equals 1, but is not (always??) included **/
		//if ((inode == 0) || (state != TCP_ESTABLISHED)) {
		if((inode == 0) || (state != 1)) {
			continue;
		}

		/* build hash key */
		g_sprintf(conn.key, "%08X:%04X %08X:%04X", conn.local_addr,
			conn.local_port, conn.remote_addr, conn.remote_port);

		/* show the connection to each port monitor. */
		for_each_tcp_port_monitor_in_collection(p_collection,
			&show_connection_to_tcp_port_monitor, (void *) &conn);
	}

	fclose(fp);

	/* rebuild the connection peek tables of all monitors
	 * so clients can peek in O(1) time */
	for_each_tcp_port_monitor_in_collection(p_collection,
		&rebuild_tcp_port_monitor_peek_table, NULL);
}

/* After clients create a monitor, use this to add it to the collection.
 * Returns 0 on success, -1 otherwise. */
int insert_tcp_port_monitor_into_collection(
		tcp_port_monitor_collection_t *p_collection,
		tcp_port_monitor_t *p_monitor)
{
	tcp_port_monitor_node_t *p_node;

	if (!p_collection || !p_monitor) {
		return -1;
	}

	/* create a container node for this monitor */
	p_node = (tcp_port_monitor_node_t *)
		calloc(1, sizeof(tcp_port_monitor_node_t));
	if (!p_node) {
		return -1;
	}

	/* populate the node */
	p_node->p_monitor = p_monitor;
	p_node->p_next = NULL;

	/* add a pointer to this monitor to the collection's hash */
#ifdef HASH_DEBUG
	fprintf(stderr, "collection hash insert of monitor [%s]\n", p_monitor->key);
#endif
	g_hash_table_insert(p_collection->hash, (gpointer) p_monitor->key,
		(gpointer) p_monitor);

	/* tail of the container gets this node */
	if (!p_collection->monitor_list.p_tail) {
		p_collection->monitor_list.p_tail = p_node;
	} else {
		/* p_next of the tail better be NULL */
		if (p_collection->monitor_list.p_tail->p_next != NULL) {
			return -1;
		}

		/* splice node onto tail */
		p_collection->monitor_list.p_tail->p_next = p_node;
		p_collection->monitor_list.p_tail = p_node;
	}

	/* if this was the first element added */
	if (!p_collection->monitor_list.p_head) {
		p_collection->monitor_list.p_head = p_collection->monitor_list.p_tail;
	}

	return 0;
}

/* Clients need a way to find monitors */
tcp_port_monitor_t *find_tcp_port_monitor(
		const tcp_port_monitor_collection_t *p_collection,
		in_port_t port_range_begin, in_port_t port_range_end)
{
	tcp_port_monitor_t *p_monitor;
	gchar key[12];

	if (!p_collection) {
		return NULL;
	}

	/* is monitor in hash? */
	g_sprintf(key, ":%04X :%04X", port_range_begin, port_range_end);
	p_monitor = g_hash_table_lookup(p_collection->hash, (gconstpointer) key);
	return p_monitor;
}
