#include "random_number_generator.h"

namespace Tools{

   RandomNumberGenerator::RandomNumberGenerator(){
      srand(time(NULL));
   }

   std::vector<byte_stream*> RandomNumberGenerator::getUniqueNumbers(uint32_t size){
      std::vector<byte_stream*> result;
      result.reserve(size);
      bool isUsed[size];
      int32_t im = 0;

      for (uint32_t in = 0; in < size && im < size; ++in) {
         uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
         if (isUsed[r]) /* we already have 'r' */
            r = in; /* use 'in' instead of the generated number */

         result[im] = new_byte_stream();
         append_bytes(result[im], &r, 4); /* +1 since your range begins from 1 */
         isUsed[r] = 1;
         im++;
      }
      return result;
   }

   std::vector<byte_stream*> RandomNumberGenerator::getRandomNumbers(uint32_t size){
      std::vector<byte_stream*> result;
      result.reserve(size);
      for (uint32_t i = 0; i < size; i++) {
         result[i] = new_byte_stream();
         uint32_t r = rand();
         append_bytes(result[i], &r, 4);
      }
      return result;
   }

}
