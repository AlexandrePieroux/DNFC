#include "murmur3.h"


/*               Private function              */

// Return the ith 32 bit block
uint32_t getblock32 (const uint32_t * p, int i);

// Perform a left rotation of 'r' bit of 'x'
uint32_t rotl32(uint32_t x, int8_t r);

// Hash mix
uint32_t fmix32 (uint32_t h);

/*               Private function              */



uint32_t murmur_hash(uint32_t value)
{
  uint32_t h1 = MURMUR_seed;
  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;
  uint32_t k1 = value;

  k1 *= c1;
  k1 = rotl32(k1,15);
  k1 *= c2;
  h1 ^= k1;
  h1 = rotl32(h1,13);
  h1 = h1*5+0xe6546b64;
  h1 ^= 1;
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
