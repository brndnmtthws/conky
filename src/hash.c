/* ------------------------------------------------------
 * Open-addressed hash using double hash probing
 *
 * for i in 0 to m-1: 
 *      h(k, i) = ( h1(k) + i*h2(k) ) mod m 
 *
 * requires: 1) m must be a power of two
 *           2) h2(k) must return an odd number
 *
 * Besed on code published in _Mastering Algorithms in C_
 * by Kyle Loudon (O'Reilly 1999).
 * Modified by Philip Kovacs (kovacsp3@comcast.net)
 * 
 * $Id$
 * ------------------------------------------------------ */

#ifdef HASH_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "hash.h"

/* Create and initialize a hash table on the heap.
   Returns 0 on success, -1 otherwise. */
int hash_create( hash_table_t *p_hash_table,
                 int positions,
                 int (*p_hash_fun1)(const void *p_data),
                 int (*p_hash_fun2)(const void *p_data),
                 int (*p_match_fun)(const void *p_data1, const void *p_data2),
                 void (*p_destroy_fun)(void *p_data)
        )
{
    if ( ( p_hash_table->pp_table = (void **)calloc(positions,  sizeof(void *))) == NULL )
       	return -1;

    p_hash_table->positions = positions;
    p_hash_table->size = 0;
    p_hash_table->vacated = 0;

    /* sentinel address indicating a vacated slot */
    p_hash_table->p_vacated = &p_hash_table->sentinel_vacated;

    p_hash_table->p_hash_fun1 = p_hash_fun1;
    p_hash_table->p_hash_fun2 = p_hash_fun2;

    p_hash_table->p_match_fun = p_match_fun;
    p_hash_table->p_destroy_fun = p_destroy_fun;

    return 0;
}


/* Destroy a hash table */
void hash_destroy( hash_table_t *p_hash_table )
{
   int	i;

   if ( !p_hash_table )
	return;

   /* Destroy the elements the hash points to, if a destroy function was provided */
   if (p_hash_table->p_destroy_fun != NULL) {

   	for (i = 0; i < p_hash_table->positions; i++) {

      		if ( p_hash_table->pp_table[i] != NULL && p_hash_table->pp_table[i] != p_hash_table->p_vacated )
         	   p_hash_table->p_destroy_fun( p_hash_table->pp_table[i] );
   	}
   }

   free( p_hash_table->pp_table );
   memset( p_hash_table, 0, sizeof(hash_table_t) );

   return;
}

/* Insert an element into a hash table.
   Returns 0 on successful insert, 1 if data already in hash and -1 if unable to insert. */
int hash_insert( hash_table_t *p_hash_table, const void *p_data )
{
   void *temp;
   int position, i;
   int hashed_1, hashed_2;

   if ( !p_hash_table )
	return -1;

   if ( p_hash_table->size == p_hash_table->positions )
      	return -1;

   temp = (void *)p_data;

   if ( hash_lookup( p_hash_table, &temp ) == 0 )
   	return 1;

   /* Loudon's original algorithm needlessly repeated running the hash algorithms with each iteration
      to find a slot.   Just running the hash algorithms once is enough since they are deterministic. */
   hashed_1 = p_hash_table->p_hash_fun1( p_data );
   hashed_2 = p_hash_table->p_hash_fun2( p_data );

   for ( i = 0; i < p_hash_table->positions; i++ ) {

   	position = ( hashed_1 + (i * hashed_2) ) % p_hash_table->positions;
#ifdef HASH_DEBUG
       	printf("--- hash_insert: probe %d, position %d\n",i,position);
#endif

   	if ( p_hash_table->pp_table[ position ] == NULL ) /* empty slot */
	{
      		p_hash_table->pp_table[ position ] = (void *)p_data;
      		p_hash_table->size++;
      		return 0;	
   	}

   	if ( p_hash_table->pp_table[ position ] == p_hash_table->p_vacated ) /* vacated slot */
	{
      		p_hash_table->pp_table[ position ] = (void *)p_data;
      		p_hash_table->size++;
		p_hash_table->vacated--;
      		return 0;	
   	}
   }

   /* hash functions not selected correctly since the above algorithm should visit all slots in the hash. */
   return -1;
}

/* Delete an element from a hash table.
   Returns 0 on successful delete, -1 if not found. */
int hash_remove( hash_table_t *p_hash_table, void **pp_data)
{
   int position, i;
   int hashed_1, hashed_2;

   if ( !p_hash_table || !pp_data )
        return -1; 

   hashed_1 = p_hash_table->p_hash_fun1( *pp_data );
   hashed_2 = p_hash_table->p_hash_fun2( *pp_data );

   for (i = 0; i < p_hash_table->positions; i++) {

   	position= ( hashed_1 + (i * hashed_2) ) % p_hash_table->positions;
#ifdef HASH_DEBUG
	printf("--- hash_remove: probe %d, position %d\n",i,position);
#endif

   	if ( p_hash_table->pp_table[ position ] == NULL ) {

      		return -1;

        }

   	else if ( p_hash_table->pp_table[ position ] == p_hash_table->p_vacated ) {

      		continue;

        }

   	else if ( p_hash_table->p_match_fun( p_hash_table->pp_table[ position ], *pp_data)) {

      		*pp_data = p_hash_table->pp_table[ position ];
      		p_hash_table->pp_table[ position ] = p_hash_table->p_vacated;
		p_hash_table->vacated++;
      		p_hash_table->size--;
      		return 0;
   	}
   }

   return -1;
}

/* Lookup an element in a hash table.
   Returns 0 if found (data passed back byref), -1 if not found. */
int hash_lookup(const hash_table_t *p_hash_table, void **pp_data)
{
   int position, i;
   int hashed_1, hashed_2;

   if ( !p_hash_table || !pp_data )
	return -1;

   hashed_1 = p_hash_table->p_hash_fun1( *pp_data );
   hashed_2 = p_hash_table->p_hash_fun2( *pp_data );

   for (i = 0; i < p_hash_table->positions; i++) {

    	position= ( hashed_1 + (i * hashed_2) ) % p_hash_table->positions;
#ifdef HASH_DEBUG
        printf("--- hash_lookup: probe %d, position %d\n",i,position);
#endif
   	if ( p_hash_table->pp_table[ position ] == NULL ) {

      		return -1;

      	}

   	else if ( p_hash_table->p_match_fun(p_hash_table->pp_table[ position ], *pp_data) ) {

      		*pp_data = p_hash_table->pp_table[ position ];
      		return 0;

   	}
   }

   return -1;
}
