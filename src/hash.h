#ifndef HASH_H
#define HASH_H

/* ------------------------------------------------------
 * Open-addressed hash using double hash probing
 *
 * for i in 0 to m-1: 
 *	h(k, i) = ( h1(k) + i*h2(k) ) mod m 
 *
 * requires: 1) m must be a power of two
 *           2) h2(k) must return an odd number
 *
 * Besed on code published in _Mastering Algorithms in C_
 * by Kyle Loudon (O'Reilly 1999).
 * Modified by Philip Kovacs (kovacsp3@comcast.net)
 * ------------------------------------------------------ */

typedef struct _hash_table_t {
	int 	positions;
	int	size;
	int	vacated;
 	int	sentinel_vacated;
 	void	*p_vacated;
	int	(*p_hash_fun1)(const void *p_data);
	int	(*p_hash_fun2)(const void *p_data);
	int 	(*p_match_fun)(const void *p_data1, const void *p_data2);
	void 	(*p_destroy_fun)(void *p_data);
	void	**pp_table;
} hash_table_t;

/* Create and initialize a hash table on the heap.
   Returns 0 on success, -1 otherwise. */
int hash_create( hash_table_t *p_hash_table,
		 int positions,
		 int (*p_hash_fun1)(const void *p_data),
		 int (*p_hash_fun2)(const void *p_data),
		 int (*p_match_fun)(const void *p_data1, const void *p_data2),
   		 void (*p_destroy_fun)(void *p_data) 
	);

/* Destroy a hash table */
void hash_destroy( hash_table_t *p_hash_table );

/* Insert an element into a hash table.
   Returns 0 on successful insert, 1 if data already in hash and -1 if unable to insert. */
int hash_insert( hash_table_t *p_hash_table, const void *p_data);

/* Delete an element from a hash table.
   Returns 0 on successful delete, -1 if not found. */
int hash_remove( hash_table_t *p_hash_table, void **pp_data);

/* Lookup an element in a hash table.
   Returns 0 if found (data passed back byref), -1 if not found. */
int hash_lookup(const hash_table_t *p_hash_table, void **pp_data);

/* Return size of a hash table */
#define hash_size(p_hash_table) ((p_hash_table)->size)

#endif
