#ifndef _SMRH_
#define _SMRH_


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../memory_management/memory_management.h"

struct node_ll{
   void* data;
   struct node_ll* next;
};

struct hazard_pointer_record{
   bool active;
   struct hazard_pointer_record* next;
   void** hp;
   struct node_ll* r_list;
   int32_t r_count;
};

struct hazard_pointer{
   size_t nb_pointers;
   struct hazard_pointer_record* head;
   void (*free_node)(void*);
   uint32_t h;
};

struct hazard_pointer* new_hazard_pointer(void (*free_node)(void*), uint32_t nb_pointers);

void hp_subscribe(struct hazard_pointer* hp);

void hp_unsubscribe(struct hazard_pointer* hp);

void** hp_get(struct hazard_pointer* hp, uint32_t index);

void hp_delete_node(struct hazard_pointer* hp, void* node);

void free_hp(struct hazard_pointer* hp);

#endif
