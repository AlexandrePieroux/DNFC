#ifndef _LINKED_LISTH_
#define _LINKED_LISTH_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include "../SMR/hazard_pointer.h"
#include "../byte_stream/byte_stream.h"



typedef struct byte_stream* key_type;
typedef uint32_t hash_type;
typedef void* data_type;



struct linked_list
{
   struct linked_list* next;
   key_type key;
   hash_type hash;
   data_type data;
};



struct linked_list* new_linked_list(
  key_type key,
  hash_type hash,
  data_type data);



struct linked_list* linked_list_insert(
  struct hazard_pointer* hp,
  struct linked_list** list,
  struct linked_list* item);



struct linked_list* linked_list_get(
  struct hazard_pointer* hp,
  struct linked_list** list,
  key_type key,
  hash_type hash);



bool linked_list_delete(
  struct hazard_pointer* hp,
  struct linked_list** list,
  key_type key,
  hash_type hash);



void linked_list_free(
  struct linked_list** list);



void delete_node(
  struct linked_list* node);


#endif
