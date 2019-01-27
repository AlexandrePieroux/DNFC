/*
  Known issues:
    * This is assumed that H (hash type) can be cast to K (key type).
    * The find function order the list in inceasing order using the hash.
*/

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

#define NB_HP 3
#define PREV 0
#define CUR 1
#define NEXT 2

template <typename K, typename H, typename D>
class LinkedListLf
{
public:
  std::atomic<LinkedListLf<K, H, D> *> next;
  K key;
  H hash;
  D data;

  LinkedListLf<K, H, D> *insert(const D &data, const K &key)
  {
    return this->insert(data, key, key);
  }

  LinkedListLf<K, H, D> *insert(const D &data, const K &key, const H &hash)
  {
    LinkedListLf<K, H, D> *item = new LinkedListLf<K, H, D>(this->hp, key, hash, data);
    LinkedListLf<K, H, D> *result = this->insert(item);
    if (item != result)
      delete (item);
    return result;
  }

  LinkedListLf<K, H, D> *insert(LinkedListLf<K, H, D> *item)
  {
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &next = get_next();
    this->hp->subscribe();

    LinkedListLf<K, H, D> *result = nullptr;
    for (;;)
    {
      if (this->find(item->key, item->hash))
      {
        result = cur.get();
        break;
      }

      LinkedListLf<K, H, D> *curcleared = cur->get_clear_pointer();
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

  LinkedListLf<K, H, D> *get(const K &key)
  {
    return this->get(key, key);
  }

  LinkedListLf<K, H, D> *get(const K &key, const H &hash)
  {
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &next = get_next();
    this->hp->subscribe();

    LinkedListLf<K, H, D> *result = nullptr;

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

  bool remove(const K &key, const H &hash)
  {
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &next = get_next();
    this->hp->subscribe();

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

      if (!cur->next.compare_exchange_strong(nextcleared, nextmarked,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      LinkedListLf<K, H, D> *cur_tmp = cur.get();
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

  LinkedListLf<K, H, D>(HazardPointer<LinkedListLf<K, H, D>> *hptr = new HazardPointer<LinkedListLf<K, H, D>>(NB_HP), const K k = 0, const H h = 0, const D &d = 0) : key(k),
                                                                                                                                                                      hash(h),
                                                                                                                                                                      data(d),
                                                                                                                                                                      hp(hptr),
                                                                                                                                                                      next(nullptr){};
  ~LinkedListLf<K, H, D>(){};

private:
  static inline boost::thread_specific_ptr<LinkedListLf<K, H, D>> &get_prev()
  {
    static boost::thread_specific_ptr<LinkedListLf<K, H, D>> prev_ptr;
    return prev_ptr;
  }

  static inline boost::thread_specific_ptr<LinkedListLf<K, H, D>> &get_cur()
  {
    static boost::thread_specific_ptr<LinkedListLf<K, H, D>> cur_ptr;
    return cur_ptr;
  }

  static inline boost::thread_specific_ptr<LinkedListLf<K, H, D>> &get_next()
  {
    static boost::thread_specific_ptr<LinkedListLf<K, H, D>> next_ptr;
    return next_ptr;
  }

  HazardPointer<LinkedListLf<K, H, D>> *hp;

  bool find(const K &key, const H &hash)
  {
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &prev = get_prev();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &cur = get_cur();
    boost::thread_specific_ptr<LinkedListLf<K, H, D>> &next = get_next();

    for (;;)
    {
      prev.reset(this);
      cur.reset(this->next.load(std::memory_order_relaxed));
      this->hp->store(CUR, cur->get_clear_pointer());

      if (prev->next.load(std::memory_order_relaxed) != cur->get_clear_pointer())
        continue;

      for (;;)
      {
        if (!cur.get())
          return false;

        next.reset(cur->next.load(std::memory_order_relaxed));
        LinkedListLf<K, H, D> *nextcleared = next->get_clear_pointer();
        this->hp->store(NEXT, nextcleared);

        if (cur->next.load(std::memory_order_relaxed) != next.get())
          break;

        K ckey = cur->key;
        H chash = cur->hash;

        if (prev->next.load(std::memory_order_relaxed) != cur->get_clear_pointer())
          break;

        if (!cur->get_mark())
        {
          if (chash > hash || (key == ckey && chash == hash))
            return (key == ckey && chash == hash);
          prev.reset(cur.get());
          this->hp->store(PREV, cur.get());
        }
        else
        {
          LinkedListLf<K, H, D> *curcleared = cur->get_clear_pointer();
          if (prev->next.compare_exchange_strong(curcleared, nextcleared,
                                                 std::memory_order_acquire, std::memory_order_relaxed))
            this->hp->delete_node(cur.get());
          else
            break;
        }
        cur.reset(nextcleared);
        this->hp->store(CUR, nextcleared);
      }
    }
  }

  LinkedListLf<K, H, D> *mark_pointer()
  {
    return (LinkedListLf<K, H, D> *)(((uintptr_t)this) | 0x1);
  }

  uintptr_t get_mark()
  {
    return (uintptr_t)(this->next.load(std::memory_order_relaxed)) & 0x1;
  }

  LinkedListLf<K, H, D> *get_clear_pointer()
  {
    return (LinkedListLf<K, H, D> *)(((uintptr_t)this) & ~((uintptr_t)0x1));
  }
};

#endif
