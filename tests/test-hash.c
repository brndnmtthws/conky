/* ------------------------------------------------------
 * test-hash.c: unit testing for hash functions in hash.h
 * Philip Kovacs kovacsp3@comcast.net 2005
 * 
 * $Id$
 * ------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

char *data[] = { 
	"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", 
	"eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", 
	"eighteen", "nineteen", "twenty", "twenty-one", "twenty-two", "twenty-three", 
	"twenty-four", "twenty-five", "twenty-six", "twenty-seven", "twenty-eight", 
        "twenty-nine", "thirty", "thirty-one", "thirty-two" 
};

/* Primary hash function is DJB with a 65521 prime modulus to govern the range. */
unsigned int DJBHash(const char* str, unsigned int len)
{
   unsigned int hash = 5381;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = ((hash << 5) + hash) + (*str);
   }

   return (hash & 0x7FFFFFFF) % 65521;
}

/* Second hash function is DEK, modified to always return an odd number,
   as required for open-address hashing using double-hash probing and
   also with a 65521 prime modulus to govern the range. */
unsigned int DEKHash(const char* str, unsigned int len)
{
   unsigned int hash = len;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = ((hash << 5) ^ (hash >> 27)) ^ (*str);
   }
   return (( hash & 0x7FFFFFFF ) % 65521 ) | 0x00000001;
}

int hash_fun1( const void *p_data )
{
   char *p_item = (char *)p_data;

   return DJBHash( p_item, strlen(p_item) );
}

int hash_fun2( const void *p_data )
{
   char *p_item = (char *)p_data;

   return DEKHash( p_item, strlen(p_item) );
}

/* must return non-zero if a match */
int match_fun( const void *p_data1, const void *p_data2 )
{
   int l1,l2;
   char *p_item1, *p_item2;

   p_item1 = (char *)p_data1;
   p_item2 = (char *)p_data2;

   l1=strlen( p_item1 );
   l2=strlen( p_item2 );

   return (l1==l2) && (strncmp( p_item1, p_item2, l1 ) == 0);
}

int main()
{
   int i,size,modulus;
   size=128;
   modulus=32;

   hash_table_t hash;

   printf("testing hash functions 1 and 2...\n");
   for ( i=0; i<31; i++ ) {
      printf("(func 1,func 2): hash of \"%s\" mod %d is (%d,%d)\n", 
		data[i], modulus, hash_fun1(data[i]) % modulus, hash_fun2(data[i]) % modulus );
   }

   printf("creating hash table with %d positions...\n",size);
   if ( hash_create( &hash, size, &hash_fun1, &hash_fun2, &match_fun, NULL ) != 0 ) {
      fprintf(stderr,"error creating hash table!\n");
      exit(1);
   }
 
   printf("testing that double-hashes can visit all slots...\n");
   for ( i=0; i<31; i++ ) {
      printf("inserting \"%s\" into hash...\n", data[i]); fflush(stdout);
      if ( hash_insert( &hash, data[i] ) != 0 ) {
	 fprintf(stderr, "error inserting value!\n");
	 exit(1);
      }
      printf("ok\n");
      printf("looking up \"%s\"...\n", data[i]); fflush(stdout);
      if ( hash_lookup( &hash, (void **)&data[i] ) != 0 ) {
	 fprintf(stderr, "error looking up value!\n");
	 exit(1);
      }
      printf("found\n");
      printf("hash size now %d, vacated slots %d\n",hash_size(&hash),hash.vacated);
   }

   printf("removing all items from hash...\n");
   for ( i=0; i<31; i++ ) {
      printf("deleting \"%s\" from hash...\n", data[i]); fflush(stdout);
      if ( hash_remove( &hash, (void **)&data[i] ) != 0 ) {
         fprintf(stderr, "error deleting value!\n");
         exit(1);
      }
      printf("ok\n");
      printf("looking up \"%s\"...\n", data[i]); fflush(stdout);
      if ( hash_lookup( &hash, (void **)&data[i] ) != -1 ) {
         fprintf(stderr, "error: deleted value still found in hash!\n");
         exit(1);
      }
      printf("not found, good\n");
      printf("hash size now %d, vacated slots %d\n",hash_size(&hash),hash.vacated);
   }

   printf("destroying hash table...\n");
   hash_destroy(&hash);

   return 0;
}
