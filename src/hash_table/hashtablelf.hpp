#ifndef _HASH_TABLEH_
#define _HASH_TABLEH_

#define _XOPEN_SOURCE

#include <cstdint>
#include <cstddef>

#include "../linked_list/linkedlistlf.hpp"
#include "murmur3.hpp"

// Lookup table for bits reversal
#define R2(n) n, n + 2 * 64, n + 1 * 64, n + 3 * 64
#define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#define R6(n) R4(n), R4(n + 2 * 4), R4(n + 1 * 4), R4(n + 3 * 4)
#define THRESHOLD 0.75

template <typename K, typename D>
class HashTableLf
{
    public:
      std::atomic<std::atomic<LinkedListLf<K, uint32_t, D> *> *> *index;
      std::atomic<int> size;
      std::atomic<int> nb_elements;

      bool insert(const K &key, const D &value)
      {
            uint32_t pre_hash = murmurhash3(key);
            uint32_t hash = this->compress(pre_hash);
            LinkedListLf<K, uint32_t, D> *bucket = this->get_bucket(hash);
            if (!bucket)
                  bucket = this->init_bucket(hash);
            LinkedListLf<K, uint32_t, D> *node = new LinkedListLf<K, uint32_t, D>(bucket->hp, key, so_regular(pre_hash), value);
            LinkedListLf<K, uint32_t, D> *result = bucket->insert(node);
            if (node != result)
            {
                  delete (node);
                  return false;
            }
            uint32_t cbsize = this->size.load(std::memory_order_relaxed);
            uint32_t csize = pow_2(cbsize + 1) - 1;
            if ((this->nb_elements++ / csize) > THRESHOLD)
                  this->size.compare_exchange_strong(cbsize, cbsize + 1,
                                                     std::memory_order_acquire, std::memory_order_relaxed);
            return true;
      }

      D get(const K &key)
      {
            uint32_t pre_hash = murmurhash3(key);
            uint32_t hash = this->compress(pre_hash);
            LinkedListLf<K, uint32_t, D> *bucket = this->get_bucket(hash);
            if (!bucket)
                  bucket = thi->init_bucket(hash);
            LinkedListLf<K, uint32_t, D> *result = bucket->get(key, so_regular(pre_hash));
            if (result)
                  return result->data;
            else
                  return NULL;
      }

      bool remove(const K &key)
      {
            uint32_t pre_hash = murmurhash3(key);
            uint32_t hash = this->compress(pre_hash);
            LinkedListLf<K, uint32_t, D> *bucket = this->get_bucket(hash);
            if (!bucket)
                  bucket = this->init_bucket(hash);
            if (!bucket->remove(key, so_regular(pre_hash)))
                  return false;
            table->nb_elements--;
            return true;
      }

      HashTableLf<T>() : size(1), nb_elements(0), index(new std::atomic<std::atomic<LinkedListLf<K, uint32_t, D> *> *>[32])
      {
            this->index[0] = new std::atomic<LinkedListLf<K, uint32_t, D> *>(new LinkedListLf<K, uint32_t, D>(0, 0, nullptr));
      };
      ~HashTableLf<T>(){};

    private:
      static const unsigned char bits_table[256] = {R6(0), R6(2), R6(1), R6(3)};

      static uint32_t pow_2(uint32_t e)
      {
            uint32_t m = 0x1;
            return (m << e);
      }

      static uint32_t reverse_bits(uint32_t v)
      {
            uint32_t c = 0;
            unsigned char *p = (unsigned char *)&v;
            unsigned char *q = (unsigned char *)&c;
            q[3] = bits_table[p[0]];
            q[2] = bits_table[p[1]];
            q[1] = bits_table[p[2]];
            q[0] = bits_table[p[3]];
            return c;
      }

      static uint32_t get_segment_index(uint32_t v)
      {
            for (int i = 31; i >= 0; --i)
                  if (v & (uint32_t)(1 << i))
                        return (uint32_t)i;
            return 0;
      }

      static uint32_t so_regular(uint32_t v)
      {
            return (reverse_bits(v) | 0x1);
      }

      static uint32_t so_dummy(uint32_t v)
      {
            return (reverse_bits(v) & (uint32_t)~0x1);
      }

      static uint32_t get_parent(uint32_t v)
      {
            for (int i = 31; i >= 0; --i)
                  if (v & (uint32_t)(1 << i))
                        return v & (uint32_t) ~(1 << i);
            return 0;
      }

      uint32_t compress(uint32_t v)
      {
            uint32_t m = pow_2(this->size.load(std::memory_order_relaxed)) - 1;
            return (v & m);
      }

      LinkedListLf<K, uint32_t, D> *get_bucket(uint32_t hash)
      {
            uint32_t segment_index = get_segment_index(hash);
            uint32_t segment_size = pow_2(segment_index);
            if (!this->index[segment_index])
                  return NULL;
            return this->index[segment_index][hash % segment_size].load(std::memory_order_relaxed);
      }

      void set_bucket(LinkedListLf<K, uint32_t, D> *head, uint32_t hash)
      {
            uint32_t segment_index = get_segment_index(hash);
            uint32_t segment_size = pow_2(segment_index);
            if (!this->index[segment_index])
            {
                  std::atomic<LinkedListLf<K, uint32_t, D> *> *segment = new std::atomic<LinkedListLf<K, uint32_t, D> *>[segment_size];
                  if (!table->index[segment_index].compare_exchange_strong(nullptr, segment,
                                                                           std::memory_order_acquire, std::memory_order_relaxed)
                        delete (segment);
            }
            table->index[segment_index][hash % segment_size].store(head, std::memory_order_relaxed);
      }

      LinkedListLf<K, uint32_t, D> *init_bucket(uint32_t hash)
      {
            uint32_t parent = get_parent(hash);
            LinkedListLf<K, uint32_t, D> *bucket = this->get_bucket(parent);
            if (!bucket)
                  bucket = this->init_bucket(parent);
            uint32_t so_hash = so_dummy(hash);
            LinkedListLf<K, uint32_t, D> *dummy = new LinkedListLf<K, uint32_t, D>(reinterpret_cast<K>(so_hash), so_hash, nullptr); /* /!\  */
            LinkedListLf<K, uint32_t, D> *result = bucket->insert(dummy);
            if (result != dummy)
            {
                  delete (dummy);
                  dummy = result;
            }
            this->set_bucket(dummy, hash);
            return dummy;
      }
}

#endif
