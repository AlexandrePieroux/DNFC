#include "queue.h"

// Atomic actions macros
#define atomic_compare_and_swap(t,old,new) __atomic_compare_exchange_n(t, old, new, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
#define atomic_load_item(p) __atomic_load_n(p, __ATOMIC_RELAXED)

#define fetch_and_inc(n) __atomic_fetch_add(n, 1, __ATOMIC_RELAXED)
#define fetch_and_dec(n) __atomic_fetch_sub(n, 1, __ATOMIC_RELAXED)

#define NB_HP         3

#define cur_index     0
#define next_index    1

/*                                     Private function                                               */

// Instantiate a new queue item that contain 'data'
struct queue_item* new_queue_item(void* data);

// Free item function
void free_item(struct queue_item* item);

/*                                     Private function                                               */


/* Queue functions */

struct queue* new_queue(size_t capacity, size_t nb_thread)
{
   struct queue* queue = chkmalloc(sizeof *queue);
   struct queue_item* dummy_node = new_queue_item(NULL);
   queue->size = 0;
   queue->max_size = capacity;
   queue->head = dummy_node;
   queue->tail = dummy_node;
   queue->hp = new_hazard_pointer((void(*)(void*))free_item, NB_HP);
   return queue;
}

bool queue_push(struct queue* queue, void* data)
{
   // We check and increment first: to ensure that we do not exceed the queue capacity
   for(;;)
   {
      if(queue->size >= queue->max_size)
         return false;
      if(fetch_and_inc(&queue->size))
         break;
   }
   
   // Get hazardous pointer
   hp_subscribe(queue->hp);
   struct queue_item** cur = (struct queue_item**) hp_get(queue->hp, cur_index);
   
   // Prepare new data
   struct queue_item* node = new_queue_item(data);
   
   // Till we succeed
   struct queue_item* tail;
   for(;;)
   {
      // Load tail and make the hazard point to it
      tail = atomic_load_item(&queue->tail);
      *cur = tail;
      
      // Check that the tail does not have changed
      if(tail != atomic_load_item(&queue->tail))
         continue;
      
      // Read the next node of tail (should be null)
      struct queue_item* next = atomic_load_item(&tail->next);
      
      // Check that the tail does not have changed
      if(tail != atomic_load_item(&queue->tail))
         continue;
      
      // If next is not null we advance the pointer of the tail to the next node
      if(next)
      {
         atomic_compare_and_swap(&queue->tail, &tail, next);
         continue;
      }
      
      // Try to effectively push the new node
      void* null_ptr = chkmalloc(sizeof(null_ptr));
      null_ptr = (void*) NULL;
      if(atomic_compare_and_swap(&tail->next, null_ptr, node))
         break;
   }
   // Set the tail to the new node
   atomic_compare_and_swap(&queue->tail, &tail, node);
   *cur = NULL;
   hp_unsubscribe(queue->hp);
   return true;
}

void* queue_pop(struct queue* queue)
{
   // Get hazardous pointers
   hp_subscribe(queue->hp);
   struct queue_item** hp_cur = (struct queue_item**) hp_get(queue->hp, cur_index);
   struct queue_item** hp_next = (struct queue_item**) hp_get(queue->hp, next_index);
      
   void* result;
   struct queue_item* head;
   for(;;)
   {
      // Load the head of the queue and make the hazard pointer point to it
      head = atomic_load_item(&queue->head);
      *hp_cur = head;
      
      // Check that the head does not have changed
      if(head != atomic_load_item(&queue->head))
         continue;
      
      // Load the tail and the next item of the head and make the hazard pointer point to it
      struct queue_item* tail = atomic_load_item(&queue->tail);
      struct queue_item* next = atomic_load_item(&head->next);
      *hp_next = next;
      
      // Check that the head does not have changed
      if(head != atomic_load_item(&queue->head))
         continue;
      
      // If there is no next to the head we return (this mean that we encountered the dummy node)
      if(!next)
      {
         *hp_cur = NULL;
         return NULL;
      }
      
      // If the head is also the tail we advance the pointer of the tail to 'next'
      if(head == tail)
      {
         atomic_compare_and_swap(&queue->tail, &tail, next);
         continue;
      }
      
      // Get the data and set the head to the next
      result = next->data;
      if(atomic_compare_and_swap(&queue->head, &head, next))
         break;
   }
   
   // Clean hazard pointers and delete the node
   *hp_cur = NULL;
   *hp_next = NULL;
   hp_delete_node(queue->hp, head);
   hp_unsubscribe(queue->hp);
   return result;
}

void free_queue(struct queue* queue)
{
   struct queue_item* next = queue->head;
   while(next)
   {
      struct queue_item* tmp = next->next;
      free(next);
      next = tmp;
   }
   free(queue);
}


/*          Private Functions              */

struct queue_item* new_queue_item(void* data)
{
   struct queue_item* result = chkmalloc(sizeof *result);
   result->next = NULL;
   result->data = data;
   return result;
}

void free_item(struct queue_item* item)
{
   free(item);
}
