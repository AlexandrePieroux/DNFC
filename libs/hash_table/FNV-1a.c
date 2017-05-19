#include "FNV-1a.h"


uint32_t FNV1a_hash(key_type value)
{
   uint32_t hash = FNV_offset;
   uint32_t octet_of_data = value;
   uint32_t mask = 255;
   uint32_t octet = 0;

   for(short i = 0; i < 4; i++)
   {
      octet = octet_of_data & mask;

      hash ^= octet;
      hash *= FNV_prime ;

      octet_of_data = octet_of_data >> 8;
   }
           
   return hash;
}
