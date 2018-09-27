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
    // Hazard pointers
    this->hp->subscribe();
    std::atomic<LinkedListLf<K, H, D> *> *prevhp = this->hp->get_pointer(PREV);
    std::atomic<LinkedListLf<K, H, D> *> *curhp = this->hp->get_pointer(CUR);
    std::atomic<LinkedListLf<K, H, D> *> *nexthp = this->hp->get_pointer(NEXT);

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
      std::atomic<LinkedListLf<K, H, D> *> *nexttmp = cur->next;
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
    prevhp->store(nullptr, std::memory_order_relaxed);
    curhp->store(nullptr, std::memory_order_relaxed);
    nexthp->store(nullptr, std::memory_order_relaxed);

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
    std::atomic<LinkedListLf<K, H, D> *> prevhp = this->hp->get_pointer(PREV);
    std::atomic<LinkedListLf<K, H, D> *> curhp = this->hp->get_pointer(CUR);
    std::atomic<LinkedListLf<K, H, D> *> nexthp = this->hp->get_pointer(NEXT);

    // Private per thread variables
    LinkedListLf<K, H, D> *cur = this->get_cur();

    LinkedListLf<K, H, D> *result = nullptr;
    if (this->find(key, hash))
    {
      result = cur;
    }

    this->hp->unsubscribe();
    prevhp->store(nullptr, std::memory_order_relaxed);
    curhp->store(nullptr, std::memory_order_relaxed);
    nexthp->store(nullptr, std::memory_order_relaxed);

    return result;
  }

  bool remove(const K &key)
  {
    return this->delete_item(key, key);
  }

  bool remove(const K &key, const H &hash)
  {
    // Hazard pointers
    this->hp->subscribe();
    std::atomic<LinkedListLf<K, H, D> *> *prevhp = this->hp->get_pointer(PREV);
    std::atomic<LinkedListLf<K, H, D> *> *curhp = this->hp->get_pointer(CUR);
    std::atomic<LinkedListLf<K, H, D> *> *nexthp = this->hp->get_pointer(NEXT);

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
      std::atomic<LinkedListLf<K, H, D> *> *curnext = cur->next;

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
    prevhp->store(nullptr, std::memory_order_relaxed);
    curhp->store(nullptr, std::memory_order_relaxed);
    nexthp->store(nullptr, std::memory_order_relaxed);

    return result;
  }
  LinkedListLf<K, H, D>(HazardPointer<LinkedListLf<K, H, D>> &hptr, const K &k, const H &h, const D &d) : next(nullptr), key(k), hash(h), data(d), hp(hptr){};
  LinkedListLf<K, H, D>(const K &k, const H &h, const D &d) : next(nullptr), key(k), hash(h), data(d), hp(new HazardPointer<LinkedListLf<K, H, D>>(NB_HP)){};
  ~LinkedListLf(){};

  std::atomic<LinkedListLf<K, H, D> *> *next;
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
    // Hazard pointers
    std::atomic<LinkedListLf<K, H, D> *> *prevhp = this->hp->get_pointer(PREV);
    std::atomic<LinkedListLf<K, H, D> *> *curhp = this->hp->get_pointer(CUR);
    std::atomic<LinkedListLf<K, H, D> *> *nexthp = this->hp->get_pointer(NEXT);

    // Private per thread variables
    LinkedListLf<K, H, D> *cur = this->get_cur();
    LinkedListLf<K, H, D> *prev = this->get_prev();
    LinkedListLf<K, H, D> *next = this->get_next();

    for (;;)
    {
      prev = this->next->load(std::memory_order_relaxed);
      cur = prev->get_clear_pointer();
      curhp->store(cur, std::memory_order_relaxed);

      if (prev != cur)
        continue;

      for (;;)
      {
        if (!cur)
          return false;

        next = cur->next.load(std::memory_order_relaxed);
        nexthp->store(next->get_clear_pointer(), std::memory_order_relaxed);

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
          prevhp->store(cur, std::memory_order_relaxed);
        }
        else
        {
          if (prev->next->compare_exchange_strong(cur, next,
                                                  std::memory_order_acquire, std::memory_order_relaxed))
            this->hp->delete_node(cur);
          else
            break;
        }
        curhp->store(next, std::memory_order_relaxed);
        cur = curhp->load(std::memory_order_relaxed);
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
