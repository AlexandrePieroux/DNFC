#include "linkedlistlf.hpp"

template <typename K, typename H, typename D>
LinkedListLf<K, H, D> *LinkedListLf<K, H, D>::insert(const D &data, const K &key)
{
  return this->insert(data, key, key);
}

template <typename K, typename H, typename D>
LinkedListLf<K, H, D> *LinkedListLf<K, H, D>::insert(const D &data, const K &key, const H &hash)
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

template <typename K, typename H, typename D>
LinkedListLf<K, H, D> *LinkedListLf<K, H, D>::get(const K &key)
{
  return this->get(key, key);
}

template <typename K, typename H, typename D>
LinkedListLf<K, H, D> *LinkedListLf<K, H, D>::get(const K &key, const H &hash)
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

template <typename K, typename H, typename D>
bool LinkedListLf<K, H, D>::remove(const K &key)
{
  return this->delete_item(key, key);
}

template <typename K, typename H, typename D>
bool LinkedListLf<K, H, D>::remove(const K &key, const H &hash)
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

/*                                     Private function                                               */

template <typename K, typename H, typename D>
inline LinkedListLf<K, H, D> *get_cur()
{
  thread_local static LinkedListLf<K, H, D> *cur;
  return cur;
}

template <typename K, typename H, typename D>
inline LinkedListLf<K, H, D> *get_prev()
{
  thread_local static LinkedListLf<K, H, D> *prev;
  return prev;
}

template <typename K, typename H, typename D>
inline LinkedListLf<K, H, D> *get_next()
{
  thread_local static LinkedListLf<K, H, D> *next;
  return next;
}

template <typename K, typename H, typename D>
bool LinkedListLf<K, H, D>::find(const K &key, const H &hash)
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

template <typename K, typename H, typename D>
uintptr_t LinkedListLf<K, H, D>::get_mark()
{
  return (uintptr_t)(this->next) & 0x1;
}

template <typename K, typename H, typename D>
LinkedListLf<K, H, D> *LinkedListLf<K, H, D>::mark_pointer()
{
  return (LinkedListLf<K, H, D> *)(((uintptr_t)this) | 1);
}

template <typename K, typename H, typename D>
LinkedListLf<K, H, D> *LinkedListLf<K, H, D>::get_clear_pointer()
{
  return (LinkedListLf<K, H, D> *)(((uintptr_t)this) & ~((uintptr_t)0x1));
}

/*                                     Private function                                               */
