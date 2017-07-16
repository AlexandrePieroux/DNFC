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
#define HEADER_BIT_L    320

struct arguments_t
{
   u_char** numbers;
   uint32_t* index;
   uint32_t size;
   hash_table** table;
};



uint32_t *get_random_numbers(uint32_t min, uint32_t max);
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
   
   for (uint32_t i = 0; i < NB_NUMBERS; ++i)
      put_flow(*(*args)->table, (*args)->numbers[i], &(*args)->numbers[i]);
   
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
   
   for (uint32_t i = 0; i < NB_NUMBERS; ++i)
      put_flow(*(*args)->table, (*args)->numbers[i], &(*args)->numbers[i]);
   
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



uint32_t *get_random_numbers(uint32_t min, uint32_t max)
{
   uint32_t size = max - min;
   uint32_t *result = new uint32_t[size];
   bool is_used[max];
   for (uint32_t i = 0; i < size; i++)
   {
      is_used[i] = false;
   }
   uint32_t im = 0;
   for (uint32_t in = min; in < max && im < size; ++in)
   {
      uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
      if (is_used[r])                 /* we already have 'r' */
         r = in;                      /* use 'in' instead of the generated number */
      
      result[im++] = r + min;
      is_used[r] = true;
   }
   return result;
}



void init(arguments_t*** args)
{
   // Get random unique flows
   uint32_t* source_address = get_random_numbers(0, NB_NUMBERS);
   uint32_t* destination_address = get_random_numbers(NB_NUMBERS, (NB_NUMBERS * 2));
   uint32_t* source_port = get_random_numbers(0, 65535);
   uint32_t* destination_port = get_random_numbers(0, 65535);
   
   u_char** keys = new u_char*[NB_NUMBERS];
   uint32_t j = 0;
   uint32_t* nb = new uint32_t;
   uint8_t zero = 0;
   uint8_t ihl = 5;
   uint8_t version = 4;
   uint8_t TCP_p = 6;
   u_short eth_type = 2048;
   for(uint32_t i = 0; i < NB_NUMBERS; i++){
      key_type tmp = new_byte_stream();
      
      // Ethernet type
      for(uint32_t j = 0; j < 12; j++)
         append_bytes(tmp, &zero, 1);
      append_bytes(tmp, &eth_type, 2);
      
      // IHL IPv4 field
      append_bytes(tmp, &ihl, 1);
      
      // Version IPv4 field
      append_bytes(tmp, &version, 1);
      
      // Put the offset of port (9 bytes)
      for(uint32_t j = 0; j < 7; j++)
         append_bytes(tmp, &zero, 1);
      
      // Put the TCP protocol number 
      append_bytes(tmp, &TCP_p, 1);
      
      // Put the offset of the addresses (2 bytes)
      for(uint32_t j = 0; j < 2; j++)
         append_bytes(tmp, &zero, 1);
      
      // Put the source address
      *nb = source_address[i];
      append_bytes(tmp, nb, 4);
      
      // Put the destination address
      *nb = destination_address[i];
      append_bytes(tmp, nb, 4);
      
      // Put the offset of the source ports (16 bytes)
      for(uint32_t j = 0; j < 16; j++)
         append_bytes(tmp, &zero, 1);
      
      // Put the source port
      *nb = source_port[(i % 65536)];
      append_bytes(tmp, nb, 2);
      
      // Put the destination port
      *nb = destination_port[(i % 65536)];
      append_bytes(tmp, nb, 2);
      
      // Get the uint8_t array that form the header
      keys[j++] = tmp->stream;
   }
   
   // Preparing the structure
   *args = new arguments_t*[NB_THREADS];
   hash_table** table = new hash_table*;
   *table = new_hash_table(FNV_1, NB_THREADS);
   
   // We distribute the work per threads
   uint32_t divider = NB_NUMBERS / NB_THREADS;
   uint32_t remain = NB_NUMBERS % NB_THREADS;
   uint32_t* indexes = new uint32_t[NB_NUMBERS];
   for (uint32_t i = 0; i < NB_NUMBERS; i++)
      indexes[i] = i;
   
   for (uint32_t p = 0; p < NB_THREADS; ++p)
   {
      (*args)[p] = new arguments_t;
      (*args)[p]->numbers = &keys[p * divider];
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
   for (uint32_t i = 0; i < args_cast->size; ++i){
      bool result = put_flow(*args_cast->table, args_cast->numbers[i], &args_cast->numbers[i]);
      EXPECT_TRUE(result);
   }
   return NULL;
}



void* job_get(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for (uint32_t i = 0; i < args_cast->size; ++i){
      if (args_cast->numbers[i])
         EXPECT_EQ(get_flow(*args_cast->table, args_cast->numbers[i]), &args_cast->numbers[i]);
   }
   return NULL;
}



void* job_remove(void* args)
{
   arguments_t* args_cast = (arguments_t*)args;
   for(uint32_t i = 0; i < args_cast->size; ++i){
      bool result = remove_flow(*args_cast->table, args_cast->numbers[i]);
      EXPECT_TRUE(result);
   }
   return NULL;
}
