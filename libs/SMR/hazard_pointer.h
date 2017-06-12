#ifndef _SMRH_
#define _SMRH_


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../memory_management/memory_management.h"

struct hazard_pointer{
   void** hp;
   size_t size;
   size_t nb_threads;
   size_t subscribed_threads;
   pthread_key_t* dcount_k;
   pthread_key_t* dlist_k;
   pthread_key_t* id_k;
   void (*free_node)(void*);
};

struct hazard_pointer* new_hazard_pointer(size_t size, size_t nb_threads);

bool hp_subscribe(struct hazard_pointer* hp);

void* hp_get(struct hazard_pointer* hp, uint32_t index);

void hp_delete_node(struct hazard_pointer* hp, void* node);

#endif
