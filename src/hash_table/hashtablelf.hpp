#ifndef _HASH_TABLEH_
#define _HASH_TABLEH_

#define _XOPEN_SOURCE

#include <cstdint>
#include <cstddef>
#include <iterator>

#include "../linked_list/linkedlistlf.hpp"
#include "murmur3.hpp"

// Lookup table for bits reversal
#define R2(n) n, n + 2 * 64, n + 1 * 64, n + 3 * 64
#define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#define R6(n) R4(n), R4(n + 2 * 4), R4(n + 1 * 4), R4(n + 3 * 4)
#define THRESHOLD 0.75

static constexpr unsigned char bits_table[256] = {R6(0), R6(2), R6(1), R6(3)};

template <typename K, typename D>
class HashTableLf
{
    public:
      std::atomic<std::atomic<LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *> *> *index;
      std::atomic<uint32_t> size;
      std::atomic<uint32_t> nb_elements;

      bool insert(const K &key, const D &value)
      {
            auto key_byte = to_bytes(key);
            uint32_t pre_hash = murmurhash3(*key_byte);
            uint32_t hash = this->compress(pre_hash);
            auto bucket = this->get_bucket(hash);
            if (!bucket)
                  bucket = this->init_bucket(hash);
            auto node = new LinkedListLf<std::vector<const unsigned char> *, uint32_t, D>(key_byte, so_regular(pre_hash), value);
            auto result = bucket->insert(node);
            if (node != result)
            {
                  delete (node);
                  return false;
            }
            uint32_t cbsize = this->size.load(std::memory_order_relaxed);
            uint32_t csize = pow_2(cbsize + 1) - 1;
            uint32_t csizenext = csize + 1;
            if ((this->nb_elements++ / csize) > THRESHOLD)
                  this->size.compare_exchange_strong(cbsize, csizenext,
                                                     std::memory_order_acquire, std::memory_order_relaxed);
            return true;
      }

      D get(const K &key)
      {
            D result_data;
            auto key_byte = to_bytes(key);
            uint32_t pre_hash = murmurhash3(*key_byte);
            uint32_t hash = this->compress(pre_hash);
            auto *bucket = this->get_bucket(hash);
            if (!bucket)
                  bucket = this->init_bucket(hash);
            auto result = bucket->get(key_byte, so_regular(pre_hash));
            if (result)
                  result_data = result->data;
            return result_data;
      }

      bool remove(const K &key)
      {
            auto key_byte = to_bytes(key);
            uint32_t pre_hash = murmurhash3(*key_byte);
            uint32_t hash = this->compress(pre_hash);
            auto bucket = this->get_bucket(hash);
            if (!bucket)
                  bucket = this->init_bucket(hash);
            if (!bucket->remove(key_byte, so_regular(pre_hash)))
                  return false;
            this->nb_elements--;
            return true;
      }

      HashTableLf<K, D>() : index(new std::atomic<std::atomic<LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *> *>[32])
      {
            auto frist_index = new std::vector<const unsigned char>(1, 0x0);
            auto first_item = new LinkedListLf<std::vector<const unsigned char> *, uint32_t, D>(frist_index, 0);
            this->index[0] = new std::atomic<LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *>(first_item);
            this->size.store(1, std::memory_order_relaxed);
            this->nb_elements.store(0, std::memory_order_relaxed);
      };
      ~HashTableLf<K, D>(){};

    private:
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

      static std::vector<const unsigned char> *to_bytes(const K &object)
      {
            const unsigned char *begin = reinterpret_cast<const unsigned char *>(std::addressof(object));
            const unsigned char *end = begin + sizeof(K);
            return new std::vector<const unsigned char>(begin, end);
      }

      uint32_t compress(uint32_t v)
      {
            uint32_t m = pow_2(this->size.load(std::memory_order_relaxed)) - 1;
            return (v & m);
      }

      LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *get_bucket(uint32_t hash)
      {
            uint32_t segment_index = get_segment_index(hash);
            uint32_t segment_size = pow_2(segment_index);
            if (!this->index[segment_index])
                  return NULL;
            return this->index[segment_index][hash % segment_size].load(std::memory_order_relaxed);
      }

      void set_bucket(LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *head, uint32_t hash)
      {
            uint32_t segment_index = get_segment_index(hash);
            uint32_t segment_size = pow_2(segment_index);
            if (!this->index[segment_index])
            {
                  std::atomic<LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *> *expected = nullptr;
                  auto segment = new std::atomic<LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *>[segment_size];
                  if (!this->index[segment_index].compare_exchange_strong(expected, segment,
                                                                          std::memory_order_acquire, std::memory_order_relaxed))
                        delete[](segment);
            }
            this->index[segment_index][hash % segment_size].store(head, std::memory_order_relaxed);
      }

      LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *init_bucket(uint32_t hash)
      {
            uint32_t parent = get_parent(hash);
            auto bucket = this->get_bucket(parent);
            if (!bucket)
                  bucket = this->init_bucket(parent);
            auto so_hash = so_dummy(hash);
            auto so_hash_byte = to_bytes(so_hash);
            auto dummy = new LinkedListLf<std::vector<const unsigned char> *, uint32_t, D>(so_hash_byte, so_hash);
            LinkedListLf<std::vector<const unsigned char> *, uint32_t, D> *result = bucket->insert(dummy);
            if (result != dummy)
            {
                  delete (dummy);
                  dummy = result;
            }
            this->set_bucket(dummy, hash);
            return dummy;
      }
};

#endif
