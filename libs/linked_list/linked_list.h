/*H**********************************************************************
* FILENAME :        linked_list.h
*
* DESCRIPTION :
*        A lock-free ordered linked list implementation. The list is only linked by "next" pointers and do not
*        support upward traversal. The "flag" bit used to mark a node is the last bit of the address of the pointer.
*        This assume that the processor fecth addresses on even numbers.
*
*
* PUBLIC FUNCTIONS :
*   struct linked_list*                          new_linked_list ( uint32_t , void* );
*   void                                            linked_list_put ( struct linked_list* , uint32_t , void* );
*   void*                                           linked_list_get ( struct linked_list* , uint32_t );
*   void*                                           linked_list_remove ( struct linked_list** , uint32_t );
*   bool                                            linked_list_contains_key ( struct linked_list* , uint32_t );
*   void                                            linked_list_free ( struct linked_list* );
*
* PUBLIC STRUCTURE :
*       struct linked_list
*
* AUTHOR :    Pieroux Alexandre
*H*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include "../memory_management/memory_management.h"



typedef uint32_t key_type;
typedef uint32_t hash_type;
typedef void* data_type;



/*******************************************************************
* NAME :                 struct linked_list
*
* DESCRIPTION :      Structure that hold all the necessarily information
*                                        of the linked list.
*
* MEMBERS:
*        key_type key                             The key index of the element.
*        data_type data                       The data stored in the node.
*        struct linked_list* next      The pointer to the next element (this address can be tagged using the LSB)
*
*******************************************************************/
struct linked_list
{
   struct linked_list* next;
   key_type key;
   hash_type hash;
   data_type data;
};



/*******************************************************************
* NAME :                 void linked_list_init ( void )
*
* DESCRIPTION :      Initialize the linked list library.
*
* INPUTS :
*       PARAMETERS:
*           None
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*           None
*
*
* NOTES :      NONE
*******************************************************************/
void linked_list_init(void);



/*******************************************************************
* NAME :                 struct linked_list* new_linked_list ( uint32_t , void* )
*
* DESCRIPTION :      Initialize a new linked list element. Each element is a
*                            sub-linked list that have a pointer to the next element.
*
* INPUTS :
*       PARAMETERS:
*           void* key            The key index of the element to create.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           struct linked_list* list   The new linked list element.
*       RETURN :
*            Type:     struct linked_list*
*            Values:  list             Sucessfully initialized list.
*
*
* NOTES :      NONE
*******************************************************************/
struct linked_list* new_linked_list(
  key_type key,
  hash_type hash,
  data_type data,
  struct linked_list** pool);



/*******************************************************************
* NAME :                 void linked_list_insert ( struct linked_list** , uint32_t , void* )
*
* DESCRIPTION :      Put a new ellement "data" in the linked list "list" with the index "key".
*                            A new element is allocated. The new ellement is inserted in a ordered manner.
*
* INPUTS :
*       PARAMETERS:
*           struct linked_list** list            The list pointer where the element will be inserted.
*           struct linked_list*   item         The linked list element to insert.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            struct linked_list* list           The list with the new element inserted
*       GLOBALS :
*            None
*       RETURN :
*            Type:         bool
*            Values:     result                           The result of the insertion.
*
* NOTES :      NONE
*******************************************************************/
struct linked_list* linked_list_insert(
  struct linked_list** list,
  struct linked_list* item,
  struct linked_list** pool);



/*******************************************************************
* NAME :                 void* linked_list_get ( struct linked_list* , uint32_t )
*
* DESCRIPTION :      Check is a specified key is present in the list. If
*                             found the function return true, otherwise false. The search in the list is done in
*                            O(N) where N is the number of elements in the list.
* INPUTS :
*       PARAMETERS:
*           struct linked_list* list         The list from where the list is retreived.
*           uint32_t key                         The key index of the element to get.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*            Type:         bool
*            Values:     result                           The result of the search.
*
* NOTES :      NONE
*******************************************************************/
struct linked_list* linked_list_get(
  struct linked_list** list,
  key_type key,
  hash_type hash,
  struct linked_list** pool);



/*******************************************************************
* NAME :                 void* linked_list_get ( struct linked_list* , uint32_t )
*
* DESCRIPTION :      Check is a specified key is present in the list. If
*                             found the function return true, otherwise false. The search in the list is done in
*                            O(N) where N is the number of elements in the list.
* INPUTS :
*       PARAMETERS:
*           struct linked_list* list         The list from where the list is retreived.
*           uint32_t key                         The key index of the element to get.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*           None
*       RETURN :
*            Type:         bool
*            Values:     result                           The result of the search.
*
* NOTES :      NONE
*******************************************************************/
bool linked_list_find(
  struct linked_list** list,
  key_type key,
  hash_type hash,
  struct linked_list** pool);



/*******************************************************************
* NAME :                 void* linked_list_delete ( struct linked_list** , uint32_t )
*
* DESCRIPTION :      Remove an element of the list according to the specified key if it is in the list.
*
* INPUTS :
*       PARAMETERS:
*           struct linked_list**  list                The list where the element will be removed from.
*           uint32_t key                                   The key index of the element.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            struct linked_list** list                The list with the element removed.
*       GLOBALS :
*       RETURN :
*            Type:      bool
*            Values:   data                              The result of the operation.
*
* NOTES :      NONE
*******************************************************************/
bool linked_list_delete(
  struct linked_list** list,
  key_type key,
  hash_type hash,
  struct linked_list** pool);



/*******************************************************************
* NAME :                 void linked_list_free ( struct linked_list* )
*
* DESCRIPTION :      Free the specified linked list.
*
* INPUTS :
*       PARAMETERS:
*           struct linked_list* list             The linked list to free.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*       RETURN :
*            None
*
* NOTES :      NONE
*******************************************************************/
void linked_list_free(struct linked_list** list);



/*******************************************************************
* NAME :                 void linked_list_free ( struct linked_list* )
*
* DESCRIPTION :      Free the specified linked list.
*
* INPUTS :
*       PARAMETERS:
*           struct linked_list* list             The linked list to free.
*       GLOBALS :
*           None
* OUTPUTS :
*       PARAMETERS:
*            None
*       GLOBALS :
*       RETURN :
*            None
*
* NOTES :      NONE
*******************************************************************/
struct linked_list* get_cur(void);
