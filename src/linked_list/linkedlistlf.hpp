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
public:
  LinkedListLf<K, H, D> *insert(const D &data, const K &key);
  LinkedListLf<K, H, D> *insert(const D &data, const K &key, const H &hash);
  LinkedListLf<K, H, D> *get(const K &key);
  LinkedListLf<K, H, D> *get(const K &key, const H &hash);
  bool remove(const K &key);
  bool remove(const K &key, const H &hash);
  LinkedListLf<K, H, D>(HazardPointer<LinkedListLf<K, H, D>> &hptr, const K &k, const H &h, const D &d) : hp(hptr), key(k), hash(h), data(d){};
  LinkedListLf<K, H, D>(const K &k, const H &h, const D &d) : hp(new HazardPointer<LinkedListLf<K, H, D>>(NB_HP)), key(k), hash(h), data(d){};
  ~LinkedListLf(){};

  std::atomic<LinkedListLf<K, H, D> *> *next;
  K key;
  H hash;
  D data;

private:
  inline LinkedListLf<K, H, D> *get_cur();
  inline LinkedListLf<K, H, D> *get_prev();
  inline LinkedListLf<K, H, D> *get_next();
  bool find(const K &key, const H &hash);
  LinkedListLf<K, H, D> *mark_pointer();
  uintptr_t get_mark();
  LinkedListLf<K, H, D> *get_clear_pointer();

  HazardPointer<LinkedListLf<K, H, D>> *hp;
};

#endif
