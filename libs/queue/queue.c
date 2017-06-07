#include "queue.h"

// Atomic actions macros
#define atomic_compare_and_swap(t,old,new) __sync_bool_compare_and_swap (t, old, new)
#define atomic_load_item(p) ({struct queue_item* __tmp = *(p); __builtin_ia32_lfence (); __tmp;})

#define fetch_and_inc(n) __sync_fetch_and_add (n, 1)
#define fetch_and_dec(n) __sync_fetch_and_sub (n, 1)

struct queue* new_queue(uint32_t capacity)
{
   // TODO
   return NULL;
}

bool queue_push(struct queue* queue, void* data)
{
   // We check and increment first: to ensure that we do not exceed the queue capacity
   if(queue->size >= queue->max_size)
      return false;
   else
      fetch_and_inc(&queue->size)
   
   // Once incremented we "booked" a slot and can push the item
   struct queue_item* item = new_queue_item(data);
   for(;;)
   {
      struct queue_item* tail = queue->tail;
      struct queue_item* head = queue->head;
      tail->next = queue->head;
      if(atomic_compare_and_swap(&(queue->tail), tail, item))
      {
         atomic_compare_and_swap(&(queue->head), NULL, item);
         break;
      }
   }
}

void* queue_pop(struct queue* queue)
{
   // TODO
   struct queue_item* item;
   for(;;)
   {
      item = (*pool);
      if(atomic_compare_and_swap(&(*pool), list, list->next) && list)
         break;
   }
   return list;
}

void free_queue(struct queue* queue)
{
   // TODO
   return NULL;
}


/*          Private Functions              */

struct queue_item* new_queue_item(void* data)
{
   struct queue_item* result = chkmalloc(sizeof *result);
   result->next = NULL;
   result->data = data;
}
