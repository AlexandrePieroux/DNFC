/**
* Same as hash_table_test but needed for cmake
**/

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "../libs/hash_table/hash_table.h"
#include "../libs/memory_management/memory_management.h"
#include "../libs/thread_pool/thread_pool.h"

#include "gtest/gtest.h"

#define NB_NUMBERS      5000000
#define NB_THREADS      64


struct arguments_t
{
   uint32_t *numbers;
   uint32_t size;
   struct hash_table** table;
};


uint32_t* get_random_numbers(uint32_t size);
void job_insert(struct arguments_t* args);
void job_get(struct arguments_t* args);
void job_remove(struct arguments_t* args);
void init(struct arguments_t** args);



TEST (LinkedListTest, Insert)
{
  struct arguments_t** args;
  struct threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < nb_threads; ++i)
     threadpool_add_work(pool, job_insert, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);
}

TEST (LinkedListTest, Get)
{
  struct arguments_t** args;
  struct threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < nb_threads; ++i)
     threadpool_add_work(pool, job_insert, args[i]);
  threadpool_wait(pool);

  for (uint32_t i = 0; i < nb_threads; ++i)
     threadpool_add_work(pool, job_get, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);
}

TEST (LinkedListTest, Remove)
{
  struct arguments_t** args;
  struct threadpool_t* pool = new_threadpool(NB_THREADS);
  init(args);

  for (uint32_t i = 0; i < nb_threads; ++i)
     threadpool_add_work(pool, job_insert, args[i]);
  threadpool_wait(pool);

  for (uint32_t i = 0; i < nb_threads; ++i)
     threadpool_add_work(pool, job_remove, args[i]);
  threadpool_wait(pool);
  free_threadpool(pool);
}



int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



void init(struct arguments_t** args)
{

  // Init phase
  srand(time(NULL));
  hash_table_init();
  uint32_t* numbers = get_random_numbers(NB_NUMBERS);

  // Preparing the structure
  struct arguments_t** args = chkmalloc((sizeof *args) * NB_THREADS);
  struct hash_table* table = new_hash_table(MURMUR);

  // We distribute the work per threads
  uint32_t divider = NB_NUMBERS / NB_THREADS;
  uint32_t remain = NB_NUMBERS % NB_THREADS;
  for (uint32_t p = 0; p < NB_THREADS; ++p)
  {
     args[p] = chkmalloc(sizeof *args[p]);
     args[p]->numbers = &(numbers[p * divider]);
     if(p != (NB_THREADS - 1))
        args[p]->size = divider;
     else
        args[p]->size = divider + remain;
     args[p]->table = table;
  }
}



void job_insert(struct arguments_t* args)
{
  for (uint32_t i = 0; i < args->size; ++i)
    EXPECT_TRUE(hash_table_put(*(args->table), args->numbers[i], args->numbers[i]));
}



void job_get(struct arguments_t* args)
{
  for (uint32_t i = 0; i < args->size; ++i)
    EXPECT_EQ(hash_table_get(*(args->table), args->numbers[i]), args->numbers[i]);
}



void job_remove(struct arguments_t* args)
{
   for(uint32_t i = 0; i < args->size; ++i)
      EXPECT_TRUE(hash_table_remove(*(args->table), args->numbers[i]));
}
