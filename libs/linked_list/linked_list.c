#include "linked_list.h"

#define atomic_compare_and_swap(t,old,new) __sync_bool_compare_and_swap (t, old, new)
#define atomic_load_list(p) ({struct linked_list* __tmp = *(p); __builtin_ia32_lfence (); __tmp;})
#define store_barrier __builtin_ia32_sfence


/*                                     Private function                                               */

// Retrieve the mark bit.
uintptr_t get_bit(struct linked_list* list);

// Mark the LSB of the address. This "tag" a node to be deleted.
struct linked_list* mark_pointer(struct linked_list* list, uintptr_t bit);

// Get the address of a tagged address.
struct linked_list*  get_list_pointer(struct linked_list* list);

// Delete a specified node.
void delete_node(struct linked_list* node);

/*                                     Private function                                               */



struct linked_list* new_linked_list(
  key_type key,
  hash_type hash,
  data_type data)
{
   struct linked_list* list = NULL;
   list->next = NULL;
   list->key = key;
   list->hash = hash;
   list->data = data;
   return list;
}



struct linked_list* linked_list_insert(
  struct linked_list** list,
  struct linked_list* item)
{
   key_type key = item->key;
   hash_type hash = item->hash;
   struct linked_list** cur_p = get_var(cur); // TODO
   struct linked_list** prev_p = get_var(prev); // TODO

   store_barrier();
   for(;;)
   {
      if(linked_list_find(list, key, hash))
         return *(get_var(cur));
      item->next = *cur_p;
      if(atomic_compare_and_swap(&(*prev_p)->next, get_list_pointer(*cur_p), item))
         return item;
   }
}



struct linked_list* linked_list_get(
  struct linked_list** list,
  key_type key,
  hash_type hash)
{
   if(linked_list_find(list, key, hash))
      return *(get_var(cur)); // TODO
   return NULL;
}



bool linked_list_find(
  struct linked_list** list,
  key_type key,
  hash_type hash)
{
   struct linked_list** prev_p = get_var(prev); // TODO
   struct linked_list** cur_p = get_var(cur); // TODO

   for(;;)
   {
      *prev_p = *list;
      *cur_p = get_list_pointer((*prev_p)->next);

      for(;;)
      {
         if(!(*cur_p))
            return NULL;

         key_type ckey = (*cur_p)->key;
         hash_type chash = (*cur_p)->hash;
         struct linked_list* next = atomic_load_list(&(*cur_p)->next);

         if(atomic_load_list(&(*prev_p)->next) != *cur_p)
            break;

         if(!get_bit(next))
         {
            bool eq = byte_stream_eq(ckey, key);
            if(chash > hash || (eq && chash == hash))
               return eq;
            *prev_p = *cur_p;
         } else {
            if(atomic_compare_and_swap(&(*prev_p)->next, (*cur_p), get_list_pointer(next)))
               delete_node(get_list_pointer(*cur_p));
            else
               break;
         }
         *cur_p = next;
      }
   }
}



bool linked_list_delete(struct linked_list** list,
  key_type key,
  hash_type hash)
{
   struct linked_list** prev_p = get_var(prev); // TODO
   struct linked_list** cur_p = get_var(cur); // TODO

   for(;;)
   {
      if(!linked_list_find(list, key, hash))
         return false;

      struct linked_list* next = atomic_load_list(&(*cur_p)->next);
      if(!atomic_compare_and_swap(&(*cur_p)->next, next, mark_pointer(next, 1)))
         continue;
      if(atomic_compare_and_swap(&(*prev_p)->next, get_list_pointer(*cur_p), get_list_pointer(next)))
         delete_node(get_list_pointer(*cur_p));
      else
         linked_list_find(list, key, hash);
      return true;
   }
}


// Not usable in concurent environement
void linked_list_free(struct linked_list** list)
{
   struct linked_list* cur_p = *list;
   struct linked_list* next = NULL;

   while(cur_p)
   {
      next = get_list_pointer(cur_p->next);
      free_byte_stream(cur_p->key);
      free(cur_p);
      cur_p = next;
   }
}



/*                                     Private function                                               */

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



void delete_node(struct linked_list* node)
{
   free(node);
}

/*                                     Private function                                               */
