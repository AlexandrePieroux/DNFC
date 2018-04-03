#define _BSD_SOURCE
#include <stdlib.h>
#include "gtest/gtest.h"
#include "tools/random_number_generator.h"
#include "tools/test_iterator.h"

extern "C"
{
  #include "../libs/linked_list/linked_list.h"
  #include "../libs/thread_pool/thread_pool.h"
}

#define MAXNBITEMS      10000
#define STEPSITEMS      10
#define MAXNBTHREAD     8
#define STEPSTHREADS    4 

struct arguments_t
{
  byte_stream* number;
  linked_list** table;
  struct hazard_pointer* hp;
};

std::vector<struct arguments_t*> init(uint32_t size);
void clean(std::vector<struct arguments_t*> items);

void* job_insert(void* args);
void* job_get(void* args);
void* job_remove(void* args);

TEST (LinkedListTest, Insert)
{
   Tools::TestIterator testIterator = new TestIterator::TestIterator(
      // Iteration parameters 
      MAXNBTHREAD,
      MAXNBITEMS,
      STEPSTHREADS,
      STEPSITEMS,

      /* Function parameters */
      // std::vector<arguments_t*> builder function
      &init,

      // Lambda work function that insert into a linked list
      [](threadpool_t* pool, std::vector<arguments_t*> args){
        for(auto const& a: args)
          threadpool_add_work(pool, &job_insert, a);
        threadpool_wait(pool);
      },

      // Cleaning callback function 
      &clean
   );
   testIterator.iterate();
}

TEST (LinkedListTest, Get)
{
   Tools::TestIterator testIterator = new TestIterator::TestIterator(
      // Iteration parameters 
      MAXNBTHREAD,
      MAXNBITEMS,
      STEPSTHREADS,
      STEPSITEMS,

      /* Function parameters */
      // std::vector<arguments_t*> builder function
      &init,

      // Lambda work function that insert into a linked list
      [](threadpool_t* pool, std::vector<arguments_t*> args){
        for(auto const& a: args)
          threadpool_add_work(pool, &job_insert, a);
        threadpool_wait(pool);

        for(auto const& a: args)
            threadpool_add_work(pool, &job_get, a);
          threadpool_wait(pool);
      },

      // Cleaning callback function 
      &clean
   );
   testIterator.iterate();
}

TEST (LinkedListTest, Remove)
{
   Tools::TestIterator testIterator = new TestIterator::TestIterator(
      // Iteration parameters 
      MAXNBTHREAD,
      MAXNBITEMS,
      STEPSTHREADS,
      STEPSITEMS,

      /* Function parameters */
      // std::vector<arguments_t*> builder function
      &init,

      // Lambda work function that insert into a linked list
      [](threadpool_t* pool, std::vector<arguments_t*> args){
        for(auto const& a: args)
          threadpool_add_work(pool, &job_insert, a);
        threadpool_wait(pool);

        for(auto const& a: args)
            threadpool_add_work(pool, &job_remove, a);
          threadpool_wait(pool);
      },

      // Cleaning callback function 
      &clean
   );
   testIterator.iterate();
}

TEST (LinkedListTest, ConcurrentUpdates)
{
   Tools::TestIterator testIterator = new TestIterator::TestIterator(
      // Iteration parameters 
      MAXNBTHREAD,
      MAXNBITEMS,
      STEPSTHREADS,
      STEPSITEMS,

      /* Function parameters */
      // std::vector<arguments_t*> builder function
      &init,

      // Lambda work function that insert into a linked list
      [](threadpool_t* pool, std::vector<arguments_t*> args){
        for(auto const& a: args){
            threadpool_add_work(pool, &job_insert, a);
            threadpool_add_work(pool, &job_get, a);
            threadpool_add_work(pool, &job_remove, a);
        }
        threadpool_wait(pool);
      },

      // Cleaning callback function 
      &clean
   );
   testIterator.iterate();
}



int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}



std::vector<arguments_t*> init(uint32_t size)
{
  // Get the nubers to work with
  Tools::RandomNumberGenerator numberGenerator = new Tools::RandomNumberGenerator;
  std::vector<byte_stream*> numbers = numberGenerator.getUniqueNumbers(size);

  // Create the result structure
  std::vector<arguments_t*> result = new std::vector<arguments_t*>;
  result.reserve(size);

  // Create the list and the hazard pointers to use with.
  struct hazard_pointer* hp = linked_list_init();
  linked_list** table = new linked_list*;
  *table = new_linked_list(0,0,NULL);

  // Build the result vector
  for(int i = 0; i < size; i++)
    arguments_t* args = new arguments_t;    
    args->table = table;
    args->hp = hp;
    args->number = numbers[i];
    result.push_back(args);
  }

  return result;
}

void clean(std::vector<struct arguments_t*> items)
{
  linked_list** table = items[0]->table;
  hazard_pointer* hp = items[0]->hp;

  for(auto const& i: items){
    free_byte_stream(i->number);
    free(i);
  }

  linked_list_free(table);
  free_hp(hp);
}



void* job_insert(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  linked_list* item = new_linked_list(args_cast->number, args_cast->number, &args_cast->number);
  linked_list* result = linked_list_insert(args_cast->hp, args_cast->table, item);
  if(args_cast->active_comparison)
    EXPECT_EQ(result, item);
  return NULL;
}



void* job_get(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  linked_list* item = linked_list_get(args_cast->hp, args_cast->table, args_cast->number, args_cast->number);
  if(args_cast->active_comparison)
    EXPECT_EQ(item->key, args_cast->number);
  return NULL;
}



void* job_remove(void* args)
{
  arguments_t* args_cast = (arguments_t*)args;
  bool result = linked_list_delete(args_cast->hp, args_cast->table, args_cast->number, args_cast->number);
  if(args_cast->active_comparison)
    EXPECT_TRUE(result);
  return NULL;
}
