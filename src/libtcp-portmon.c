/* -------------------------------------------------------------------------
 * libtcp-portmon.c:  tcp port monitoring library.               
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

#include "libtcp-portmon.h"

/* -------------------------------------------------------------------
 * IMPLEMENTATION INTERFACE
 *
 * Implementation-specific interface begins here.  Clients should not
 * manipulate these structures directly, nor call the defined helper
 * functions.  Use the "Client interface" functions defined at bottom.
 * ------------------------------------------------------------------- */

/* ----------------------------------
 * Copy a tcp_connection_t
 *
 * Returns 0 on success, -1 otherwise.
 * ----------------------------------*/
int copy_tcp_connection( 
	tcp_connection_t *			p_dest_connection,
	const tcp_connection_t *		p_source_connection
	)
{
   if ( !p_dest_connection || !p_source_connection )
	return (-1);

   p_dest_connection->local_addr = p_source_connection->local_addr;
   p_dest_connection->local_port = p_source_connection->local_port;
   p_dest_connection->remote_addr = p_source_connection->remote_addr;
   p_dest_connection->remote_port = p_source_connection->remote_port;
   p_dest_connection->age = p_source_connection->age;

   return 0;
}

/* -----------------------------------------------------------------------------
 * Open-addressed hash implementation requires that we supply two hash functions
 * and a match function to compare two hash elements for identity.
 * ----------------------------------------------------------------------------- */

/* --------------------------------------------------
 * Functions to hash the connections within a monitor
 * --------------------------------------------------*/

#define CONNECTION_HASH_KEY_LEN 17

/* ----------------------------------------------------------------------------------
 * First connection hash function: DJB with a 65521 prime modulus to govern the range. 
 * ----------------------------------------------------------------------------------*/
int connection_hash_function_1( const void *p_data )
{
   tcp_connection_t *p_conn;
   char key[CONNECTION_HASH_KEY_LEN];
   unsigned int hash = 5381;
   unsigned int i    = 0;

   if ( !p_data )
	   return -1;
   
   memset(key,0,sizeof(key));

   /* p_data is a pointer to tcp_connection_t */
   p_conn = (tcp_connection_t *)p_data; 

   /* key is a hex representation of the connection */
   snprintf(key, CONNECTION_HASH_KEY_LEN, "%08X%04X%04X", 
		   p_conn->remote_addr, p_conn->remote_port, p_conn->local_port);
#ifdef HASH_DEBUG
   fprintf(stderr,"--- key=[%s]\n",key);
#endif

   for(i = 0; i < CONNECTION_HASH_KEY_LEN-1; i++)
   {
      hash = ((hash << 5) + hash) + (key[i]);
   }

   return (hash & 0x7FFFFFFF) % 65521;
}

/* -------------------------------------------------------------------------
 * Second connection hash function: DEK, modified to return odd numbers only,
 * as required for open-address hashing using double-hash probing.
 * Also uses a 65521 prime modulus to govern the range. 
 * -------------------------------------------------------------------------*/
int connection_hash_function_2( const void *p_data )
{
   tcp_connection_t *p_conn;
   char key[CONNECTION_HASH_KEY_LEN];
   unsigned int hash = CONNECTION_HASH_KEY_LEN-1;
   unsigned int i    = 0;

   if ( !p_data )
	   return -1;

   memset(key,0,sizeof(key));

   /* p_data is a pointer to a tcp_connection_t */
   p_conn = (tcp_connection_t *)p_data;

   /* key is a hex representation of the connection */
   snprintf(key, CONNECTION_HASH_KEY_LEN, "%08X%04X%04X", 
		   p_conn->remote_addr, p_conn->remote_port, p_conn->local_port);

   for(i = 0; i < CONNECTION_HASH_KEY_LEN-1; i++)
   {
      hash = ((hash << 5) ^ (hash >> 27)) ^ (key[i]);
   }
   return (( hash & 0x7FFFFFFF ) % 65521 ) | 0x00000001;
}

/* -------------------------------------------------------------------------
 * Connection Match function returns non-zero if hash elements are identical. 
 * -------------------------------------------------------------------------*/
int connection_match_function( const void *p_data1, const void *p_data2 )
{
   tcp_connection_t *p_conn1, *p_conn2;
   
   if ( !p_data1 || !p_data2 )
	   return 0;

   /* p_data1, p_data2 are pointers to tcp_connection_t */
   p_conn1 = (tcp_connection_t *)p_data1;
   p_conn2 = (tcp_connection_t *)p_data2;

   return (p_conn1->local_addr == p_conn2->local_addr &&
	   p_conn1->local_port == p_conn2->local_port &&
	   p_conn1->remote_addr ==  p_conn2->remote_addr &&
	   p_conn1->remote_port == p_conn2->remote_port );
}

/* --------------------------------------------------
 * Functions to hash the monitors within a collection
 * --------------------------------------------------*/

#define MONITOR_HASH_KEY_LEN 9

/* -------------------------------------------------------------------------------
 * First monitor hash function: DJB with a 65521 prime modulus to govern the range.
 * -------------------------------------------------------------------------------*/
int monitor_hash_function_1( const void *p_data )
{
   tcp_port_monitor_t *p_monitor;
   char key[MONITOR_HASH_KEY_LEN];
   unsigned int hash = 5381;
   unsigned int i    = 0;

   if ( !p_data )
           return -1;
   
   memset(key,0,sizeof(key));

   /* p_data is a pointer to tcp_port_monitor_t */
   p_monitor = (tcp_port_monitor_t *)p_data;

   /* key is a hex representation of the starting port concatenated to the ending port */
   snprintf(key, MONITOR_HASH_KEY_LEN, "%04X%04X",
                   p_monitor->port_range_begin, p_monitor->port_range_end );
#ifdef HASH_DEBUG
   fprintf(stderr,"--- key=[%s]\n",key);
#endif

   for(i = 0; i < MONITOR_HASH_KEY_LEN-1; i++)
   {
      hash = ((hash << 5) + hash) + (key[i]);
   }

   return (hash & 0x7FFFFFFF) % 65521;
}

/* -----------------------------------------------------------------------
 * Second monitor hash function: DEK, modified to return odd numbers only,
 * as required for open-address hashing using double-hash probing.
 * Also uses a 65521 prime modulus to govern the range.
 * -----------------------------------------------------------------------*/
int monitor_hash_function_2( const void *p_data )
{
   tcp_port_monitor_t *p_monitor;
   char key[MONITOR_HASH_KEY_LEN];
   unsigned int hash = MONITOR_HASH_KEY_LEN-1;
   unsigned int i    = 0;

   if ( !p_data )
           return -1;

   memset(key,0,sizeof(key));

   /* p_data is a pointer to a tcp_port_monitor_t */
   p_monitor = (tcp_port_monitor_t *)p_data;

   /* key is a hex representation of the starting port concatenated to the ending port */
   snprintf(key, MONITOR_HASH_KEY_LEN, "%04X%04X",
                   p_monitor->port_range_begin, p_monitor->port_range_end );

   for(i = 0; i < MONITOR_HASH_KEY_LEN-1; i++)
   {
      hash = ((hash << 5) ^ (hash >> 27)) ^ (key[i]);
   }
   return (( hash & 0x7FFFFFFF ) % 65521 ) | 0x00000001;
}

/* ----------------------------------------------------------------------
 * Monitor match function returns non-zero if hash elements are identical.
 * ----------------------------------------------------------------------*/
int monitor_match_function( const void *p_data1, const void *p_data2 )
{
   tcp_port_monitor_t *p_monitor1, *p_monitor2;

   if ( !p_data1 || !p_data2 )
           return 0;

   /* p_data1, p_data2 are pointers to tcp_connection_t */
   p_monitor1 = (tcp_port_monitor_t *)p_data1;
   p_monitor2 = (tcp_port_monitor_t *)p_data2;

   return (p_monitor1->port_range_begin == p_monitor2->port_range_begin &&
	   p_monitor1->port_range_end == p_monitor2->port_range_end);
}

/* ---------------------------------------------------------------------------
 * Port monitor utility functions implementing tcp_port_monitor_function_ptr_t
 * ---------------------------------------------------------------------------*/
void destroy_tcp_port_monitor(
        tcp_port_monitor_t *                    p_monitor,
	void *					p_void 
        )
{
   tcp_connection_node_t *p_node, *p_temp;

   if ( !p_monitor || p_void ) 	/* p_void should be NULL in this context */
      return;

   /* destroy the monitor's hash */
   hash_destroy(&p_monitor->hash);

   /* destroy the monitor's peek array */
   free( p_monitor->p_peek );

   /* destroy the monitor's connection list */
   for ( p_node=p_monitor->connection_list.p_head; p_node!=NULL; )
   {
	   /* p_temp is for the next iteration */
	   p_temp = p_node->p_next;
	   
	   free( p_node );

	   p_node = p_temp;
   }

   /* destroy the monitor */
   free( p_monitor );
   p_monitor=NULL;
}

void age_tcp_port_monitor(
        tcp_port_monitor_t *                    p_monitor,
	void *					p_void
        )
{
   /* Run through the monitor's connections and decrement the age variable. 
    * If the age goes negative, we remove the connection from the monitor. 
    * Function takes O(n) time on the number of connections. */

   tcp_connection_node_t *p_node, *p_temp;
   tcp_connection_t *p_conn;
   void *p_cast;

   if ( !p_monitor || p_void )  /* p_void should be NULL in this context */
	   return;

   if ( !p_monitor->p_peek )
	   return;

   for ( p_node = p_monitor->connection_list.p_head; p_node != NULL; )
   {
	   if ( --p_node->connection.age >= 0 ) {
		   p_node = p_node->p_next;
		   continue;
	   }

	   /* connection on p_node is old.  remove connection from the hash. */
	   p_conn = &p_node->connection;
	   p_cast = (void *)p_conn;
	   if ( hash_remove( &p_monitor->hash, &p_cast ) != 0 ) {
#ifdef HASH_DEBUG
		   fprintf(stderr, "--- hash_remove error\n");
#endif
		   return;
	   }

	   /* splice p_node out of the connection_list */
	   if ( p_node->p_prev != NULL )
		   p_node->p_prev->p_next = p_node->p_next;
	   if ( p_node->p_next != NULL )
		   p_node->p_next->p_prev = p_node->p_prev;

	   /* correct the list head and tail if necessary */
	   if ( p_monitor->connection_list.p_head == p_node )
		   p_monitor->connection_list.p_head = p_node->p_next;
	   if ( p_monitor->connection_list.p_tail == p_node )
		   p_monitor->connection_list.p_tail = p_node->p_prev;

	   /* p_temp is for the next iteration */
	   p_temp = p_node->p_next;

	   /* destroy the node */
	   free( p_node );

	   p_node = p_temp;
   }
}

void maintain_tcp_port_monitor_hash(
	tcp_port_monitor_t *                    p_monitor,
	void *                                  p_void
	)
{
   /* Check the number of vacated slots in the hash.  If it exceeds our maximum
    * threshold (should be about 1/4 of the hash table), then the hash table
    * performance degrades from O(1) toward O(n) as the number of vacated slots
    * climbs.  This is avoided by clearing the hash and reinserting the entries.
    * The benefit of open-addressing hashing does come with this price --
    * you must rebalance it occasionally. */

    tcp_connection_node_t *p_node;
    double vacated_load;

    if ( !p_monitor || p_void )  /* p_void should be NULL in this context */
	return;

    vacated_load = (double)p_monitor->hash.vacated / (double)p_monitor->hash.positions;
#ifdef HASH_DEBUG
    fprintf(stderr,"--- num vacated is %d, vacated factor is %.3f\n", p_monitor->hash.vacated, vacated_load );
#endif
    if ( vacated_load <= TCP_CONNECTION_HASH_MAX_VACATED_RATIO )
    {
	    /* hash is fine and needs no rebalancing */
	    return;
    }

#ifdef HASH_DEBUG
    fprintf(stderr,"--- rebuilding hash\n");
#endif

    /* rebuild the hash  */
    memset( p_monitor->hash.pp_table, 0, p_monitor->hash.positions * sizeof(void **));
    p_monitor->hash.size = 0;
    p_monitor->hash.vacated = 0;

    for ( p_node=p_monitor->connection_list.p_head; p_node!=NULL; p_node=p_node->p_next )
    {
	    if ( hash_insert( &p_monitor->hash, (void *)&p_node->connection ) != 0 )
	    {
#ifdef HASH_DEBUG
	 	fprintf(stderr,"--- hash_insert error\n");
#endif
	    	;   	
	    }
    }
}

void rebuild_tcp_port_monitor_peek_table(
	tcp_port_monitor_t *			p_monitor,
	void *					p_void
	)
{
   /* Run through the monitor's connections and rebuild the peek table
    * of connection pointers.  This is done so peeking into the monitor
    * can be done in O(1) time instead of O(n) time for each peek. */

   tcp_connection_node_t *p_node;
   int i = 0;

   if ( !p_monitor || p_void )  /* p_void should be NULL in this context */
	return;

   /* zero out the peek array */
   memset( p_monitor->p_peek, 0, p_monitor->hash.positions * sizeof(tcp_connection_t *) );

   for ( p_node=p_monitor->connection_list.p_head; p_node!=NULL; p_node=p_node->p_next, i++ )
   {
	   p_monitor->p_peek[i] = &p_node->connection;
   }
}

void show_connection_to_tcp_port_monitor(
        tcp_port_monitor_t *                    p_monitor,
        void *                      		p_void
        )
{
   /* The monitor gets to look at each connection to see if it falls within
    * the monitor's port range of interest.  Connections of interest are first
    * looked up in the hash to see if they are already there.  If they are, we
    * reset the age of the connection so it is not deleted.  If the connection 
    * is not in the hash, we add it, but only if the hash is not saturated.  
    * The function takes O(1) time. */

   tcp_connection_node_t *p_node;
   void *p_cast;

   if ( !p_monitor || !p_void )
	return;

   /* This p_connection is on caller's stack and not the heap.  If we are interested,
    * we will create a copy of the connection (on the heap) and add it to our list. */
   tcp_connection_t *p_connection = (tcp_connection_t *)p_void;
   
   /* inspect the local port number of the connection to see if we're interested. */
   if ( (p_monitor->port_range_begin <= p_connection->local_port) &&
        (p_connection->local_port <= p_monitor->port_range_end) )
   {
  	/* the connection is in the range of the monitor. */

	/* first check the hash to see if the connection is already there. */
	p_cast = (void *)p_connection;
	if ( hash_lookup( &p_monitor->hash, &p_cast ) == 0 )
	{
		p_connection = (tcp_connection_t *)p_cast;
		/* it's already in the hash.  reset the age of the connection. */
		if ( p_connection != NULL )
		{
			p_connection->age = TCP_CONNECTION_STARTING_AGE;
		}

		return;
	}

	/* Connection is not yet in the hash.  We will try to add it, but only if the hash is not
	 * yet saturated.  We assume the hash is saturated (and therefore ignore this connection)
	 * if our load factor cap is now exceeded.  The benefit of limiting connections in this way
	 * is that the hash will continue to function at an average (1) speed by keeping the load
	 * load factor down.  Of course the downside is that each port monitor has a strict maximum 
	 * connection limit. */

	if ( (double)p_monitor->hash.size / (double)p_monitor->hash.positions >= TCP_CONNECTION_HASH_MAX_LOAD_RATIO )
	{
		/* hash exceeds our load limit is now "full" */
		return;
	}

	/* create a new connection node */
	if ( (p_node = (tcp_connection_node_t *) calloc(1, sizeof(tcp_connection_node_t))) == NULL )
		return;

	/* copy the connection data */
	if ( copy_tcp_connection( &p_node->connection, p_connection ) != 0 )
  	{
	 	/* error copying the connection data. deallocate p_node to avoid leaks and return. */
		free( p_node );
		return;
 	}

	p_node->connection.age = TCP_CONNECTION_STARTING_AGE;
	p_node->p_next = NULL;

	/* insert it into the monitor's hash table */
	if ( hash_insert( &p_monitor->hash, (void *)&p_node->connection ) != 0 )
	{
		/* error inserting into hash.  delete the connection node we just created, so no leaks. */
#ifdef HASH_DEBUG
		fprintf(stderr, "--- hash_insert error\n");
#endif
		free(p_node);
		return;
	}

	/* append the node to the monitor's connection list */
	if ( p_monitor->connection_list.p_tail == NULL )  /* assume p_head is NULL too */
	{
		p_monitor->connection_list.p_head = p_node;
		p_monitor->connection_list.p_tail = p_node;
		p_node->p_prev = NULL;
	}
	else
	{
		p_monitor->connection_list.p_tail->p_next = p_node;
		p_node->p_prev = p_monitor->connection_list.p_tail;
		p_monitor->connection_list.p_tail = p_node;
	}
   }
}

/* ---------------------------------------------------------------------------------------
 * Apply a tcp_port_monitor_function_ptr_t function to each port monitor in the collection. 
 * ---------------------------------------------------------------------------------------*/
void for_each_tcp_port_monitor_in_collection(
 	tcp_port_monitor_collection_t *         p_collection,
  	tcp_port_monitor_function_ptr_t		p_function,
	void *					p_function_args
	)
{
   tcp_port_monitor_node_t * p_current_node, * p_next_node;
   
   if ( !p_collection || !p_function )
   	return;

   /* for each monitor in the collection */
   for ( p_current_node = p_collection->monitor_list.p_head; p_current_node != NULL; )
   {
        p_next_node = p_current_node->p_next;  /* do this first! */

	if ( p_current_node->p_monitor )
	{
            /* apply the function with the given arguments */
   	    (*p_function)( p_current_node->p_monitor, p_function_args );
	}

        p_current_node = p_next_node;
   }
  
}

/* ----------------------------------------------------------------------------------------
 * Calculate an efficient hash size based on the desired number of elements and load factor.
 * ---------------------------------------------------------------------------------------- */
int calc_efficient_hash_size(
	int                                     min_elements,
	int					max_hash_size,
	double                                  max_load_factor
	)
{
   double min_size, hash_size, log_base_2;
   
   /* the size of the hash will the smallest power of two such that the minimum number
      of desired elements does not exceed the maximum load factor. */                 
   
   min_size = (double)min_elements / max_load_factor;   /* starting point */
     
   /* now adjust size up to nearest power of two */
   log_base_2 = (double) (int) ( log(min_size) / log(2) ) ;  /* lop off fractional portion of log */

   hash_size = pow(2,log_base_2) >= min_size ? min_size : pow(2,(double)++log_base_2);

   /* respect the maximum */
   hash_size = hash_size <= max_hash_size ? hash_size : max_hash_size;

   /*
   fprintf(stderr,"hash size is %d, based on %d min_elements and %.02f max load, %d maximum\n",
		   (int)hash_size, min_elements, max_load_factor, max_hash_size);
   */

   return hash_size;
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
   so that there are no redundant monitors. */
tcp_port_monitor_t * create_tcp_port_monitor(
	in_port_t 				port_range_begin, 
	in_port_t 				port_range_end,
	tcp_port_monitor_args_t *		p_creation_args
	)
{
   tcp_port_monitor_t * p_monitor;

   /* create the monitor */
   p_monitor = (tcp_port_monitor_t *) calloc(1, sizeof(tcp_port_monitor_t) );
   if ( !p_monitor )
      	return NULL;

   /* create the monitor's connection hash */
   if ( hash_create( &p_monitor->hash, 
			p_creation_args && p_creation_args->min_port_monitor_connections > 0 ?
				calc_efficient_hash_size( p_creation_args->min_port_monitor_connections,
							  TCP_CONNECTION_HASH_SIZE_MAX,
							  TCP_CONNECTION_HASH_MAX_LOAD_RATIO ) :
				TCP_CONNECTION_HASH_SIZE_DEFAULT,
			&connection_hash_function_1, &connection_hash_function_2,
			&connection_match_function, NULL ) != 0 ) 
   {
	/* we failed to create the hash, so destroy the monitor completely so we don't leak */
	destroy_tcp_port_monitor(p_monitor,NULL);
	return NULL;
   }

   /* create the monitor's peek array */
   if ( (p_monitor->p_peek = (tcp_connection_t **) calloc( p_monitor->hash.positions, sizeof(tcp_connection_t *))) == NULL )
   {
	/* we failed to create the peek array, so destroy the monitor completely, again, so we don't leak */
	destroy_tcp_port_monitor(p_monitor,NULL);
	return NULL ;
   }

   p_monitor->port_range_begin = port_range_begin;
   p_monitor->port_range_end = port_range_end;

   p_monitor->connection_list.p_head = NULL;
   p_monitor->connection_list.p_tail = NULL;

   return p_monitor;
}

/* Clients use this function to get connection data from the indicated port monitor.
   The requested monitor value is copied into a client-supplied char buffer.
   Returns 0 on success, -1 otherwise. */
int peek_tcp_port_monitor(
        const tcp_port_monitor_t *              p_monitor,
        int                                     item,
        int                                     connection_index,
        char *                                  p_buffer,
        size_t                                  buffer_size
        )
{
   struct hostent *p_hostent;
   struct servent *p_servent;
   struct in_addr net;

   if ( !p_monitor || !p_buffer || connection_index < 0 )
      	return(-1);

   memset(p_buffer, 0, buffer_size);
   memset(&net, 0, sizeof(net));

   /* if the connection index is out of range, we simply return with no error
    * having first cleared the client-supplied buffer. */
   if ( (item!=COUNT) && (connection_index > p_monitor->hash.size - 1) )
	   return(0);
		   
   switch (item) {

   case COUNT:
   
   	snprintf( p_buffer, buffer_size, "%d" , p_monitor->hash.size );
	break;

   case REMOTEIP:

	net.s_addr = p_monitor->p_peek[ connection_index ]->remote_addr;
	snprintf( p_buffer, buffer_size, "%s", inet_ntoa( net ) );
	break;

   case REMOTEHOST:

	p_hostent = gethostbyaddr( &p_monitor->p_peek[ connection_index ]->remote_addr, sizeof(in_addr_t), AF_INET);
	/* if no host name found, just use ip address. */
  	if ( !p_hostent || !p_hostent->h_name )
	{
		net.s_addr = p_monitor->p_peek[ connection_index ]->remote_addr;
		snprintf( p_buffer, buffer_size, "%s", inet_ntoa( net ) );
	  	break;
	}
	snprintf( p_buffer, buffer_size, "%s", p_hostent->h_name );
	break;

   case REMOTEPORT:

        snprintf( p_buffer, buffer_size, "%d", p_monitor->p_peek[ connection_index ]->remote_port );                          
	break;
	
   case LOCALIP:

	net.s_addr = p_monitor->p_peek[ connection_index ]->local_addr;
	snprintf( p_buffer, buffer_size, "%s", inet_ntoa( net ) );
        break;

   case LOCALHOST:

	p_hostent = gethostbyaddr( &p_monitor->p_peek[ connection_index ]->local_addr, sizeof(in_addr_t), AF_INET);
	/* if no host name found, just use ip address. */
	if ( !p_hostent || !p_hostent->h_name )
	{
		net.s_addr = p_monitor->p_peek[ connection_index ]->local_addr;
		snprintf( p_buffer, buffer_size, "%s", inet_ntoa( net ) );
		break;
	}
	snprintf( p_buffer, buffer_size, "%s", p_hostent->h_name );
	break;

   case LOCALPORT: 

        snprintf( p_buffer, buffer_size, "%d", p_monitor->p_peek[ connection_index ]->local_port );
        break;        

   case LOCALSERVICE:

	p_servent = getservbyport( htons(p_monitor->p_peek[ connection_index ]->local_port ), "tcp" );
	/* if no service name found for the port, just use the port number. */
      	if ( !p_servent || !p_servent->s_name )
	{
		snprintf( p_buffer, buffer_size, "%d", p_monitor->p_peek[ connection_index ]->local_port );
		break;
	}
	snprintf( p_buffer, buffer_size, "%s", p_servent->s_name );
	break;

   default:
	return(-1);
   }

   return(0);
}

/* --------------------------------
 * Client operations on collections
 * -------------------------------- */

/* Create a monitor collection.  Do this one first. */
tcp_port_monitor_collection_t * create_tcp_port_monitor_collection(
	tcp_port_monitor_collection_args_t * 	p_creation_args
	)
{
   tcp_port_monitor_collection_t * p_collection;

   p_collection = (tcp_port_monitor_collection_t *) calloc( 1, sizeof( tcp_port_monitor_collection_t ) );
   if ( !p_collection )
	   return NULL;

   /* create the collection's monitor hash */
   if ( hash_create( &p_collection->hash, 
			p_creation_args && p_creation_args->min_port_monitors > 0 ?
				calc_efficient_hash_size( p_creation_args->min_port_monitors,
							  TCP_MONITOR_HASH_SIZE_MAX,
							  TCP_MONITOR_HASH_MAX_LOAD_RATIO ) :
				TCP_MONITOR_HASH_SIZE_DEFAULT,
			&monitor_hash_function_1, &monitor_hash_function_2,
			&monitor_match_function, NULL ) != 0 )
   {
         /* we failed to create the hash, so destroy the monitor completely so we don't leak */
         destroy_tcp_port_monitor_collection(p_collection);
         return NULL;
   }

   p_collection->monitor_list.p_head = NULL;
   p_collection->monitor_list.p_tail = NULL;

   return p_collection;
}

/* Destroy the monitor collection (and the monitors inside).  Do this one last. */
void destroy_tcp_port_monitor_collection( 
	tcp_port_monitor_collection_t * 	p_collection
	)
{
   tcp_port_monitor_node_t * p_current_node, * p_next_node;

   if ( !p_collection )
	   return;

   /* destroy the collection's hash */
   hash_destroy( &p_collection->hash );

   /* destroy the monitors */
   for_each_tcp_port_monitor_in_collection(
        p_collection,
	&destroy_tcp_port_monitor,
	NULL
	);

   /* next destroy the empty monitor nodes */
   for ( p_current_node = p_collection->monitor_list.p_head; p_current_node != NULL; )
   {
      	p_next_node = p_current_node->p_next;  /* do this first! */

        free( p_current_node );
 	p_current_node = p_next_node;
   }
   
   free( p_collection );
   p_collection=NULL;
}

/* Updates the tcp statistics for all monitors within a collection */
void update_tcp_port_monitor_collection(
        tcp_port_monitor_collection_t *         p_collection
        )
{
	FILE *fp;
        char buf[256];
        tcp_connection_t conn;
        unsigned long inode,uid,state;

	if ( !p_collection )
		return;

	/* age the connections in all port monitors. */
	for_each_tcp_port_monitor_in_collection(
	        p_collection,
	        &age_tcp_port_monitor,
	        NULL
	        );

	/* read tcp data from /proc/net/tcp */
	if ( ( fp = fopen("/proc/net/tcp", "r" ) ) == NULL )
		return;

        /* ignore field name line */
        fgets(buf, 255, fp);

        /* read all tcp connections */
        while (fgets (buf, sizeof (buf), fp) != NULL) {

                if ( sscanf (buf, "%*d: %lx:%hx %lx:%hx %lx %*x:%*x %*x:%*x %*x %lu %*d %lu",
                        (unsigned long *)&conn.local_addr, &conn.local_port,
                        (unsigned long *)&conn.remote_addr, &conn.remote_port,
                        (unsigned long *)&state, (unsigned long *)&uid, (unsigned long *)&inode) != 7 )

                        fprintf( stderr, "/proc/net/tcp: bad file format\n" );

                if ((inode == 0) || (state != TCP_ESTABLISHED)) continue;

		/* show the connection to each port monitor. */
		for_each_tcp_port_monitor_in_collection(
		        p_collection,
		        &show_connection_to_tcp_port_monitor,
		        (void *) &conn
		        );
        }

	fclose(fp);

	/* check the health of the monitor hashes and rebuild them if nedded */
	for_each_tcp_port_monitor_in_collection(
		p_collection,
		&maintain_tcp_port_monitor_hash,
		NULL
		);

	/* rebuild the connection peek tables of all monitors so clients can peek in O(1) time */
	for_each_tcp_port_monitor_in_collection(
		p_collection,
		&rebuild_tcp_port_monitor_peek_table,
		NULL
		);
}

/* After clients create a monitor, use this to add it to the collection.
   Returns 0 on success, -1 otherwise. */
int insert_tcp_port_monitor_into_collection( 
	tcp_port_monitor_collection_t * 	p_collection,
	tcp_port_monitor_t * 			p_monitor
	)
{
   tcp_port_monitor_node_t * p_node;

   if ( !p_collection || !p_monitor )
      	return (-1);

   /* create a container node for this monitor */
   p_node = (tcp_port_monitor_node_t *) calloc( 1, sizeof(tcp_port_monitor_node_t) );
   if ( !p_node )
   	return (-1);

   /* populate the node */
   p_node->p_monitor = p_monitor;
   p_node->p_next = NULL;
	   
   /* add a pointer to this monitor to the collection's hash */
   if ( hash_insert( &p_collection->hash, (void *)p_monitor ) != 0 )
   {
   	/* error inserting into hash.  destroy the monitor's container node so no leaks */
	free( p_node );
	return (-1);
   }

   /* tail of the container gets this node */
   if ( !p_collection->monitor_list.p_tail )
   	p_collection->monitor_list.p_tail = p_node;
   else
   {
	/* p_next of the tail better be NULL */
        if ( p_collection->monitor_list.p_tail->p_next != NULL )
	   return (-1);

        /* splice node onto tail */
      	p_collection->monitor_list.p_tail->p_next = p_node;
  	p_collection->monitor_list.p_tail = p_node;
   }

   /* if this was the first element added */
   if ( !p_collection->monitor_list.p_head )
   	p_collection->monitor_list.p_head = p_collection->monitor_list.p_tail;

   return 0;
}

/* Clients need a way to find monitors */
tcp_port_monitor_t * find_tcp_port_monitor( 
	const tcp_port_monitor_collection_t * 	p_collection, 
	in_port_t 				port_range_begin,
	in_port_t 				port_range_end
	)
{
   tcp_port_monitor_t monitor,*p_monitor;
   void *p_cast;

   if ( !p_collection )
        return NULL;

   /* need a monitor object to use for searching the hash */
   monitor.port_range_begin = port_range_begin;
   monitor.port_range_end = port_range_end;
   p_monitor = &monitor;
   p_cast = (void *)p_monitor;

   /* simple hash table lookup */
   if ( hash_lookup( &p_collection->hash, &p_cast ) == 0 )
   {
	   /* found the monitor and p_cast now points to it */
	   p_monitor = (tcp_port_monitor_t *)p_cast;
	   return( p_monitor );
   }

   return NULL;  /* monitor not found */
}
