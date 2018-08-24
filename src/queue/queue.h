#ifndef _QUEUEH_
#define _QUEUEH_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../SMR/hazard_pointer.h"

struct queue_item{
   struct queue_item* next;
   void* data;
};

struct queue{
   struct queue_item* head;
   struct queue_item* tail;
   size_t size;
   size_t max_size;
   struct hazard_pointer* hp;
};

struct queue* new_queue(size_t capacity, size_t nb_thread);

bool queue_push(struct queue* queue, void* data);

void* queue_pop(struct queue* queue);

void free_queue(struct queue* queue);

#endif
