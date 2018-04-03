#include <cstdarg>
#include <chrono>
#include <stdio.h>
#include <iostream>
#include <vector>

extern "C"{
   #include "../../libs/thread_pool/thread_pool.h"
}

namespace Tools{
      template<typename ArgsType>
      class TestIterator{
            #define MAXNBITEMS      10000
            #define STEPSITEMS      10
            #define MAXNBTHREAD     8
            #define STEPSTHREADS    4 

            private:
                  int maxNbThreads;
                  int maxNbItems;
                  int stepsThreads;
                  int stepsItems;
                  std::vector<ArgsType> (init)(int);
                  void (work)(threadpool_t*, std::vector<ArgsType>);
                  void (callBack)(std::vector<ArgsType>);

            public:
                  TestIterator(
                        std::vector<ArgsType> (init)(int),
                        void (work)(threadpool_t*, std::vector<ArgsType>), 
                        void (callBack)(std::vector<ArgsType>)
                  );
                  TestIterator(
                        int maxNbThreads, 
                        int maxNbItems,
                        std::vector<ArgsType> (init)(int),
                        void (work)(threadpool_t*, std::vector<ArgsType>), 
                        void (callBack)(std::vector<ArgsType>)
                  );
                  TestIterator(
                        int maxNbThreads,
                        int maxNbItems,
                        int stepsThreads,
                        int stepsItems,
                        std::vector<ArgsType> (init)(int),
                        void (work)(threadpool_t*, std::vector<ArgsType>), 
                        void (callBack)(std::vector<ArgsType>)
                  );
                  void iterate(void);
      };
}
