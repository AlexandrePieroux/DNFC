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
            uint32_t pre_hash;
            std::array<byte> key_bytes = to_byte(key);
            murmurhash3(key_bytes, key_bytes.size(), &pre_hash);
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

      HashTableLf<T>() : size(1), nb_elements(0), index(new std::atomic<std::atomic<LinkedListLf<K, uint32_t, D> *> *>[32])
      {
            this->index[0] = new std::atomic<LinkedListLf<K, uint32_t, D> *>(new LinkedListLf<K, uint32_t, D>(0, 0, nullptr));
      };
      ~HashTableLf<T>(){};

    private:
      static const unsigned std::byte bits_table[256] = {R6(0), R6(2), R6(1), R6(3)};

      static K &from_word(const uint32_t hash)
      {
            K object;
            static_assert(std::is_trivially_copyable<K>::value, "not a TriviallyCopyable type");
            uint32_t *begin_object = reinterpret_cast<uint32_t *>(std::addressof(object));
            std::copy(std::begin(hash), std::end(hash), begin_object);
            return object;
      }

      static uint32_t pow_2(uint32_t e)
      {
            uint32_t m = 0x1;
            return (m << e);
      }

      static uint32_t compress(uint32_t v, uint32_t l)
      {
            uint32_t m = pow_2(l) - 1;
            return (v & m);
      }

      static uint32_t reverse_bits(uint32_t v)
      {
            uint32_t c = 0;
            std::byte *p = (std::byte *)&v;
            std::byte *q = (std::byte *)&c;
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
            LinkedListLf<K, uint32_t, D> *dummy = new LinkedListLf<K, uint32_t, D>(from_byte(so_hash), so_hash, nullptr);
            LinkedListLf<K, uint32_t, D> *result = bucket->(dummy);
            if (result != dummy)
            {
                  delete (dummy);
                  dummy = result;
            }
            set_bucket(dummy, hash, table);
            return dummy;
      }
}

bool
hash_table_put(struct hash_table *table, key_type key, void *value)
{
      uint32_t pre_hash = table->hash(key);
      uint32_t hash = compress(pre_hash, table->size);
      struct linked_list *node = new_linked_list(key, so_regular(pre_hash), value);
      struct linked_list *bucket = get_bucket(hash, table);
      if (!bucket)
            bucket = init_bucket(hash, table);
      struct linked_list *result = linked_list_insert(table->hp, &bucket, node);
      if (result != node)
      {
            free(node);
            return false;
      }
      uint32_t csize = pow_2(table->size + 1) - 1;
      uint32_t cbsize = table->size;
      if ((fetch_and_inc(&table->nb_elements) / csize) > THRESHOLD)
            atomic_compare_and_swap(&table->size, &cbsize, cbsize + 1);
      return true;
}

void *hash_table_get(struct hash_table *table, key_type key)
{
      uint32_t pre_hash = table->hash(key);
      uint32_t hash = compress(pre_hash, table->size);
      struct linked_list *bucket = get_bucket(hash, table);
      if (!bucket)
            bucket = init_bucket(hash, table);
      struct linked_list *result = linked_list_get(table->hp, &bucket, key, so_regular(pre_hash));
      if (result)
            return atomic_load_list(&result->data);
      else
            return NULL;
}

bool hash_table_remove(struct hash_table *table, key_type key)
{
      uint32_t pre_hash = table->hash(key);
      uint32_t hash = compress(pre_hash, table->size);
      struct linked_list *bucket = get_bucket(hash, table);
      if (!bucket)
            bucket = init_bucket(hash, table);
      if (!linked_list_delete(table->hp, &bucket, key, so_regular(pre_hash)))
            return false;
      fetch_and_dec(&table->nb_elements);
      return true;
}

void hash_table_free(struct hash_table **table)
{
      linked_list_free(*((*table)->index));

      for (uint32_t i = 0; i < (*table)->size; ++i)
      {
            if ((*table)->index[i])
                  free((*table)->index[i]);
      }
      free((*table)->index);
      free(*table);
}

/*                     Private functions                      */

/*                     Private functions                      */

#endif
