#ifndef _SMRH_
#define _SMRH_

#include <list>
#include <vector>
#include <algorithm>
#include <atomic>

template <typename T>
class HazardPointer
{
public:
  struct HazardPointerRecord
  {
    std::vector<std::atomic<T *>> hp;
    std::list<T *> rlist;
    std::atomic<bool> active;
    HazardPointerRecord *next;
  };

  void subscribe()
  {
    // First try to reuse a retire HP record
    bool expected = false;
    HazardPointer<T>::HazardPointerRecord *myhp = this->get_myhp();
    for (HazardPointer<T>::HazardPointerRecord *i = this->head; i; i = i->next)
    {
      if (i->active.load(std::memory_order_relaxed) ||
          !i->active.compare_exchange_strong(expected, true,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      // Sucessfully locked one record to use
      myhp = i;
      return;
    }

    // No HP records available for reuse so we add new ones
    this->nbhp.fetch_add(this->nbpointers, std::memory_order_relaxed);

    // Allocate and push a new record
    myhp = new HazardPointer<T>::HazardPointerRecord();
    myhp->next = this->head.load(std::memory_order_relaxed);
    myhp->active = true;

    // Add the new record to the list
    while (!this->head.compare_exchange_weak(myhp->next, myhp,
                                             std::memory_order_release,
                                             std::memory_order_relaxed))
      ;
  }

  void unsubscribe()
  {
    HazardPointer<T>::HazardPointerRecord *myhp = this->get_myhp();
    if (!myhp)
      return;
    myhp->hp.clear();
    myhp->active = false;
  }

  T *get_pointer(const int &index)
  {
    HazardPointer<T>::HazardPointerRecord *myhp = this->get_myhp();
    if (!myhp || index >= this->nbpointers)
      return NULL;
    return &(myhp)->hp[index];
  }

  void delete_node(const T *node)
  {
    HazardPointer<T>::HazardPointerRecord *myhp = this->get_myhp();
    if (!myhp)
      return;
    myhp->rList.push_front(node);
    if (myhp->rList.size >= this->get_batch_size())
    {
      this->scan();
      this->help_scan();
    }
  }

  HazardPointer(int a) : head(nullptr), nbhp(0), nbpointers(a){};
  ~HazardPointer(){};

private:
  inline HazardPointer<T>::HazardPointerRecord *get_myhp()
  {
    thread_local static typename HazardPointer<T>::HazardPointerRecord *myhp;
    return myhp;
  }

  inline unsigned int get_batch_size()
  {
    return (this->nbhp.load(std::memory_order_relaxed) * this->nbpointers * 2) + this->nbhp.load(std::memory_order_relaxed);
  }

  void scan()
  {
    // Stage 1
    HazardPointer<T>::HazardPointerRecord *myhp = this->get_myhp();
    std::vector<void *> plist;
    for (HazardPointer<T>::HazardPointerRecord *i = this->head.load(std::memory_order_relaxed); i; i = i->next)
    {
      for (const std::atomic<T *> &e : i->hp)
      {
        std::atomic<T *> hp = e->load(std::memory_order_relaxed);
        if (hp)
          plist.push_back(hp);
      }
    }
    if (plist.size() <= 0)
      return;

    // Stage 2
    std::sort(plist, plist);
    std::list<T *> localRList = myhp->rList;
    myhp->rList.clear();

    // Stage 3
    for (const T *&e : localRList)
    {
      if (std::binary_search(plist.begin(), plist.end(), *e))
        myhp->rList.push_front(*e);
      else
        delete *e;
    }
  }
  
  void help_scan()
  {
    bool expected = false;
    HazardPointer<T>::HazardPointerRecord *myhp = this->get_myhp();
    for (HazardPointer<T>::HazardPointerRecord *i = this->head.load(std::memory_order_relaxed); i; i = i->next)
    {
      // Trying to lock the next non-used hazard pointer record
      if (i->active.load(std::memory_order_relaxed) ||
          !i->active.compare_exchange_strong(expected, true,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      // Inserting the rList of the node in myhp
      for (const T *&e : i->rList)
      {
        myhp->rList.push_front(*(T **)e);

        // scan if we reached the threshold
        if (myhp->rList.size >= this->get_batch_size())
          this->scan();
      }
      // Release the record
      i->active = false;
    }
  }

  std::atomic<HazardPointerRecord *> head;
  std::atomic<int> nbhp;
  int nbpointers;
};

#endif
