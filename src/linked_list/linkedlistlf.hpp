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
  LinkedListLf<K, H, D> *insert(const D &data, const K &key)
  {
    return this->insert(data, key, key);
  }

  LinkedListLf<K, H, D> *insert(const D &data, const K &key, const H &hash)
  {
    this->hp->subscribe();

    // Private per thread variables
    LinkedListLf<K, H, D> *cur = this->get_cur();

    LinkedListLf<K, H, D> *result = nullptr;

    for (;;)
    {
      if (this->find(key, hash))
      {
        result = cur;
        break;
      }

      LinkedListLf<K, H, D> *item = new LinkedListLf<K, H, D>(this->hp, key, hash, data);
      LinkedListLf<K, H, D> *nexttmp = cur->next;
      LinkedListLf<K, H, D> *curcleared = cur->get_clear_pointer();
      item->next = curcleared;

      if (!nexttmp->compare_exchange_strong(curcleared, item,
                                            std::memory_order_acquire, std::memory_order_relaxed))
      {
        result = item;
        break;
      }
    }

    this->hp->unsubscribe();
    return result;
  }

  LinkedListLf<K, H, D> *get(const K &key)
  {
    return this->get(key, key);
  }

  LinkedListLf<K, H, D> *get(const K &key, const H &hash)
  {
    // Hazard pointers
    this->hp->subscribe();

    // Private per thread variables
    LinkedListLf<K, H, D> *cur = this->get_cur();

    LinkedListLf<K, H, D> *result = nullptr;
    if (this->find(key, hash))
    {
      result = cur;
    }

    this->hp->unsubscribe();
    return result;
  }

  bool remove(const K &key)
  {
    return this->remove(key, key);
  }

  bool remove(const K &key, const H &hash)
  {
    // Hazard pointers
    this->hp->subscribe();

    // Private per thread variables
    LinkedListLf<K, H, D> *cur = this->get_cur();
    LinkedListLf<K, H, D> *prev = this->get_prev();
    LinkedListLf<K, H, D> *next = this->get_next();

    bool result;

    for (;;)
    {
      if (!this->find(key, hash))
      {
        result = false;
        break;
      }

      LinkedListLf<K, H, D> *nextmarked = next->mark_pointer();
      LinkedListLf<K, H, D> *nextcleared = next->get_clear_pointer();
      LinkedListLf<K, H, D> *curnext = cur->next;

      if (!cur->next->compare_exchange_strong(nextcleared, nextmarked,
                                              std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      if (prev->next->compare_exchange_strong(cur, nextcleared,
                                              std::memory_order_acquire, std::memory_order_relaxed))
        this->hp->delete_node(cur);
      else
        this->find(key, hash);

      result = true;
      break;
    }

    this->hp->unsubscribe();
    return result;
  }

  LinkedListLf<K, H, D>(HazardPointer<LinkedListLf<K, H, D>> &hptr, const K &k, const H &h, const D &d) : next(nullptr), key(k), hash(h), data(d), hp(hptr){};
  LinkedListLf<K, H, D>(const K &k, const H &h, const D &d) : next(nullptr), key(k), hash(h), data(d), hp(new HazardPointer<LinkedListLf<K, H, D>>(NB_HP)){};
  ~LinkedListLf(){};

  LinkedListLf<K, H, D> *next;
  K key;
  H hash;
  D data;

private:
  inline LinkedListLf<K, H, D> *get_cur()
  {
    thread_local static LinkedListLf<K, H, D> *cur;
    return cur;
  }

  inline LinkedListLf<K, H, D> *get_prev()
  {
    thread_local static LinkedListLf<K, H, D> *prev;
    return prev;
  }

  inline LinkedListLf<K, H, D> *get_next()
  {
    thread_local static LinkedListLf<K, H, D> *next;
    return next;
  }

  bool find(const K &key, const H &hash)
  {
    // Private per thread variables
    LinkedListLf<K, H, D> *cur = this->get_cur();
    LinkedListLf<K, H, D> *prev = this->get_prev();
    LinkedListLf<K, H, D> *next = this->get_next();

    for (;;)
    {
      prev = this->next->load(std::memory_order_relaxed);
      cur = prev->get_clear_pointer();
      this->hp->store(CUR, cur);

      if (prev != cur)
        continue;

      for (;;)
      {
        if (!cur)
          return false;

        next = cur->next.load(std::memory_order_relaxed);
        this->hp->store(NEXT, next->get_clear_pointer());

        if (cur->next.load(std::memory_order_relaxed) != next)
          break;

        K ckey = cur->key;
        H chash = cur->hash;

        if (prev != cur->get_clear_pointer())
          break;

        if (!cur->get_mark())
        {
          if (chash > hash || (key == ckey && chash == hash))
            return (key == ckey && chash == hash);
          prev = cur->next.load(std::memory_order_relaxed);
          this->hp->store(PREV, cur);
        }
        else
        {
          if (prev->next->compare_exchange_strong(cur, next,
                                                  std::memory_order_acquire, std::memory_order_relaxed))
            this->hp->delete_node(cur);
          else
            break;
        }
        this->hp->store(CUR, next);
        cur = this->hp->load(CUR);
      }
    }
  }

  LinkedListLf<K, H, D> *mark_pointer()
  {
    return (LinkedListLf<K, H, D> *)(((uintptr_t)this) | 1);
  }

  uintptr_t get_mark()
  {
    return (uintptr_t)(this->next) & 0x1;
  }

  LinkedListLf<K, H, D> *get_clear_pointer()
  {
    return (LinkedListLf<K, H, D> *)(((uintptr_t)this) & ~((uintptr_t)0x1));
  }

  HazardPointer<LinkedListLf<K, H, D>> *hp;
};

#endif
