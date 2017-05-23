#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>

extern "C"
{
#include "../src/classifiers/dynamic_classifiers/flow_table/flow_table.h"
#include "../libs/memory_management/memory_management.h"
#include "../libs/thread_pool/thread_pool.h"
}

#include "gtest/gtest.h"

#define NB_NUMBERS      1000000
#define NB_THREADS      64
#define HEADER_LENGTH   16

struct arguments_t
{
   u_char* numbers;
   uint32_t* index;
   uint32_t size;
   hash_table** table;
};



uint32_t* get_random_numbers(uint32_t size);
void* job_insert(void* args);
void* job_get(void* args);
void* job_remove(void* args);
void init(arguments_t*** args);



TEST (FlowTable, Insert)
{
   arguments_t** args;
   threadpool_t* pool = new_threadpool(NB_THREADS);
   init(&args);
   
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, job_insert, args[i]);
   threadpool_wait(pool);
   free_threadpool(pool);
   
   hash_table_free(args[0]->table);
}

TEST (FlowTable, Get)
{
   arguments_t** args;
   threadpool_t* pool = new_threadpool(NB_THREADS);
   init(&args);
   
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, job_insert, args[i]);
   threadpool_wait(pool);
   
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, job_get, args[i]);
   threadpool_wait(pool);
   free_threadpool(pool);
   
   hash_table_free(args[0]->table);
}

TEST (FlowTable, Remove)
{
   arguments_t** args;
   threadpool_t* pool = new_threadpool(NB_THREADS);
   init(&args);
   
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, job_insert, args[i]);
   threadpool_wait(pool);
   
   for (uint32_t i = 0; i < NB_THREADS; ++i)
      threadpool_add_work(pool, job_remove, args[i]);
   threadpool_wait(pool);
   free_threadpool(pool);
   
   hash_table_free(args[0]->table);
}



int main(int argc, char **argv)
{
   srand(time(NULL));
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}



uint32_t *get_random_numbers(uint32_t size)
{
   /**
    Could be generalised to random over bigger range than 0 - 'size'
    **/
   uint32_t *result = new uint32_t[size];
   bool is_used[size];
   for (uint32_t i = 0; i < size; i++)
   {
      is_used[i] = false;
   }
   uint32_t im = 0;
   for (uint32_t in = 0; in < size && im < size; ++in)
   {
      uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
      if (is_used[r])                 /* we already have 'r' */
         r = in;                       /* use 'in' instead of the generated number */
      
      result[im++] = r; /* +1 since your range begins from 1 */
      is_used[r] = true;
   }
   return result;
}



void init(arguments_t*** args)
{
   // Init phase
   hash_table_init();
   uint32_t* numbers = get_random_numbers(NB_NUMBERS * 4);
   u_char** keys = new u_char*[NB_NUMBERS];
   uint32_t j = 0;
   for(uint32_t i = 0; i < NB_NUMBERS * 4; i+=4){
      key_type tmp = new_byte_stream();
      append_bytes(tmp, &(numbers[i]), 4);
      append_bytes(tmp, &(numbers[i+1]), 4);
      append_bytes(tmp, &(numbers[i+2]), 4);
      append_bytes(tmp, &(numbers[i+3]), 4);
      keys[j++] = tmp->stream;
   }
   
   // Preparing the structure
   *args = new arguments_t*[NB_THREADS];
   hash_table** table = new hash_table*;
   *table = new_hash_table(FNV_1);
   
   // We distribute the work per threads
   uint32_t divider = NB_NUMBERS / NB_THREADS;
   uint32_t remain = NB_NUMBERS % NB_THREADS;
   uint32_t* indexes = new uint32_t[NB_NUMBERS];
   for (uint32_t i = 0; i < NB_NUMBERS; i++)
      indexes[i] = i;
   
   for (uint32_t p = 0; p < NB_THREADS; ++p)
   {
      (*args)[p] = new arguments_t;
      (*args)[p]->numbers = keys[p * divider];
      (*args)[p]->index = &(indexes[p * divider]);
      
      if(p != (NB_THREADS - 1))
         (*args)[p]->size = divider;
      else
         (*args)[p]->size = divider + remain;
      (*args)[p]->table = table;
   }
}
              

              
void* job_insert(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for (uint32_t i = 0; i < args_cast->size; ++i)
      EXPECT_TRUE(put_flow(*args_cast->table, &args_cast->numbers[i], 128, &args_cast->numbers[i]));
   return NULL;
}



void* job_get(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for (uint32_t i = 0; i < args_cast->size; ++i){
      EXPECT_EQ(get_flow(*args_cast->table, &args_cast->numbers[i], 128), &args_cast->numbers[i]);
   }
   return NULL;
}



void* job_remove(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for(uint32_t i = 0; i < args_cast->size; ++i){
      bool result = remove_flow(*args_cast->table, &args_cast->numbers[i], 128);
      EXPECT_TRUE(result);
   }
   return NULL;
}
