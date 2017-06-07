#ifndef _QUEUEH_
#define _QUEUEH_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "../memory_management/memory_management.h"

struct queue_item{
   struct queue_item* next:
   void* data;
};

struct queue{
   struct queue_item* head;
   struct queue_item* tail;
   uint32_t size;
   uint32_t max_size;
};

struct queue* new_queue(uint32_t capacity);

bool queue_push(struct queue* queue, void* data);

void* queue_pop(struct queue* queue);

void free_queue(struct queue* queue);

#endif