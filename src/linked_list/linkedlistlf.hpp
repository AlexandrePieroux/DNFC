#ifndef _LINKED_LISTH_
#define _LINKED_LISTH_

#include <cstdint>
#include <boost/thread/tss.hpp>
#include "../SMR/hazardpointer.hpp"

#define RELEASE_PTRS(p, c, n) \
  {                           \
    p.release();              \
    c.release();              \
    n.release();              \
  }

#define SOFT_SET(p, n) \
  {                    \
    p.release();       \
    p.reset(n);        \
  }

#define NB_HP 3
#define PREV 0
#define CUR 1
#define NEXT 2

using namespace SMR;

namespace LLLF
{
template <typename K, typename D>
class LinkedListLf
{
public:
  std::atomic<LinkedListLf<K, D> *> next;
  K key;
  K hash;
  D data;

  LinkedListLf<K, D> *insert(const K &key, const D &data)
  {
    return this->insert(key, data, key);
  }

  LinkedListLf<K, D> *insert(const K &key, const D &data, const K &hash)
  {
    LinkedListLf<K, D> *item = new LinkedListLf<K, D>(key, data, hash, this->hp);
    LinkedListLf<K, D> *result = this->insert(item);
    if (item != result)
      delete (item);
    return result;
  }

  LinkedListLf<K, D> *insert(LinkedListLf<K, D> *item)
  {
    boost::thread_specific_ptr<LinkedListLf<K, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &next = get_next();
    this->hp->subscribe();

    LinkedListLf<K, D> *result = nullptr;
    for (;;)
    {
      if (this->find(item->key, item->hash))
      {
        result = cur.get();
        break;
      }

      LinkedListLf<K, D> *curcleared = cur->get_clear_pointer();
      item->next = curcleared;

      if (prev->next.compare_exchange_strong(curcleared, item,
                                             std::memory_order_acquire, std::memory_order_relaxed))
      {
        result = item;
        item->hp = this->hp;
        break;
      }
    }

    this->hp->unsubscribe();
    RELEASE_PTRS(prev, cur, next)
    return result;
  }

  LinkedListLf<K, D> *get(const K &key)
  {
    return this->get(key, key);
  }

  LinkedListLf<K, D> *get(const K &key, const K &hash)
  {
    boost::thread_specific_ptr<LinkedListLf<K, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &next = get_next();
    this->hp->subscribe();

    LinkedListLf<K, D> *result = nullptr;

    if (this->find(key, hash))
      result = cur.get();

    this->hp->unsubscribe();
    RELEASE_PTRS(prev, cur, next)
    return result;
  }

  bool remove(const K &key)
  {
    return this->remove(key, key);
  }

  bool remove(const K &key, const K &hash)
  {
    boost::thread_specific_ptr<LinkedListLf<K, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &next = get_next();
    this->hp->subscribe();

    bool result;

    for (;;)
    {
      if (!this->find(key, hash))
      {
        result = false;
        break;
      }

      LinkedListLf<K, D> *nextmarked = next->mark_pointer();
      LinkedListLf<K, D> *nextcleared = next->get_clear_pointer();

      if (!cur->next.compare_exchange_strong(nextcleared, nextmarked,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      LinkedListLf<K, D> *cur_tmp = cur.get();
      if (prev->next.compare_exchange_strong(cur_tmp, nextcleared,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        this->hp->delete_node(cur.get());
      else
        this->find(key, hash);

      result = true;
      break;
    }

    this->hp->unsubscribe();
    RELEASE_PTRS(prev, cur, next)
    return result;
  }

  LinkedListLf<K, D>(const K k = 0, const D &d = 0, const K h = 0, HazardPointer<LinkedListLf<K, D>> *hptr = new HazardPointer<LinkedListLf<K, D>>(NB_HP)) : key(k),
                                                                                                                                                             hash(h),
                                                                                                                                                             data(d),
                                                                                                                                                             hp(hptr),
                                                                                                                                                             next(nullptr){};
  ~LinkedListLf<K, D>(){};

private:
  static inline boost::thread_specific_ptr<LinkedListLf<K, D>> &get_prev()
  {
    static boost::thread_specific_ptr<LinkedListLf<K, D>> prev_ptr;
    return prev_ptr;
  }

  static inline boost::thread_specific_ptr<LinkedListLf<K, D>> &get_cur()
  {
    static boost::thread_specific_ptr<LinkedListLf<K, D>> cur_ptr;
    return cur_ptr;
  }

  static inline boost::thread_specific_ptr<LinkedListLf<K, D>> &get_next()
  {
    static boost::thread_specific_ptr<LinkedListLf<K, D>> next_ptr;
    return next_ptr;
  }

  HazardPointer<LinkedListLf<K, D>> *hp;

  bool find(const K &key, const K &hash)
  {
    boost::thread_specific_ptr<LinkedListLf<K, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, D>> &next = get_next();

    for (;;)
    {
      SOFT_SET(prev, this)
      SOFT_SET(cur, this->next.load(std::memory_order_relaxed))
      this->hp->store(CUR, cur->get_clear_pointer());

      if (prev->next.load(std::memory_order_relaxed) != cur->get_clear_pointer())
        continue;

      for (;;)
      {
        if (!cur.get())
          return false;

        SOFT_SET(next, cur->next.load(std::memory_order_relaxed))
        LinkedListLf<K, D> *nextcleared = next->get_clear_pointer();
        this->hp->store(NEXT, nextcleared);

        if (cur->next.load(std::memory_order_relaxed) != next.get())
          break;

        K ckey = cur->key;
        K chash = cur->hash;

        if (prev->next.load(std::memory_order_relaxed) != cur->get_clear_pointer())
          break;

        if (!cur->get_mark())
        {
          if (chash > hash || (key == ckey && chash == hash))
            return (key == ckey && chash == hash);
          SOFT_SET(prev, cur.get())
          this->hp->store(PREV, cur.get());
        }
        else
        {
          LinkedListLf<K, D> *curcleared = cur->get_clear_pointer();
          if (prev->next.compare_exchange_strong(curcleared, nextcleared,
                                                 std::memory_order_acquire, std::memory_order_relaxed))
            this->hp->delete_node(cur.get());
          else
            break;
        }
        SOFT_SET(cur, nextcleared)
        this->hp->store(CUR, nextcleared);
      }
    }
  }

  LinkedListLf<K, D> *mark_pointer()
  {
    return (LinkedListLf<K, D> *)(((uintptr_t)this) | 0x1);
  }

  uintptr_t get_mark()
  {
    return (uintptr_t)(this->next.load(std::memory_order_relaxed)) & 0x1;
  }

  LinkedListLf<K, D> *get_clear_pointer()
  {
    return (LinkedListLf<K, D> *)(((uintptr_t)this) & ~((uintptr_t)0x1));
  }
};
} // namespace LLLF

#endif
