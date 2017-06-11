#include "queue.h"

// Atomic actions macros
#define atomic_compare_and_swap(t,old,new) __sync_bool_compare_and_swap (t, old, new)
#define atomic_load_item(p) ({struct queue_item* __tmp = *(p); __builtin_ia32_lfence (); __tmp;})

#define fetch_and_inc(n) __sync_fetch_and_add (n, 1)
#define fetch_and_dec(n) __sync_fetch_and_sub (n, 1)

/*                                     Private function                                               */
// Instantiate a new queue item that contain 'data'
struct queue_item* new_queue_item(void* data)

// Delete a node (memory management)
void delete_node(struct queue_item* node);

// Retreive a thread context variable denoted by the key.
struct queue_item** get_var(pthread_key_t key);

// Create the key of the hasard pointers.
void make_keys(void);

/*                                     Private function                                               */


/*                                     Private per thread variables                                           */

static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t cur;
static pthread_key_t next;

/*                                     Private per thread variables                                           */



void queue_init(void)
{
   pthread_once(&key_once, make_keys);
}


/* Queue functions */

struct queue* new_queue(size_t capacity)
{
   struct queue* queue = chkmalloc(sizeof *queue);
   struct queue_item* dummy_node = new_queue_item(NULL);
   queue->size = 0;
   queue->max_size = capacity;
   queue->head = dummy_node;
   queue->tail = dummy_node;
   return queue;
}

bool queue_push(struct queue* queue, void* data)
{
   // We check and increment first: to ensure that we do not exceed the queue capacity
   if(queue->size >= queue->max_size)
      return false;
   else
      fetch_and_inc(&queue->size)
      
   // Get hazardous pointer
   struct queue_item** cur = get_var(cur);
   
   // Prepare new data
   struct queue_item* node = new_queue_item(data);
   
   // Till we succeed
   for(;;)
   {
      // Load tail and make the hazard point to it
      struct queue_item* tail = atomic_load_item(queue->tail);
      *cur = tail;
      
      // Check that the tail does not have changed
      if(tail != atomic_load_item(queue->tail))
         continue;
      
      // Read the next node of tail (should be null)
      struct queue_item* next = atomic_load_item(tail->next);
      
      // Check that the tail does not have changed
      if(tail != atomic_load_item(queue->tail))
         continue;
      
      // If next is not null we advance the pointer of the tail to the next node
      if(next)
      {
         atomic_compare_and_swap(&queue->tail, tail, next);
         continue;
      }
      
      // Try to effectively push the new node
      if(atomic_compare_and_swap(&tail->next, NULL, node))
         break;
   }
   // Set the tail to the new node
   atomic_compare_and_swap(&queue->tail, tail, node);
   *cur = NULL;
   return true;
}

void* queue_pop(struct queue* queue)
{
   // Check if there are elements in the queue before pop
   if(queue->size <= 0)
      return false;
   else
      fetch_and_dec(&queue->size)
      
   // Get hazardous pointers
   struct queue_item** hp_cur = get_var(cur);
   struct queue_item** hp_next = get_var(next);
      
   void* result;
   for(;;)
   {
      // Load the head of the queue and make the hazard pointer point to it
      struct queue_item* head = atomic_load_item(queue->head);
      *hp_cur = head;
      
      // Check that the head does not have changed
      if(head != atomic_load_item(queue->head))
         continue;
      
      // Load the tail and the next item of the head and make the hazard pointer point to it
      struct queue_item* tail = atomic_load_item(queue->tail);
      struct queue_item* next = atomic_load_item(head->next);
      *hp_next = next;
      
      // Check that the head does not have changed
      if(head != atomic_load_item(queue->head))
         continue;
      
      // If there is no next to the head we return (this that we encountered the dummy node)
      if(!next)
      {
         *hp_cur = NULL;
         return NULL;
      }
      
      // If the head is also the tail we advance the pointer of the tail to 'next'
      if(head == tail)
      {
         atomic_compare_and_swap(&queue->tail, tail, next);
         continue;
      }
      
      // Get the data and set the head to the next
      result = next->data;
      if(atomic_compare_and_swap(&queue->head, head, next)
         break;
   }
   
   // Clean hazard pointers and delete the node
   *hp_cur = NULL;
   *hp_next = NULL;
   delete_node(head);
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
}
         
void delete_node(struct queue_item* node)
{
   // TODO
   return;
}

struct queue_item** get_var(pthread_key_t key)
{
   void** p = pthread_getspecific(key);
   if(!p)
   {
      p = chkcalloc(sizeof *p, 1);
      pthread_setspecific(key, p);
   }
   return (struct queue_item**) p;
}

void make_keys()
{
   pthread_key_create(&cur, NULL);
   pthread_key_create(&next, NULL);
}
