#include "hazard_pointer.h"

#define atomic_tas(t) __atomic_test_and_set(t, __ATOMIC_RELAXED)
#define fetch_and_add(n, v) __atomic_fetch_add(n, v, __ATOMIC_RELAXED)
#define atomic_compare_and_swap(t,old,new) __atomic_compare_exchange_n(t, old, new, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)
#define atomic_load(p) __atomic_load_n(p, __ATOMIC_RELAXED)

#define batch_size(hp) ((atomic_load(&hp->h) * hp->nb_pointers * 2) + atomic_load(&hp->h))


/*                Private function                     */

// Allocate a new hazard pointer record
struct hazard_pointer_record* new_hp_record(struct hazard_pointer* hp);

// Retreive the TLS variable denoted by 'my_hp_record.
struct hazard_pointer_record** get_my_hp_record(void);

// Push a node on the list of removable node in the record
void hp_push(struct hazard_pointer_record* record, void* node);

// Create the key of the hasard pointers.
void hp_scan(struct hazard_pointer* hp);

// Helper scan function performed after a scan
void hp_help_scan(struct hazard_pointer* hp);

// Compare function for qs
int cmpfunc (const void* a, const void* b);

// Binary search
int binary_search(struct node_ll** arr, int l, int r, int x);

// Create pthread keys
void smr_make_keys(void);

/*                Private function                     */



static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t my_hp_record;



struct hazard_pointer* new_hazard_pointer(void (*free_node)(void*), uint32_t nb_pointers)
{
   // Allocate the structure and the basic data
   struct hazard_pointer* result = chkmalloc(sizeof(*result));
   result->free_node = free_node;
   result->nb_pointers = nb_pointers;
   result->h = 0;
   result->head = NULL;

   pthread_once(&key_once, smr_make_keys);
   return result;
}



void hp_subscribe(struct hazard_pointer* hp)
{
   // Retrive TLS variable 'my_hp_record'
   struct hazard_pointer_record** my_hp_record = get_my_hp_record();
   
   // First try to reuse a retire HP record
   for(struct hazard_pointer_record* i = hp->head ; i; i = i->next)
   {
      if(i->active)
         continue;
      if(atomic_tas(&i->active))
         continue;
      
      // Sucessfully locked one record to use
      *my_hp_record = i;
      return;
   }
   
   // No HP records available for reuse
   while(!fetch_and_add(&hp->h, hp->nb_pointers));
   
   // Allocate and push a new record
   *my_hp_record = new_hp_record(hp);
   (*my_hp_record)->active = true;
   struct hazard_pointer_record* oldhead = atomic_load(&hp->head);

   // Add the new record to the list
   for(;;)
   {
      (*my_hp_record)->next = hp->head;
      if(atomic_compare_and_swap(&hp->head, &oldhead, *my_hp_record))
         break;
      oldhead = atomic_load(&hp->head);
   }
}



void hp_unsubscribe(struct hazard_pointer* hp)
{
   struct hazard_pointer_record** my_hp_record = get_my_hp_record();
   for(uint32_t i = 0; i < hp->nb_pointers; i++)
   {
      (*my_hp_record)->hp[i] = NULL;
   }
   (*my_hp_record)->active = false;
}



void** hp_get(struct hazard_pointer* hp, uint32_t index)
{
   struct hazard_pointer_record** my_hp_record = get_my_hp_record();
   return &(*my_hp_record)->hp[index];
}



void hp_delete_node(struct hazard_pointer* hp, void* node)
{
   struct hazard_pointer_record** my_hp_record = get_my_hp_record();
   hp_push(*my_hp_record, node);
   
   if((*my_hp_record)->r_count >= batch_size(hp))
   {
      hp_scan(hp);
      hp_help_scan(hp);
   }
}



// Be sure that the nodes contained in the pointers are all freed
void free_hp(struct hazard_pointer* hp)
{
   struct hazard_pointer_record* tmp = hp->head;
   while(tmp)
   {
      struct node_ll* node_tmp = tmp->r_list;
      while(node_tmp)
      {
         hp->free_node(node_tmp->data);
         struct node_ll* next_tmp = node_tmp->next;
         free(node_tmp);
         node_tmp = next_tmp;
      }
      struct hazard_pointer_record* record_tmp = tmp->next;
      free(tmp);
      tmp = record_tmp;
   }
   free(hp);
}



/*                Private function                     */

struct hazard_pointer_record* new_hp_record(struct hazard_pointer* hp)
{
   struct hazard_pointer_record* result = chkmalloc(sizeof(*result));
   result->hp = chkmalloc(sizeof(result->hp) * hp->nb_pointers);
   result->r_list = NULL;
   result->r_count = 0;
   return result;
}

struct hazard_pointer_record** get_my_hp_record(void)
{
   void** hp_record = pthread_getspecific(my_hp_record);
   if(!hp_record)
   {
      hp_record = chkcalloc(1, sizeof *hp_record);
      pthread_setspecific(my_hp_record, hp_record);
   }
   return (struct hazard_pointer_record**) hp_record;
}

void hp_push(struct hazard_pointer_record* record, void* node)
{
   struct node_ll* deleted_node = chkmalloc(sizeof(*deleted_node));
   deleted_node->data = node;
   deleted_node->next = record->r_list;
   record->r_list = deleted_node;
   record->r_count++;
}
      
void hp_scan(struct hazard_pointer* hp)
{
   struct hazard_pointer_record** my_hp_record = get_my_hp_record();
   uint32_t new_dcount = 0;
   size_t batch_size = batch_size(hp);
   struct node_ll** plist = chkcalloc(atomic_load(&hp->h), sizeof(*plist));
   
   // Retrieve the dlist of the record
   struct node_ll** dlist = chkmalloc(sizeof(*dlist) * (*my_hp_record)->r_count);
   uint32_t dlist_count = 0;
   for(struct node_ll* i = (*my_hp_record)->r_list; i; i = i->next)
      dlist[dlist_count++] = i;
   (*my_hp_record)->r_list = NULL;
   
   // Stage 1
   uint32_t p = 0;
   for(struct hazard_pointer_record* i = atomic_load(&hp->head); i; i = i->next)
   {
      if(i->active)
      {
         for(uint32_t j = 0; j < hp->nb_pointers; j++)
         {
            void* hp = atomic_load(&i->hp[j]);
            if(hp)
               plist[p++] = hp;
         }
      }
   }
   
   // Stage 2
   qsort(plist, p, sizeof(struct node_ll*), cmpfunc);
   
   // Stage 3
   struct node_ll* tmp;
   for(uint32_t i = 0; i < batch_size; ++i)
   {
      if(binary_search(plist, 0, p - 1, dlist[i]->data))
      {
         tmp = (*my_hp_record)->r_list;
         (*my_hp_record)->r_list = dlist[i];
         dlist[i]->next = tmp;
      } else {
         hp->free_node(dlist[i]->data);
         free(dlist[i]);
      }
   }
   
   // Stage 4
   (*my_hp_record)->r_count = new_dcount;
   
   free(plist);
   free(dlist);
}

void hp_help_scan(struct hazard_pointer* hp)
{
   struct hazard_pointer_record** my_hp_record = get_my_hp_record();
   for (struct hazard_pointer_record* i = hp->head; i; i = i->next)
   {
      // Trying to lock the next non-used hazard pointer record
      if(i->active)
         continue;
      if(atomic_tas(&i->active))
         continue;
      
      // Inserting the r_list of the node in my_hp_record
      for (struct node_ll* j = i->r_list; j; j = i->r_list)
      {
         i->r_list = j->next;
         j->next = (*my_hp_record)->r_list;
         (*my_hp_record)->r_list = j;
         
         i->r_count--;
         (*my_hp_record)->r_count++;
         
         // Scan if we reached the threshold
         if((*my_hp_record)->r_count >= batch_size(hp))
            hp_scan(hp);
      }
      // Release the record
      i->active = false;
   }
}

int cmpfunc (const void* a, const void* b)
{
   return (((struct node_ll*)a)->data - ((struct node_ll*)b)->data);
}

int binary_search(struct node_ll** arr, int l, int r, int x)
{
   if (r >= l)
   {
      int mid = l + (r - l)/2;
      
      // If the element is present in the middle
      if ((uint32_t)arr[mid]->data == x)
         return mid;
      
      // Search left
      if ((uint32_t)arr[mid]->data > x)
         return binary_search(arr, l, mid - 1, x);
      
      // Else search right
      return binary_search(arr, mid + 1, r, x);
   }
   // Not found
   return NULL;
}

void smr_make_keys(void)
{
   pthread_key_create(&my_hp_record, NULL);
}

/*                Private function                     */
