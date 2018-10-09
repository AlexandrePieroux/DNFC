#ifndef __MURMUR3H__
#define __MURMUR3H__

#define _XOPEN_SOURCE

#include <cstdint>
#include <cstddef>

template <typename K>
uint32_t murmurhash3_new(const K &key)
{
   const unsigned char *data = reinterpret_cast<const unsigned char *>(std::addressof(key));
   const int nblocks = sizeof(K) / 4;
   int len = sizeof(K);

   uint32_t h1 = 2166136261;
   const uint32_t c1 = 0xcc9e2d51;
   const uint32_t c2 = 0x1b873593;

   uint32_t k1;
   const uint32_t *blocks = reinterpret_cast<const uint32_t *>(data + len);
   for (int i = -nblocks; i; i++)
   {
      k1 = blocks[i];
      k1 *= c1;
      k1 = rotl32(k1, 15);
      k1 *= c2;

      h1 ^= k1;
      h1 = rotl32(h1, 13);
      h1 = h1 * 5 + 0xe6546b64;
   }

   const unsigned char *tail = reinterpret_cast<const unsigned char *>(data + len);
   k1 = 0;

   switch (len & 3)
   {
   case 3:
      k1 ^= tail[2] << 16;
   case 2:
      k1 ^= tail[1] << 8;
   case 1:
      k1 ^= tail[0];
      k1 *= c1;
      k1 = rotl32(k1, 15);
      k1 *= c2;
      h1 ^= k1;
   };

   h1 ^= len;
   h1 = fmix32(h1);
   return h1;
}

static uint32_t rotl32(uint32_t x, const int8_t &r)
{
   return (x << r) | (x >> (32 - r));
}

static uint32_t fmix32(uint32_t h)
{
   h ^= h >> 16;
   h *= 0x85ebca6b;
   h ^= h >> 13;
   h *= 0xc2b2ae35;
   h ^= h >> 16;
   return h;
}

#endif
