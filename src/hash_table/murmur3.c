#include "murmur3.h"


/*               Private function              */

// Return the ith 32 bit block
uint32_t getblock32(const uint32_t * p, int i);

// Perform a left rotation of 'r' bit of 'x'
uint32_t rotl32(uint32_t x, int8_t r);

// Hash mix
uint32_t fmix32 (uint32_t h);

/*               Private function              */


uint32_t murmur_hash(key_type key)
{
   const uint8_t * data = (const uint8_t*)key;
   const int nblocks = key->size / 4;
   int i;
   
   uint32_t h1 = MURMUR_seed;
   
   uint32_t c1 = 0xcc9e2d51;
   uint32_t c2 = 0x1b873593;
   
   //----------
   // body
   
   const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);
   
   for(i = -nblocks; i; i++)
   {
      uint32_t k1 = getblock32(blocks,i);
      
      k1 *= c1;
      k1 = rotl32(k1,15);
      k1 *= c2;
      
      h1 ^= k1;
      h1 = rotl32(h1,13);
      h1 = h1*5+0xe6546b64;
   }
   
   //----------
   // tail
   
   const uint8_t * tail = (const uint8_t*)(data + nblocks*4);
   
   uint32_t k1 = 0;
   
   switch(key->size & 3)
   {
      case 3: k1 ^= tail[2] << 16;
      case 2: k1 ^= tail[1] << 8;
      case 1: k1 ^= tail[0];
         k1 *= c1; k1 = rotl32(k1,15); k1 *= c2; h1 ^= k1;
   };
   
   //----------
   // finalization
   
   h1 ^= key->size;
   
   h1 = fmix32(h1);
   
   return h1;
}



/*               Private function              */

uint32_t getblock32 (const uint32_t * p, int i)
{
  return p[i];
}



uint32_t rotl32(uint32_t x, int8_t r)
{
  return (x << r) | (x >> (32 - r));
}



uint32_t fmix32 (uint32_t h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

/*               Private function              */
