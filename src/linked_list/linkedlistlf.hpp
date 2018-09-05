#ifndef _LINKED_LISTH_
#define _LINKED_LISTH_

#include <cstdint>
#include "../SMR/hazardpointer.hpp"

#define NB_HP 3
#define PREV 0
#define CUR 1
#define NEXT 2

template <typename K, typename H, typename D>
class LinkedListLf
{
  std::atomic<LinkedListLf<K, H, D> *> *next;
  K key;
  H hash;
  D data;

public:
  LinkedListLf<K, H, D> *insert(const D *data, const K *key);
  LinkedListLf<K, H, D> *insert(const D *data, const K *key, const H *hash);
  LinkedListLf<K, H, D> *get(const K *key);
  LinkedListLf<K, H, D> *get(const K *key, const H *hash);
  bool delete_item(const K *key);
  bool delete_item(const K *key, const H *hash);

  LinkedListLf<K, H, D>(HazardPointer<LinkedListLf<K, H, D>> *hptr, const K *k, const H *h, const D *d) : hp(hptr), key(k), hash(h), data(d){};
  LinkedListLf<K, H, D>(const K *k, const H *h, const D *d) : hp(new HazardPointer<LinkedListLf<K, H, D>>(NB_HP)), key(k), hash(h), data(d){};
  ~LinkedListLf(){};

private:
  bool find(K key, H hash);
  LinkedListLf<K, H, D> *mark_pointer(uintptr_t bit);
  uintptr_t get_mark();
  LinkedListLf<K, H, D> *get_clear_pointer();

  HazardPointer<LinkedListLf<K, H, D>> *hp;
}

#endif
