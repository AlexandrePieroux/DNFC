#include "linked_list.h"

#define atomic_compare_and_swap(t,old,new) __sync_bool_compare_and_swap (t, old, new)
#define atomic_load_list(p) ({struct linked_list* __tmp = *(p); __builtin_ia32_lfence (); __tmp;})
#define store_barrier __builtin_ia32_sfence


/*                                     Private function                                               */

// Retreive a thread context variable denoted by the key.
struct linked_list** get_var(pthread_key_t key);

// Retrieve the mark bit.
uintptr_t get_bit(struct linked_list* list);

// Mark the LSB of the address. This "tag" a node to be deleted.
struct linked_list* mark_pointer(struct linked_list* list, uintptr_t bit);

// Get the address of a tagged address.
struct linked_list*  get_list_pointer(struct linked_list* list);

// Delete a specified node.
void delete_node(struct linked_list* node, struct linked_list** pool);

// Create the key of the hasard pointers.
void make_keys(void);

/*                                     Private function                                               */




/*                                     Private per thread variables                                           */

static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t cur;
static pthread_key_t prev;

/*                                     Private per thread variables                                           */



void linked_list_init(void)
{
   pthread_once(&key_once, make_keys);
}



struct linked_list* new_linked_list(
  key_type key,
  hash_type hash,
  data_type data,
  struct linked_list** pool)
{
   struct linked_list* list = NULL;
   for(;;)
   {
      if(!(*pool))
      {
         list = chkmalloc(sizeof *list);
         break;
      } else {
         list = (*pool);
         if(atomic_compare_and_swap(&(*pool), list, list->next) && list)
            break;
      }
   }
   list->next = NULL;
   list->key = key;
   list->hash = hash;
   list->data = data;
   return list;
}



struct linked_list* linked_list_insert(
  struct linked_list** list,
  struct linked_list* item,
  struct linked_list** pool)
{
   key_type key = item->key;
   hash_type hash = item->hash;
   struct linked_list** cur_p = get_var(cur);
   struct linked_list** prev_p = get_var(prev);

   store_barrier();
   for(;;)
   {
      if(linked_list_find(list, key, hash, pool))
         return *(get_var(cur));
      item->next = *cur_p;
      if(atomic_compare_and_swap(&(*prev_p)->next, get_list_pointer(*cur_p), item))
         return item;
   }
}



struct linked_list* linked_list_get(
  struct linked_list** list,
  key_type key,
  hash_type hash,
  struct linked_list** pool)
{
   if(linked_list_find(list, key, hash, pool))
      return *(get_var(cur));
   return NULL;
}



bool linked_list_find(
  struct linked_list** list,
  key_type key,
  hash_type hash,
  struct linked_list** pool)
{
   struct linked_list** prev_p = get_var(prev);
   struct linked_list** cur_p = get_var(cur);

   for(;;)
   {
      *prev_p = *list;
      *cur_p = (*prev_p)->next;

      for(;;)
      {
         if(!get_list_pointer(*cur_p))
            return NULL;

         key_type ckey = (*cur_p)->key;
         hash_type chash = (*cur_p)->hash;
         struct linked_list* next = get_list_pointer(*cur_p)->next;

         if(atomic_load_list(&(*prev_p)->next) != get_list_pointer(*cur_p))
            break;

         if(!get_bit(next))
         {
            if(chash > hash || (ckey == key && chash == hash))
               return (ckey == key);
            *prev_p = *cur_p;
         } else {
            if(atomic_compare_and_swap(&(*prev_p)->next, get_list_pointer(*cur_p), get_list_pointer(next)))
               delete_node(get_list_pointer(*cur_p), pool);
            else
               break;
         }
         *cur_p = next;
      }
   }
}



bool linked_list_delete(struct linked_list** list,
  key_type key,
  hash_type hash,
  struct linked_list** pool)
{
   struct linked_list** prev_p = get_var(prev);
   struct linked_list** cur_p = get_var(cur);

   for(;;)
   {
      if(!linked_list_find(list, key, hash, pool))
         return false;

      struct linked_list* next = (*cur_p)->next;
      if(!atomic_compare_and_swap(&(*cur_p)->next, next, mark_pointer(next, 1)))
         continue;
      if(atomic_compare_and_swap(&(*prev_p)->next, get_list_pointer(*cur_p), get_list_pointer(next)))
         delete_node(get_list_pointer(*cur_p), pool);
      else
         linked_list_find(list, key, hash, pool);
      return true;
   }
}



void linked_list_free(struct linked_list** list)
{
   struct linked_list* cur_p = *list;
   struct linked_list* next = NULL;

   while(cur_p)
   {
      next = get_list_pointer(cur_p->next);
      free(cur_p);
      cur_p = next;
   }
}



/*                                     Private function                                               */

struct linked_list** get_var(pthread_key_t key)
{
   void** p = pthread_getspecific(key);
   if(!p)
   {
      p = chkcalloc(sizeof *p, 1);
      pthread_setspecific(key, p);
   }
   return (struct linked_list**) p;
}



uintptr_t get_bit(struct linked_list* list)
{
   return (uintptr_t) list & 0x1;
}



struct linked_list* mark_pointer(struct linked_list* list, uintptr_t bit)
{
   return (struct linked_list*) (((uintptr_t) list) | bit);
}



struct linked_list* get_list_pointer(struct linked_list* list)
{
   return (struct linked_list*) (((uintptr_t) list) & ~((uintptr_t)0x1));
}



void delete_node(struct linked_list* node, struct linked_list** pool)
{
   assert(get_bit(node) == 0);

   for(;;)
   {
      struct linked_list* head = *pool;
      node->next = head;
      if(atomic_compare_and_swap(&(*pool), head, node))
         break;
   }
}



void make_keys()
{
   pthread_key_create(&cur, NULL);
   pthread_key_create(&prev, NULL);
}

/*                                     Private function                                               */
