#include "linked_list.h"

#define atomic_compare_and_swap(t,old,new) __atomic_compare_exchange_n(t, old, new, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
#define atomic_load_list(p) __atomic_load_n(&p, __ATOMIC_RELAXED)
#define store_barrier __atomic_thread_fence(__ATOMIC_RELAXED)

#define NB_HP         3

#define prev_hp_index 0
#define cur_hp_index  1
#define next_hp_index 2



/*                                     Private function                                               */

// Find a given node and set the hazard pointer on it and its predecesor
bool linked_list_find(
  struct hazard_pointer* hp,
  struct linked_list** list,
  key_type key,
  hash_type hash);

// Retreive a thread context variable denoted by the key.
struct linked_list** get_var(pthread_key_t key);

// Retreive a thread context variable denoted by the key 'prev'.
struct linked_list*** get_prev_var(pthread_key_t key);

// Retrieve the mark bit.
uintptr_t get_mark(struct linked_list* list);

// Compare the pointer and their marked bit
bool ptr_bit_equal(struct linked_list* a, struct linked_list* b);

// Mark the LSB of the address. This "tag" a node to be deleted.
struct linked_list* mark_pointer(struct linked_list* list, uintptr_t bit);

// Get the address of a tagged address.
struct linked_list*  get_clear_pointer(struct linked_list* list);

// Delete a specified node.
void delete_node(struct linked_list* node);

// Create the key of private variable per thread.
void make_keys(void);

/*                                     Private function                                               */



/*                                     Private per thread variables                                           */

static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static pthread_key_t cur;
static pthread_key_t prev;
static pthread_key_t next;

/*                                     Private per thread variables                                           */



struct hazard_pointer* linked_list_init(void)
{
   struct hazard_pointer* res = new_hazard_pointer((void(*)(void*))delete_node, NB_HP);
   pthread_once(&key_once, make_keys);
   return res;
}



struct linked_list* new_linked_list(
  key_type key,
  hash_type hash,
  data_type data)
{
   struct linked_list* list = chkmalloc(sizeof *list);
   list->next = NULL;
   list->key = key;
   list->hash = hash;
   list->data = data;
   return list;
}



struct linked_list* linked_list_insert(
  struct hazard_pointer* hp,
  struct linked_list** list,
  struct linked_list* item)
{
   key_type key = item->key;
   hash_type hash = item->hash;
   
   // Hazard pointers
   hp_subscribe(hp);
   struct linked_list** prev_hp = (struct linked_list**) hp_get(hp, prev_hp_index);
   struct linked_list** cur_hp = (struct linked_list**) hp_get(hp, cur_hp_index);
   struct linked_list** next_hp = (struct linked_list**) hp_get(hp, next_hp_index);
   
   // Private per thread variables
   struct linked_list** cur_p = get_var(cur);
   struct linked_list*** prev_p = get_prev_var(prev);
   struct linked_list* result;
   
   for(;;)
   {
      if(linked_list_find(hp, list, key, hash))
      {
         result = *cur_p;
         break;
      }
      
      struct linked_list* clear_cur_p = get_clear_pointer(*cur_p);
      item->next = clear_cur_p;
      
      if(atomic_compare_and_swap(*prev_p, &clear_cur_p, item))
      {
         result = item;
         break;
      }
   }
   
   hp_unsubscribe(hp);
   *prev_hp = NULL;
   *cur_hp = NULL;
   *next_hp = NULL;
   
   return result;
}



struct linked_list* linked_list_get(
  struct hazard_pointer* hp,
  struct linked_list** list,
  key_type key,
  hash_type hash)
{
   // Hazard pointers
   hp_subscribe(hp);
   struct linked_list** prev_hp = (struct linked_list**) hp_get(hp, prev_hp_index);
   struct linked_list** cur_hp = (struct linked_list**) hp_get(hp, cur_hp_index);
   struct linked_list** next_hp = (struct linked_list**) hp_get(hp, next_hp_index);
   
   // Private per thread variable
   struct linked_list** cur_p = get_var(cur);
   
   struct linked_list* result = NULL;
   if(linked_list_find(hp, list, key, hash))
   {
      result = get_clear_pointer(*cur_p);
   }
   
   hp_unsubscribe(hp);
   *prev_hp = NULL;
   *cur_hp = NULL;
   *next_hp = NULL;
   
   return result;
}



bool linked_list_delete(
  struct hazard_pointer* hp,
  struct linked_list** list,
  key_type key,
  hash_type hash)
{
   // Hazard pointers
   hp_subscribe(hp);
   struct linked_list** prev_hp = (struct linked_list**) hp_get(hp, prev_hp_index);
   struct linked_list** cur_hp = (struct linked_list**) hp_get(hp, cur_hp_index);
   struct linked_list** next_hp = (struct linked_list**) hp_get(hp, next_hp_index);
   
   // Private per thread variables
   struct linked_list*** prev_p = get_prev_var(prev);
   struct linked_list** cur_p = get_var(cur);
   struct linked_list** next_p = get_var(next);
   
   bool result;

   for(;;)
   {
      if(!linked_list_find(hp, list, key, hash))
      {
         result = false;
         break;
      }

      struct linked_list* next_cleared = get_clear_pointer(*next_p);
      if(!atomic_compare_and_swap(&(*cur_p)->next, &next_cleared, mark_pointer(*next_p, 1)))
         continue;
      
      struct linked_list* cur_p_list = get_clear_pointer(*cur_p);
      if(atomic_compare_and_swap(*prev_p, &cur_p_list, next_cleared))
         hp_delete_node(hp, *cur_p);
      else
         linked_list_find(hp, list, key, hash);
      
      result = true;
      break;
   }
   
   hp_unsubscribe(hp);
   *prev_hp = NULL;
   *cur_hp = NULL;
   *next_hp = NULL;
   
   return result;
}


// Not usable in concurent environements
void linked_list_free(struct linked_list** list)
{
   struct linked_list* cur_p = *list;
   struct linked_list* next = NULL;

   while(cur_p)
   {
      next = get_clear_pointer(cur_p->next);
      free_byte_stream(cur_p->key);
      delete_node(cur_p);
      cur_p = next;
   }
}



/*                                     Private function                                               */

bool linked_list_find(
  struct hazard_pointer* hp,
  struct linked_list** list,
  key_type key,
  hash_type hash)
{
   // Hazard pointers
   struct linked_list** prev_hp = (struct linked_list**) hp_get(hp, prev_hp_index);
   struct linked_list** cur_hp = (struct linked_list**) hp_get(hp, cur_hp_index);
   struct linked_list** next_hp = (struct linked_list**) hp_get(hp, next_hp_index);
   
   // Private per thread variables
   struct linked_list*** prev_p = get_prev_var(prev);
   struct linked_list** cur_p = get_var(cur);
   struct linked_list** next_p = get_var(next);
      
   for(;;)
   {
      *prev_p = &(*list)->next;
      *cur_p = get_clear_pointer(**prev_p);
      *cur_hp = *cur_p;
      
      if(**prev_p != *cur_p)
         continue;
      
      for(;;)
      {
         if(!(*cur_p))
            return NULL;
         
         *next_p = atomic_load_list((*cur_p)->next);
         *next_hp = get_clear_pointer(*next_p);
         
         if(atomic_load_list((*cur_p)->next) != *next_p)
            break;
         
         key_type ckey = (*cur_p)->key;
         hash_type chash = (*cur_p)->hash;
         
         if(**prev_p != get_clear_pointer(*cur_p))
            break;
         
         if(!get_mark(*cur_p))
         {
            bool eq = byte_stream_eq(ckey, key) && chash == hash;
            if(chash > hash || eq)
               return eq;
            *prev_p = &(*cur_p)->next;
            *prev_hp = *cur_p;
         } else {
            struct linked_list* cur_p_clean = get_clear_pointer(*cur_p);
            if(atomic_compare_and_swap(*prev_p, &cur_p_clean, get_clear_pointer(*next_p)))
               hp_delete_node(hp, *cur_p);
            else
               break;
         }
         *cur_hp = get_clear_pointer(*next_p);
         *cur_p = *cur_hp;
      }
   }
   return false; // Never happen
}



struct linked_list** get_var(pthread_key_t key)
{
   void** p = pthread_getspecific(key);
   if(!p)
   {
      p = chkcalloc(1, sizeof *p);
      pthread_setspecific(key, p);
   }
   return (struct linked_list**) p;
}

struct linked_list*** get_prev_var(pthread_key_t key)
{
   void** p = pthread_getspecific(key);
   if(!p)
   {
      p = chkcalloc(1, sizeof *p);
      pthread_setspecific(key, p);
      if(!(*p))
         *p = chkcalloc(1, sizeof **p);
   }
   return (struct linked_list***) p;
}



uintptr_t get_mark(struct linked_list* list)
{
   return (uintptr_t) (list->next) & 0x1;
}



struct linked_list* mark_pointer(struct linked_list* list, uintptr_t bit)
{
   return (struct linked_list*) (((uintptr_t) list) | bit);
}



struct linked_list* get_clear_pointer(struct linked_list* list)
{
   return (struct linked_list*) (((uintptr_t) list) & ~((uintptr_t)0x1));
}



void make_keys()
{
   pthread_key_create(&prev, NULL);
   pthread_key_create(&cur, NULL);
   pthread_key_create(&next, NULL);
}



void delete_node(struct linked_list* node)
{
   free(node);
}

/*                                     Private function                                               */
