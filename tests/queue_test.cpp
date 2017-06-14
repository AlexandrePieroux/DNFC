#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>

extern "C"
{
#include "../libs/queue/queue.h"
#include "../libs/thread_pool/thread_pool.h"
}

#include "gtest/gtest.h"

#define NB_NUMBERS      1000000
#define NB_THREADS      65

struct arguments_t
{
   uint32_t* numbers;
   uint32_t* index;
   uint32_t size;
   struct queue** queue;
};



uint32_t* get_random_numbers(uint32_t size);
void* job_insert(void* args);
void* job_get(void* args);
void* job_remove(void* args);
void init(arguments_t** &args);



TEST (QueueTest, Insert)
{
   arguments_t** args;
   threadpool_t* pool = new_threadpool(NB_THREADS);
   init(args);
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, &job_insert, args[i]);
   threadpool_wait(pool);
   free_threadpool(pool);
   
   free_queue(*args[0]->queue);
}

TEST (QueueTest, Get)
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
   
   free_queue(*args[0]->queue);
}

TEST (QueueTest, Remove)
{
   arguments_t** args;
   threadpool_t* pool = new_threadpool(NB_THREADS);
   init(args);
   
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, &job_insert, args[i]);
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, &job_get, args[i]);
   threadpool_wait(pool);
   free_threadpool(pool);
   
   free_queue(*args[0]->queue);
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
   uint32_t im = 0;
   for (uint32_t in = 0; in < size && im < size; ++in) {
      uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
      if (is_used[r]) /* we already have 'r' */
         r = in; /* use 'in' instead of the generated number */
      
      is_used[r] = 1;
      im++;
   }
   return result;
}



void init(arguments_t** &args)
{
   // Init phase
   srand(time(NULL));
   uint32_t* numbers = get_random_numbers(NB_NUMBERS);
   
   // Preparing the structure
   args = new arguments_t*[NB_THREADS];
   struct queue** queue = new struct queue*;
   *queue = new_queue(NB_NUMBERS, NB_THREADS);
   
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
      args[p]->queue = queue;
   }
}



void* job_insert(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for (uint32_t i = 0; i < args_cast->size; ++i)
      queue_push(*args_cast->queue, &args_cast->numbers[i]);
   return NULL;
}



void* job_get(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for (uint32_t i = 0; i < args_cast->size; ++i)
      queue_pop(*args_cast->queue);
   return NULL;
}

