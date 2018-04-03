#include "test_iterator.h"

namespace Tools{

   template<typename ArgsType>
   TestIterator::TestIterator(
      std::vector<ArgsType> (init)(int),
      void (work)(threadpool_t*, std::vector<ArgsType>), 
      void (callBack)(std::vector<ArgsType>)
   ){
      maxNbThreads = MAXNBTHREAD;
      maxNbItems = MAXNBITEMS;
      stepsThreads = STEPSTHREADS;
      stepsItems = STEPSITEMS;
   }

   template<typename ArgsType>
   TestIterator::TestIterator(
      int maxNbThreads, 
      int maxNbItems,
      std::vector<ArgsType> (init)(int),
      void (work)(threadpool_t*, std::vector<ArgsType>), 
      void (callBack)(std::vector<ArgsType>)
   ){
      maxNbThreads = maxNbThreads;
      maxNbItems = maxNbItems;
      stepsThreads = STEPSTHREADS;
      stepsItems = STEPSITEMS;
   }

   template<typename ArgsType>
   TestIterator::TestIterator(
      int maxNbThreads,
      int maxNbItems,
      int stepsThreads,
      int stepsItems,
      std::vector<ArgsType> (init)(int),
      void (work)(threadpool_t*, std::vector<ArgsType>), 
      void (callBack)(std::vector<ArgsType> args)
   ){
      maxNbThreads = maxNbThreads;
      maxNbItems = maxNbItems;
      stepsThreads = stepsThreads;
      stepsItems = stepsItems;
   }

   template<typename ArgsTypej>
   void TestIterator::iterate(void){      
      for(int nbThreads = this->stepsThreads; nbThreads <= this->maxNbThreads; nbThreads+=this->stepsThreads){
         threadpool_t* pool = new_threadpool(nbThreads);
         for(int nbItems = this->stepsItems; nbItems <= this->maxNbItems; nbItems+=this->stepsItems){

            // Initialization phase
            std::cout << "[ ITERATION] " << "Threads: " << nbThreads << " - Random numbers: " << nbItems;
            std::vector<ArgsType> items = init(nbItems);

            // We distribute the work per threads
            uint32_t divider = nbItems / nbThreads;    
            for (uint32_t i = 0; i < nbItems; i += divider)
            {
               auto last = std::min(items.size(), i + divider);
               std::vector<ArgsType> args = new std::vector<ArgsType>;
               args.reserve(last - i);
               move(items.begin() + i, items.begin() + last, back_inserter(args));

               // Perform the work with the thread share of work
               auto start = std::chrono::high_resolution_clock::now();
               work(pool, args);
               auto end = std::chrono::high_resolution_clock::now();
               std::chrono::duration<double> elapsed = end - start;
            }
            
            // Callback for cleaning structures
            callBack(item);
            std::cout << " - " << elapsed.count() << " s" << std::endl;
         }
         free_threadpool(pool);
      }
   }
}