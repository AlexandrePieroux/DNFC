#ifndef _HASH_TABLEH_
#define _HASH_TABLEH_
/*H**********************************************************************
* FILENAME :        hash_table.h
*
* DESCRIPTION :
*        A hash table implementation that use integer hashing and dynamic
*        incremental resiszing. The colisions are handled with linked_list structures.
*
* PUBLIC FUNCTIONS :
*       struct hash_table*       new_hash_table( size_t )
*       int                               put( struct hash_table*, uint32_t, void* )
*       void*                           get( struct hash_table*, uint32_t )
*       int                               remove( struct hash_table*, uint32_t )
*       int                               contains_key( struct hash_table*, uint32_t )
*
* PUBLIC STRUCTURE :
*       struct hash_table
*
*
* AUTHOR :    Pieroux Alexandre
*H*/

#define _XOPEN_SOURCE

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>


/************** Utilities *****************/
#include "../memory_management/memory_management.h"
#include "../byte_stream/byte_stream.h"
#include "../linked_list/linked_list.h"


/************** Available hash functions ***************/
#include "FNV-1a.h"
#include "murmur3.h"


#define FNV_1       1
#define MURMUR      2



#define THRESHOLD 0.75


struct hash_table
{
   struct linked_list*** index;
   struct hazard_pointer* hp;
   uint32_t size;
   uint32_t nb_elements;
   uint32_t (*hash)(key_type);
};


struct hash_table* new_hash_table(uint32_t hash_type, uint32_t nb_threads);

void hash_table_free(struct hash_table** table);

bool hash_table_put(struct hash_table* table, key_type key, void* value);

void* hash_table_get(struct hash_table* table, key_type key);

bool hash_table_remove(struct hash_table* table, key_type key);

#endif
