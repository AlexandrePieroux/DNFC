#include "linkedlistlf.hpp"

/*                                     Private per thread variables                                           */

template <typename K, typename H, typename D>
thread_local LinkedListLf<K, H, D> *cur;
thread_local LinkedListLf<K, H, D> *prev;
thread_local LinkedListLf<K, H, D> *next;

/*                                     Private per thread variables                                           */

LinkedListLf<K, H, D> *LinkedListLf::insert(const D *data, const K *key)
{
  return this->insert(data, key, key);
}

LinkedListLf<K, H, D> *LinkedListLf::insert(const D *data, const K *key, const H *hash)
{
  // Hazard pointers
  this->hp->subscribe();
  std::atomic<LinkedListLf<K, H, D> *> *prevhp = this->hp->get_pointer(PREV);
  std::atomic<LinkedListLf<K, H, D> *> *curhp = this->hp->get_pointer(CUR);
  std::atomic<LinkedListLf<K, H, D> *> *nexthp = this->hp->get_pointer(NEXT);

  // Private per thread variables
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

LinkedListLf<K, H, D> *LinkedListLf::get(const K *key)
{
  return this->get(key, key);
}

LinkedListLf<K, H, D> *LinkedListLf::get(const K *key, const H *hash)
{
  // Hazard pointers
  this->hp->subscribe();
  std::atomic<LinkedListLf<K, H, D> *> prevhp = this->hp->get_pointer(PREV);
  std::atomic<LinkedListLf<K, H, D> *> curhp = this->hp->get_pointer(CUR);
  std::atomic<LinkedListLf<K, H, D> *> nexthp = this->hp->get_pointer(NEXT);

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

bool LinkedListLf::delete_item(const K *key)
{
  return this->delete_item(k, k);
}

bool LinkedListLf::delete_item(const K *key, const H *hash)
{
  // Hazard pointers
  this->hp->subscribe();
  std::atomic<LinkedListLf<K, H, D> *> *prevhp = this->hp->get_pointer(PREV);
  std::atomic<LinkedListLf<K, H, D> *> *curhp = this->hp->get_pointer(CUR);
  std::atomic<LinkedListLf<K, H, D> *> *nexthp = this->hp->get_pointer(NEXT);

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

bool LinkedListLf::find(K key, H hash)
{
  // Hazard pointers
  std::atomic<LinkedListLf<K, H, D> *> *prevhp = this->hp->get_pointer(PREV);
  std::atomic<LinkedListLf<K, H, D> *> *curhp = this->hp->get_pointer(CUR);
  std::atomic<LinkedListLf<K, H, D> *> *nexthp = this->hp->get_pointer(NEXT);

  for (;;)
  {
    prev = this->next->load(std::memory_order_relaxed);
    cur = prev->get_clear_pointer();
    curhp->store(cur, std::memory_order_relaxed);

    if (*prev != *cur_p) // ??
      continue;

    for (;;)
    {
      if (!cur)
        return nullptr;

      next = cur->next.load(std::memory_order_relaxed);
      nexthp->store(next->get_clear_pointer(), std::memory_order_relaxed);

      if (cur->next.load(std::memory_order_relaxed) != *next)
        break;

      K ckey = cur->key;
      H chash = cur->hash;

      if (*prev != cur->get_clear_pointer()) // ??
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
        struct linked_list *cur_cleared = get_clear_pointer(*cur_p);

        if(prev->next->compare_exchange_strong(cur, next,
                                               std::memory_order_acquire, std::memory_order_relaxed))
          this->hp->delete_node(cur);
        else
          break;
      }
      curhp->store(next, std::memory_order_relaxed);
      cur = curhp->load(std::memory_order_relaxed);
    }
  }
  return false; // Never happen
}

uintptr_t LinkedListLf::get_mark();
{
  return (uintptr_t)(list->next) & 0x1;
}

LinkedListLf<K, H, D> *LinkedListLf::mark_pointer()
{
  return (struct linked_list *)(((uintptr_t)list) | 1);
}

LinkedListLf<K, H, D> *LinkedListLf::get_clear_pointer()
{
  return (struct linked_list *)(((uintptr_t)this) & ~((uintptr_t)0x1));
}

/*                                     Private function                                               */
