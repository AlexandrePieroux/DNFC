#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>

extern "C"
{
  #include "../libs/hash_table/hash_table.h"
  #include "../libs/memory_management/memory_management.h"
  #include "../libs/thread_pool/thread_pool.h"
}

#include "gtest/gtest.h"

#define NB_NUMBERS      5000000
#define NB_THREADS      64

struct arguments_t
{
   uint32_t *numbers;
   uint32_t size;
   hash_table** table;
};



uint32_t* get_random_numbers(uint32_t size = 100);
void* job_insert(void* args);
void* job_get(void* args);
void* job_remove(void* args);
void init(arguments_t** &args);



TEST (HashTableTest, Insert)
{
  arguments_t** args;
  threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
     threadpool_add_work(pool, job_insert, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);
}

TEST (HashTableTest, Get)
{
  arguments_t** args;
  threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
     threadpool_add_work(pool, job_insert, args[i]);
  threadpool_wait(pool);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
     threadpool_add_work(pool, job_get, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);
}

TEST (HashTableTest, Remove)
{
  arguments_t** args;
  threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
     threadpool_add_work(pool, job_insert, args[i]);
  threadpool_wait(pool);

  for (uint32_t i = 0; i < NB_THREADS; ++i)
     threadpool_add_work(pool, job_remove, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);
}



int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



uint32_t* get_random_numbers(uint32_t size)
{
  uint32_t* result = new uint32_t[size];
  bool is_used[size];
  for (size_t i = 0; i < size; i++) {
    is_used[i] = false;
  }
  uint32_t im = 0;
  for (uint32_t in = 0; in < size && im < size; ++in) {
    uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
    if (is_used[r]) /* we already have 'r' */
      r = in; /* use 'in' instead of the generated number */

    result[im++] = r; /* +1 since your range begins from 1 */
    is_used[r] = true;
  }

  return result;
}



void init(arguments_t** &args)
{
  // Init phase
  srand(time(NULL));
  hash_table_init();
  uint32_t* numbers = get_random_numbers(NB_NUMBERS);

  // Preparing the re
  args = new arguments_t*[NB_THREADS];
  hash_table** table = new hash_table*;
  *table = new_hash_table(FNV_1);

  // We distribute the work per threads
  uint32_t divider = NB_NUMBERS / NB_THREADS;
  uint32_t remain = NB_NUMBERS % NB_THREADS;
  for (uint32_t p = 0; p < NB_THREADS; ++p)
  {
     args[p] = new arguments_t;
     args[p]->numbers = &(numbers[p * divider]);
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
  for (uint32_t i = 0; i < args_cast->size; ++i)
    EXPECT_TRUE(hash_table_put(*args_cast->table, args_cast->numbers[i], &args_cast->numbers[i]));
  return NULL;
}



void* job_get(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  for (uint32_t i = 0; i < args_cast->size; ++i){
    EXPECT_EQ(hash_table_get(*args_cast->table, args_cast->numbers[i]), &args_cast->numbers[i]);
    if(hash_table_get(*args_cast->table, args_cast->numbers[i]) != &args_cast->numbers[i])
      std::cout << "Failed to get number: " << args_cast->numbers[i] << std::endl;
  }
  return NULL;
}



void* job_remove(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
   for(uint32_t i = 0; i < args_cast->size; ++i){
      bool result = hash_table_remove(*args_cast->table, args_cast->numbers[i]);
      EXPECT_TRUE(result);
      if(!result)
        std::cout << "Tried to remove: " << args_cast->numbers[i] << '\n';
    }
    return NULL;
}
