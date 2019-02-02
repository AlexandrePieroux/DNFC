#ifndef _SMRH_
#define _SMRH_

#include <list>
#include <vector>
#include <algorithm>
#include <atomic>
#include <boost/thread/tss.hpp>

namespace DNFC
{
template <typename T>
class HazardPointer
{
public:
  void subscribe()
  {
    // First try to reuse a retire HP record
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    bool expected = false;
    for (HazardPointerRecord *i = this->head.load(std::memory_order_relaxed); i; i = i->next)
    {
      if (i->active.load(std::memory_order_relaxed) ||
          !i->active.compare_exchange_strong(expected, true,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      // Sucessfully locked one record to use
      myhp.reset(i);
      return;
    }

    // No HP records available for reuse so we add new ones
    this->nbhp.fetch_add(this->nbpointers, std::memory_order_relaxed);

    // Allocate and push a new record
    myhp.reset(new HazardPointerRecord());
    myhp->hp = new std::atomic<T *>[this->nbpointers];
    myhp->next = this->head.load(std::memory_order_relaxed);
    myhp->active.store(true, std::memory_order_relaxed);

    // Add the new record to the list
    while (!this->head.compare_exchange_weak(myhp->next, myhp.get(),
                                             std::memory_order_release,
                                             std::memory_order_relaxed))
      ;
  }

  void unsubscribe()
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    if (!myhp.get())
      return;
    // Clear hp
    for (int i = 0; i < this->nbpointers; i++)
      myhp->hp[i].store(nullptr, std::memory_order_relaxed);
    myhp->active.store(false, std::memory_order_relaxed);
    myhp.release();
  }

  T *load(const int &index)
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    if (!myhp.get() || index >= this->nbpointers)
      return NULL;
    return myhp->hp[index].load(std::memory_order_relaxed);
  }

  void store(const int &index, T *value)
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    if (!myhp.get() || index >= this->nbpointers)
      return;
    myhp->hp[index].store(value, std::memory_order_relaxed);
  }

  void delete_node(T *node)
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    if (!myhp.get())
      return;
    myhp->rlist.push_front(node);
    if (myhp->rlist.size() >= this->get_batch_size())
    {
      this->scan();
      this->help_scan();
    }
  }

  HazardPointer(int a) : head(nullptr), nbhp(0), nbpointers(a){};
  ~HazardPointer(){};

private:
  struct HazardPointerRecord
  {
    std::atomic<T *> *hp;
    std::list<T *> rlist;
    std::atomic<bool> active;
    HazardPointerRecord *next;
  };

  std::atomic<HazardPointerRecord *> head;
  std::atomic<int> nbhp;
  int nbpointers;

  static inline boost::thread_specific_ptr<HazardPointerRecord> &get_myhp()
  {
    static boost::thread_specific_ptr<HazardPointerRecord> myhp;
    return myhp;
  }

  unsigned int get_batch_size()
  {
    return (this->nbhp.load(std::memory_order_relaxed) * this->nbpointers * 2) + this->nbhp.load(std::memory_order_relaxed);
  }

  void scan()
  {
    // Stage 1
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    std::vector<T *> plist;
    for (HazardPointerRecord *i = this->head.load(std::memory_order_relaxed); i; i = i->next)
    {
      for (int j = 0; j < this->nbpointers; j++)
      {
        T *hp = i->hp[j].load(std::memory_order_relaxed);
        if (hp)
          plist.push_back(hp);
      }
    }
    if (plist.size() <= 0)
      return;

    // Stage 2
    std::sort(plist.begin(), plist.end());
    std::list<T *> localRList = myhp->rlist;
    myhp->rlist.clear();

    // Stage 3
    for (T *e : localRList)
    {
      if (std::binary_search(plist.begin(), plist.end(), e))
        myhp->rlist.push_front(e);
      else
        delete e;
    }
  }

  void help_scan()
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = get_myhp();
    bool expected = false;
    for (HazardPointerRecord *i = this->head.load(std::memory_order_relaxed); i; i = i->next)
    {
      // Trying to lock the next non-used hazard pointer record
      if (i->active.load(std::memory_order_relaxed) ||
          !i->active.compare_exchange_strong(expected, true,
                                             std::memory_order_acquire, std::memory_order_relaxed))
        continue;

      // Inserting the rlist of the node in myhp
      for (T *e : i->rlist)
      {
        myhp->rlist.push_front(e);

        // scan if we reached the threshold
        if (myhp->rlist.size() >= this->get_batch_size())
          this->scan();
      }
      // Release the record
      i->rlist.clear();
      i->active.store(false, std::memory_order_relaxed);
    }
  }
};
} // namespace DNFC

#endif
