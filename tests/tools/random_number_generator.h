#include <cstdarg>
#include <iostream>
#include <time.h>
#include <vector>

extern "C"
{
   #include "../../libs/byte_stream/byte_stream.h"
}

namespace Tools{
   class RandomNumberGenerator{
      public:
         RandomNumberGenerator();
         std::vector<byte_stream*> getUniqueNumbers(uint32_t size);
         std::vector<byte_stream*> getRandomNumbers(uint32_t size);
   };
}