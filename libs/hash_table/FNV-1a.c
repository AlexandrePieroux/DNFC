#include "FNV-1a.h"


uint32_t FNV1a_hash(key_type value)
{
   uint32_t hash = FNV_offset;
   uint8_t* octet_of_data = value->stream;

   for(uint32_t i = 0; i < value->size; i++)
   {
      hash ^= octet_of_data[i];
      hash *= FNV_prime ;
   }
           
   return hash;
}
