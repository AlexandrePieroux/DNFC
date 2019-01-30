#ifndef _HASH_TABLEH_
#define _HASH_TABLEH_

#include <cstdint>
#include <cstddef>
#include <iterator>
#include <boost/container_hash/hash.hpp>

#include "../linked_list/linkedlistlf.hpp"

// Lookup table for bits reversal
#define R2(n) n, n + 2 * 64, n + 1 * 64, n + 3 * 64
#define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#define R6(n) R4(n), R4(n + 2 * 4), R4(n + 1 * 4), R4(n + 3 * 4)
#define LOAD_FACTOR  0.75
#define INDEX_SIZE   32

static constexpr unsigned char bits_table[256] = {R6(0), R6(2), R6(1), R6(3)};

using namespace LLLF;

namespace HTLF
{
template <typename K, typename D>
class HashTableLf
{
    public:
      std::atomic<std::atomic<LinkedListLf<std::size_t, D> *> *> *index;
      std::atomic<unsigned int> size;
      std::atomic<unsigned int> nb_elements;
      boost::hash<K> hash;

      bool insert(const K &key, const D &value)
      {
            std::size_t pre_hash = this->hash(key);
            std::size_t hash = this->compress(pre_hash);
            auto bucket = this->get_bucket(hash);
            auto node = new LinkedListLf<std::size_t, D>(key, value, so_regular(pre_hash));
            auto result = bucket->insert(node);
            if (node != result)
            {
                  delete (node);
                  return false;
            }
            unsigned int cbsize = this->size.load(std::memory_order_relaxed);
            unsigned int csize = pow_2(cbsize + 1) - 1;
            unsigned int csizenext = csize + 1;
            if ((this->nb_elements++ / csize) > LOAD_FACTOR)
                  this->size.compare_exchange_strong(cbsize, csizenext,
                                                     std::memory_order_acquire, std::memory_order_relaxed);
            return true;
      }

      D get(const K &key)
      {
            std::size_t pre_hash = this->hash(key);
            std::size_t hash = this->compress(pre_hash);
            auto bucket = this->get_bucket(hash);
            auto result = bucket->get(key, so_regular(pre_hash));
            if (result)
                  return result->data;
            return 0;
      }

      bool remove(const K &key)
      {
            std::size_t pre_hash = this->hash(key);
            std::size_t hash = this->compress(pre_hash);
            auto bucket = this->get_bucket(hash);
            if (!bucket->remove(key, so_regular(pre_hash)))
                  return false;
            this->nb_elements--;
            return true;
      }

      HashTableLf<K, D>()
      {
            auto first_list = new LinkedListLf<std::size_t, D>;

            this->index = new std::atomic<std::atomic<LinkedListLf<std::size_t, D> *> *>[INDEX_SIZE];
            this->index[0] = new std::atomic<LinkedListLf<std::size_t, D> *>(first_list);

            this->size.store(1, std::memory_order_relaxed);
            this->nb_elements.store(0, std::memory_order_relaxed);
      };
      ~HashTableLf<K, D>(){};

    private:
      static unsigned int pow_2(unsigned int e)
      {
            unsigned int m = 0x1;
            return (m << e);
      }

      static std::size_t reverse_bits(std::size_t v)
      {
            std::size_t c = 0;
            unsigned char *p = (unsigned char *)&v;
            unsigned char *q = (unsigned char *)&c;
            q[3] = bits_table[p[0]];
            q[2] = bits_table[p[1]];
            q[1] = bits_table[p[2]];
            q[0] = bits_table[p[3]];
            return c;
      }

      static unsigned int get_segment_index(std::size_t v)
      {
            for (int i = 31; i >= 0; --i)
                  if (v & (unsigned int)(1 << i))
                        return (unsigned int)i;
            return 0;
      }

      static std::size_t so_regular(std::size_t v)
      {
            return (reverse_bits(v) | 0x1);
      }

      static std::size_t so_dummy(std::size_t v)
      {
            return (reverse_bits(v) & (std::size_t)~0x1);
      }

      static std::size_t get_parent(std::size_t v)
      {
            for (int i = 31; i >= 0; --i)
                  if (v & (std::size_t)(1 << i))
                        return v & (std::size_t) ~(1 << i);
            return 0;
      }

      std::size_t compress(std::size_t v)
      {
            std::size_t m = pow_2(this->size.load(std::memory_order_relaxed)) - 1;
            return (v & m);
      }

      LinkedListLf<std::size_t, D> *get_bucket(std::size_t hash)
      {
            unsigned int segment_index = get_segment_index(hash);
            unsigned int segment_size = pow_2(segment_index);
            if (!this->index[segment_index])
                  return this->init_bucket(hash);
            return this->index[segment_index][hash % segment_size].load(std::memory_order_relaxed);
      }

      void set_bucket(LinkedListLf<std::size_t, D> *head, std::size_t hash)
      {
            unsigned int segment_index = get_segment_index(hash);
            unsigned int segment_size = pow_2(segment_index);
            if (!this->index[segment_index])
            {
                  std::atomic<LinkedListLf<std::size_t, D> *> *expected = nullptr;
                  auto segment = new std::atomic<LinkedListLf<std::size_t, D> *>[segment_size];
                  if (!this->index[segment_index].compare_exchange_strong(expected, segment,
                                                                          std::memory_order_acquire, std::memory_order_relaxed))
                        delete[](segment);
            }
            this->index[segment_index][hash % segment_size].store(head, std::memory_order_relaxed);
      }

      LinkedListLf<std::size_t, D> *init_bucket(std::size_t hash)
      {
            std::size_t parent = get_parent(hash);
            auto bucket = this->get_bucket(parent);
            if (!bucket)
                  bucket = this->init_bucket(parent);
            std::size_t so_hash = so_dummy(hash);
            auto result = bucket->insert(so_hash, 0);
            this->set_bucket(result, hash);
            return result;
      }
};
} // namespace HTLF

#endif
