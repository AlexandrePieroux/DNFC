#ifndef _SMRH_
#define _SMRH_


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../memory_management/memory_management.h"

struct hazard_pointer{
   void** hp;
   void** dlist;
   size_t size;
   size_t nb_threads;
   size_t subscribed_threads;
   void (*free_node)(void*);
};

struct hazard_pointer* new_hazard_pointer(size_t size, size_t nb_threads, void (*free_node)(void*));

void** hp_get(struct hazard_pointer* hp, uint32_t index);

void hp_delete_node(struct hazard_pointer* hp, void* node);

void free_hp(struct hazard_pointer* hp);

#endif
