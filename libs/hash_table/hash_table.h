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
#include "../linked_list/linked_list.h"


/************** Hash functions available ***************/
#include "FNV-1.h"
#include "murmur3.h"

#define FNV_1          1
#define MURMUR      2



#define THRESHOLD 0.75



/*******************************************************************
* NAME :                 struct hash_table
*
* DESCRIPTION :      Structure that hold all the necessarily informations to perform
*                             the dynamic classification
*
* MEMBERS:
*
*******************************************************************/
struct hash_table
{
   struct linked_list*** index;
   struct linked_list* pool;
   uint32_t size;
   uint32_t nb_elements;
   uint32_t (*hash)(uint32_t);
};



/*******************************************************************
* NAME :                 struct hash_table* new_hash_table(int)
*
* DESCRIPTION :      Initialize a new hash table of size 2^"size". The hash table size is dynamicaly
*                             updated when the density reach 75%, the size is then doubled. The maximum
*                             number of elements that a hash table can contain is 2^32 elements.
*
* INPUTS :
*       PARAMETERS:
*           int hash_type            The type of hash function.
*           uint32_t size             The initial size of the hash table.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*            struct hash_table* table         The output hash table allocated.
*       RETURN :
*            Type:     struct hash_table*
*            Values:   table                         Sucessfully allocated hash table
*
* PROCESS :
*                   [1]  Allocate the structure
*                   [2]  Initialize algorithm variables a and b
*
* NOTES :      NONE
*******************************************************************/
void hash_table_init(void);



/*******************************************************************
* NAME :                 struct hash_table* new_hash_table(int)
*
* DESCRIPTION :      Initialize a new hash table of size 2^"size". The hash table size is dynamicaly
*                             updated when the density reach 75%, the size is then doubled. The maximum
*                             number of elements that a hash table can contain is 2^32 elements.
*
* INPUTS :
*       PARAMETERS:
*           int hash_type            The type of hash function.
*           uint32_t size             The initial size of the hash table.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*            struct hash_table* table         The output hash table allocated.
*       RETURN :
*            Type:     struct hash_table*
*            Values:   table                         Sucessfully allocated hash table
*
* PROCESS :
*                   [1]  Allocate the structure
*                   [2]  Initialize algorithm variables a and b
*
* NOTES :      NONE
*******************************************************************/
struct hash_table* new_hash_table(uint32_t hash_type);



/*******************************************************************
* NAME :                 void hash_table_free(struct hash_table*);
*
* DESCRIPTION :      Free the given hash table.
*
* INPUTS :
*       PARAMETERS:
*           struct hash_table* table           The hash table to free.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*            None
*
*
* NOTES :      NONE
*******************************************************************/
void hash_table_free(struct hash_table** table);



/*******************************************************************
* NAME :                 int put(struct hash_table table, uint32_t key, void* value)
*
* DESCRIPTION :      Insert in the given hash table a value at the index hash(key). Where
*                             hash() is the hash function used to compute the index (here the hash function
*                             used is the universal hash).
*
* INPUTS :
*       PARAMETERS:
*           uint32_t key         The key of the value to insert.
*           void* value           The value to insert in the hash table.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*           struct hash_table* table     The hash table where the data will be inserted.
*       GLOBALS :
*            None
*       RETURN :
*            Type:      int                            Error code:
*            Values:   SUCESS                         Operation sucess
*                          ERROR                           Insertion failed
* PROCESS :
*                   [1]  Check the size of the hash table - resize
*                   [2]  Hash the key
*                   [3]  Check the index - handle colision
*                   [4]  Create and insert the new linked_list
*
* NOTES :      NONE
*******************************************************************/
bool hash_table_put(struct hash_table* table, uint32_t key, void* value);



/*******************************************************************
* NAME :                 int get(struct hash_table* table, uint32_t key)
*
* DESCRIPTION :      Retrieve the value stored at the hash(key). Where
*                             hash() is the hash function used to compute the index (here the hash function
*                             used is the universal hash).
*
* INPUTS :
*       PARAMETERS:
*           struct hash_table* table     The hash table where the data will be retreived.
*           uint32_t key                      The key of the value to get.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*            None
*       RETURN :
*            Type:      void*                           The pointer to the data retreived on sucess, NULL otherwise.
*
* PROCESS :
*                   [1]  Locate the element in the hash table
*                   [2]  if found return the void pointer data otherwise NULL
*
* NOTES :      NONE
*******************************************************************/
void* hash_table_get(struct hash_table* table, uint32_t key);



/*******************************************************************
* NAME :                 void* get(struct hash_table* table, uint32_t key)
*
* DESCRIPTION :      Remove the value at the index hash(key) and return
*                                        the data tha was removed. Where hash() is the hash
*                                        function used to compute the index (here the hash function
*                                        used is the universal hash).
*
* INPUTS :
*       PARAMETERS:
*           uint32_t key                      The key of the value to remove.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*           struct hash_table* table     The hash table where the data will be removed.
*       GLOBALS :
*            None
*       RETURN :
*            Type:      void*                      The data removed
*
* PROCESS :
*                   [1]  Locate the element in the hash table
*                   [2]  If found remove the element and return the
*                          void pointer data otherwise NULL
*
* NOTES :      NONE
*******************************************************************/
bool hash_table_remove(struct hash_table* table, uint32_t key);
