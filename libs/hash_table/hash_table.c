  #include "hash_table.h"



  typedef uint32_t (*func_t)(uint32_t);


  /*                    Private macro                       */

  // Atomic compare and swap
  #define atomic_compare_and_swap(t,old,new) __sync_bool_compare_and_swap (t, old, new)

  // Atomicaly fetch and increase the value of a given number
  #define fetch_and_inc(n) __sync_fetch_and_add (n, 1)

  // Atomicaly fetch and decrease the value of a given number
  #define fetch_and_dec(n) __sync_fetch_and_sub (n, 1)

  // Lookup table for bits reversal
  #define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
  #define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
  #define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
  static const unsigned char bits_table[256] =
  {
      R6(0), R6(2), R6(1), R6(3)
  };

  /*                    Private macro                       */




  /*             Private functions           */

  // Power in base 2
  uint32_t pow_2(uint32_t e);

  // Compress the hash into a 2^l representation
  uint32_t compress(uint32_t v, uint32_t l);

  // Simple reduction function without hashing
  uint32_t mod_size_hash(uint32_t v);

  // Get the hash function acording to the specified type
  func_t get_hash_func(uint32_t type);

  // Get the bucket segment that contain the key
  struct linked_list* get_bucket(uint32_t hash, struct hash_table* table);

  // Set the bucket segment in the segment index
  void set_bucket(struct linked_list* head, uint32_t hash, struct hash_table* table);

  // Reverse bits order of a uint32_t value
  uint32_t reverse_bits(uint32_t v);

  // Switch order of bits of regular node
  uint32_t so_regular(uint32_t v);

  // Switch order of bits of dummy node
  uint32_t so_dummy(uint32_t v);

  // Initialize bucket
  struct linked_list* init_bucket(uint32_t hash, struct hash_table* table);

  // Get the parent bucket
  uint32_t get_parent (uint32_t v);

  // Retrieve the segment index of the hash table
  uint32_t get_segment_index(uint32_t v);

  /*             Private functions           */



  void hash_table_init(void)
  {
     linked_list_init();
  }



  struct hash_table* new_hash_table(uint32_t hash_type)
  {
     struct hash_table* result = chkmalloc(sizeof *result);
     result->index = chkcalloc(32, (sizeof result->index)); // this can be optimized (index length of 32 by default)
     result->pool = NULL;

     // First dummy node
     result->index[0] = chkcalloc(1, sizeof (struct linked_list*));
     result->index[0][0] = new_linked_list(0, 0, NULL, &result->pool);

     result->size = 1;
     result->nb_elements = 0;
     result->hash = get_hash_func(hash_type);
     return result;
  }



  bool hash_table_put(struct hash_table* table, uint32_t key, void* value)
  {
     uint32_t pre_hash = table->hash(key);
     uint32_t hash = compress(pre_hash, table->size);
     struct linked_list* node = new_linked_list(pre_hash, so_regular(pre_hash), value, &table->pool);
     struct linked_list* bucket = get_bucket(hash, table);
     if(!bucket)
        bucket = init_bucket(hash, table);
     struct linked_list* result = linked_list_insert(&bucket, node, &table->pool);
     if(result != node)
     {
        free(node);
        return false;
     }
     uint32_t csize = pow_2(table->size + 1) - 1;
     uint32_t cbsize = table->size;
     if((fetch_and_inc(&table->nb_elements) / csize) > THRESHOLD)
        atomic_compare_and_swap(&table->size, cbsize, cbsize + 1);
     return true;
  }



  void* hash_table_get(struct hash_table* table, uint32_t key)
  {
     uint32_t pre_hash = table->hash(key);
     uint32_t hash = compress(pre_hash, table->size);
     struct linked_list* bucket = get_bucket(hash, table);
     if(!bucket)
        bucket = init_bucket(hash, table);
     struct linked_list* result = linked_list_get(&bucket, pre_hash, so_regular(pre_hash), &table->pool);
     if(result)
      return result->data;
     else
      return NULL;
  }



  bool hash_table_remove(struct hash_table* table, uint32_t key)
  {
     uint32_t pre_hash = table->hash(key);
     uint32_t hash = compress(pre_hash, table->size);
     struct linked_list* bucket = get_bucket(hash, table);
     if(!bucket)
        bucket = init_bucket(hash, table);
     if(!linked_list_delete(&bucket, pre_hash, so_regular(pre_hash), &table->pool))
        return false;
     fetch_and_dec(&table->nb_elements);
     return true;
  }



  void hash_table_free(struct hash_table** table)
  {
     linked_list_free(*((*table)->index));
     linked_list_free(&(*table)->pool);

     for (uint32_t i = 0; i < (*table)->size; ++i)
     {
        if((*table)->index[i])
           free((*table)->index[i]);
     }
     free((*table)->index);
     free((*table));
  }



  /*                     Private functions                      */

  uint32_t pow_2(uint32_t e)
  {
     uint32_t m = 0x1;
     return (m << e);
  }



  uint32_t compress(uint32_t v, uint32_t l)
  {
     uint32_t m = pow_2(l) - 1;
     return (v & m);
  }



  uint32_t mod_size_hash(uint32_t v)
  {
     return v;
  }



  func_t get_hash_func(uint32_t type)
  {
     switch(type)
     {
        case FNV_1:
           return FNV1_hash;
        case MURMUR:
           return murmur_hash;
        default:
           return mod_size_hash;
     }
  }



  struct linked_list* get_bucket(uint32_t hash, struct hash_table* table)
  {
     uint32_t segment_index = get_segment_index(hash);
     uint32_t segment_size = pow_2(segment_index);
     if (!table->index[segment_index])
        return NULL;
     return table->index[segment_index][hash % segment_size];
  }



  void set_bucket(struct linked_list* head, uint32_t hash, struct hash_table* table)
  {
     uint32_t segment_index = get_segment_index(hash);
     uint32_t segment_size = pow_2(segment_index);
     if (!table->index[segment_index])
     {
       struct linked_list** segment = chkcalloc(segment_size, sizeof **segment);
       if(!atomic_compare_and_swap(&table->index[segment_index], NULL, segment))
           free(segment);
     }
     table->index[segment_index][hash % segment_size] = head;
  }



  uint32_t reverse_bits(uint32_t v)
  {
     uint32_t c = 0;
     unsigned char * p = (unsigned char *) &v;
     unsigned char * q = (unsigned char *) &c;
     q[3] = bits_table[p[0]];
     q[2] = bits_table[p[1]];
     q[1] = bits_table[p[2]];
     q[0] = bits_table[p[3]];
     return c;
  }



  uint32_t so_regular(uint32_t v)
  {
     return (reverse_bits(v) | 0x1);
  }



  uint32_t so_dummy(uint32_t v)
  {
     return (reverse_bits(v) & (uint32_t)~0x1);
  }



  struct linked_list* init_bucket(uint32_t hash, struct hash_table* table)
  {
     uint32_t parent = get_parent(hash);
     struct linked_list* bucket = get_bucket(parent, table);
     if(!bucket)
        bucket = init_bucket(parent, table);
     struct linked_list* dummy = new_linked_list(so_dummy(hash), so_dummy(hash), NULL, &table->pool);
     struct linked_list* result = linked_list_insert(&bucket, dummy, &table->pool);
     if (result != dummy)
     {
        free(dummy);
        dummy = result;
     }
     set_bucket(dummy, hash, table);
     return dummy;
  }



  uint32_t get_parent(uint32_t v)
  {
     for (int i = 31; i >= 0; --i)
     {
        if (v & (uint32_t)(1 << i))
           return v & (uint32_t) ~(1 << i);
     }
     return 0;
  }



  uint32_t get_segment_index(uint32_t v)
  {
     for (int i = 31; i >= 0; --i)
     {
        if (v & (uint32_t)(1 << i))
           return (uint32_t)i;
     }
     return 0;
  }

  /*                     Private functions                      */
