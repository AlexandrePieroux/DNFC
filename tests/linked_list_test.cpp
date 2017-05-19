#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>

extern "C"
{
  #include "../libs/linked_list/linked_list.h"
  #include "../libs/memory_management/memory_management.h"
  #include "../libs/thread_pool/thread_pool.h"
}

#include "gtest/gtest.h"

#define NB_NUMBERS      100000
#define NB_THREADS      5

struct arguments_t
{
  key_type* numbers;
  uint32_t* index;
  uint32_t size;
  linked_list** table;
};



key_type* get_random_numbers(uint32_t size);
void* job_insert(void* args);
void* job_get(void* args);
void* job_remove(void* args);
void init(arguments_t** &args);



TEST (LinkedListTest, Insert)
{
  arguments_t** args;
  threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);
  for (uint32_t i = 0; i < NB_THREADS; ++i)
    threadpool_add_work(pool, &job_insert, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);

  linked_list_free(args[0]->table);
}

TEST (LinkedListTest, Get)
{
  arguments_t** args;
  threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
    threadpool_add_work(pool, &job_insert, args[i]);
  threadpool_wait(pool);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
    threadpool_add_work(pool, &job_get, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);

  linked_list_free(args[0]->table);
}

TEST (LinkedListTest, Remove)
{
  arguments_t** args;
  threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
    threadpool_add_work(pool, &job_insert, args[i]);
  threadpool_wait(pool);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
    threadpool_add_work(pool, &job_remove, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);

  linked_list_free(args[0]->table);
}



int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



key_type* get_random_numbers(uint32_t size)
{
  key_type* result = new key_type[size];
  bool is_used[size];
  uint32_t im = 0;
  for (uint32_t in = 0; in < size && im < size; ++in) {
    uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
    if (is_used[r]) /* we already have 'r' */
      r = in; /* use 'in' instead of the generated number */

    result[im] = new_byte_stream();
    append_bytes(result[im], &r, 4); /* +1 since your range begins from 1 */
    is_used[r] = 1;
    im++;
  }
  return result;
}



void init(arguments_t** &args)
{
  // Init phase
  srand(time(NULL));
  linked_list_init();
  key_type* numbers = get_random_numbers(NB_NUMBERS);

  // Preparing the structure
  args = new arguments_t*[NB_THREADS];
  linked_list* pool = NULL;
  linked_list** table = new linked_list*;
  *table = new_linked_list(0,0,NULL, &pool);

  // We distribute the work per threads
  uint32_t divider = NB_NUMBERS / NB_THREADS;
  uint32_t remain = NB_NUMBERS % NB_THREADS;
  uint32_t* indexes = new uint32_t[NB_NUMBERS];
  for (uint32_t i = 0; i < NB_NUMBERS; i++)
   indexes[i] = i;
   
  for (uint32_t p = 0; p < NB_THREADS; ++p)
  {
    args[p] = new arguments_t;
    args[p]->numbers = &(numbers[p * divider]);
    args[p]->index = &(indexes[p * divider]);
     
    if(p != (NB_THREADS - 1))
      args[p]->size = divider;
    else
      args[p]->size = divider + remain;
    args[p]->table = table;
  }
}



void* job_insert(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  linked_list* pool = NULL;
  for (uint32_t i = 0; i < args_cast->size; ++i)
  {
    linked_list* item = new_linked_list(args_cast->numbers[i], args_cast->index[i], &args_cast->numbers[i], &pool);
    EXPECT_EQ(linked_list_insert(args_cast->table, item, &pool), item);
  }
  return NULL;
}



void* job_get(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  linked_list* pool = new linked_list;
  linked_list* item;
  for (uint32_t i = 0; i < args_cast->size; ++i)
  {
    item = linked_list_get(args_cast->table, args_cast->numbers[i], args_cast->index[i], &pool);
    if(item)
       EXPECT_EQ(item->key, args_cast->numbers[i]);
  }
  return NULL;
}



void* job_remove(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  linked_list* pool = NULL;
  for(uint32_t i = 0; i < args_cast->size; ++i)
     EXPECT_TRUE(linked_list_delete(args_cast->table, args_cast->numbers[i], args_cast->index[i], &pool));
  return NULL;
}
