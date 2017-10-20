#include "hazard_pointer.h"

#define atomic_compare_and_swap(t,old,new) __sync_bool_compare_and_swap (t, old, new)
#define atomic_load(p) ({uint32_t* __tmp = *(p); __builtin_ia32_lfence (); __tmp;})
#define batch_size(hp) ((hp->h * hp->nb_pointers * 2) + hp->h)


/*                Private function                     */

// Allocate a new hazard pointer record
struct hazard_pointer_record* new_hp_record(struct hazard_pointer* hp);

// Retreive the TLS variable denoted by 'my_hp_record.
struct hazard_pointer_record* get_my_hp_record(void);

// Subscribe to the structure to perform a task
uint32_t hp_subscribe(struct hazard_pointer* hp);

// Unubscribe to the structure after having performed a task
uint32_t hp_unsubscribe(struct hazard_pointer* hp);

// Create the key of the hasard pointers.
void hp_scan(struct hazard_pointer* hp);

// Helper scan function performed after a scan
void hp_help_scan(struct hazard_pointer* hp);

// Compare function for qs
int cmpfunc (const void* a, const void* b);

// Binary search
int binary_search(void** arr, int l, int r, int x);

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
   result->head = new_hp_record(result);

   pthread_once(&key_once, smr_make_keys);
   return result;
}



void** hp_get(struct hazard_pointer* hp, uint32_t index)
{
   struct hazard_pointer_record* my_hp_record = get_my_hp_record();
   return &my_hp_record->hp[index];
}



void hp_delete_node(struct hazard_pointer* hp, void* node)
{
   size_t batch_size = hp->nb_threads * hp->size * 2;
   void** dlist = get_dlist(hp, batch_size);
   uint32_t* dcount = get_dcount();
   dlist[*dcount] = node;
   (*dcount)++;
   if(*dcount == batch_size)
      hp_scan(hp);
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
   result->r_list = chkmalloc(sizeof(result->r_list));
   result->r_list->next = NULL;
   return result;
}

struct hazard_pointer_record* get_my_hp_record(void)
{
   void* hp_record = pthread_getspecific(my_hp_record);
   if(!hp_record)
   {
      hp_record = chkcalloc(1, sizeof *hp_record);
      pthread_setspecific(my_hp_record, hp_record);
   }
   return (struct hazard_pointer_record*) hp_record;
}

uint32_t* get_id(struct hazard_pointer* hp)
{
   void* id = pthread_getspecific(id_k);
   if(!id)
   {
      id = chkcalloc(1, sizeof *id);
      pthread_setspecific(id_k, id);
      *(uint32_t*)id = hp_subscribe(hp);
   }
   return (uint32_t*) id;
}

void** get_dlist(struct hazard_pointer* hp, size_t size)
{
   uint32_t* id = get_id(hp);
   void** dlist = hp->dlist[*id];
   if(!dlist)
   {
      void** dlist_p = chkcalloc(size, sizeof *dlist_p);
      hp->dlist[*id] = dlist_p;
   }
   return hp->dlist[*id];
}

uint32_t hp_subscribe(struct hazard_pointer* hp)
{
   // Subscribe and get an id if possible
   uint32_t id;
   uint32_t next_id;
   for(;;)
   {
      id = atomic_load(&hp->subscribed_threads);
      next_id = id + 1;
      if(id >= (uint32_t)atomic_load(&hp->nb_threads))
         return false;
      if(atomic_compare_and_swap(&hp->subscribed_threads, id, next_id))
         break;
   }
   return next_id;
}

uint32_t hp_unsubscribe(struct hazard_pointer* hp)
{
   
}
      
void hp_scan(struct hazard_pointer* hp)
{
   uint32_t p = 0;
   uint32_t new_dcount = 0;
   size_t nb_pointers = hp->nb_threads * hp->size;
   size_t batch_size = nb_pointers * 2;
   void** plist = chkcalloc(nb_pointers, sizeof(*plist));
   void** new_dlist = chkmalloc(sizeof(*new_dlist) * batch_size);
   
   void** dlist = get_dlist(hp, batch_size);
   uint32_t* dcount = get_dcount();
   
   // Stage 1
   for(uint32_t i = 0; i < nb_pointers; ++i)
   {
      if(hp->hp[i])
         plist[p++] = atomic_load(&hp->hp[i]);
   }
   
   // Stage 2
   qsort(plist, p, sizeof(void*), cmpfunc);
   
   // Stage 3
   for(uint32_t i = 0; i < batch_size; ++i)
   {
      if(binary_search(plist, 0, p, dlist[i]))
         new_dlist[new_dcount++] = dlist[i];
      else
         hp->free_node(dlist[i]);
   }
   
   // Stage 4
   for (uint32_t i = 0; i < new_dcount; ++i)
   {
      dlist[i] = new_dlist[i];
   }
   *dcount = new_dcount;
   
   free(plist);
   free(new_dlist);
}

void hp_help_scan(struct hazard_pointer* hp)
{
   
}

int cmpfunc (const void* a, const void* b)
{
   return (*(void**)a - *(void**)b);
}

int binary_search(void** arr, int l, int r, int x)
{
   if (r >= l)
   {
      int mid = l + (r - l)/2;
      
      // If the element is present at the middle itself
      if ((uint32_t)arr[mid] == x)
         return mid;
      
      // Search left
      if ((uint32_t)arr[mid] > x)
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
