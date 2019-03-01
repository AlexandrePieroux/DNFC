#ifndef _SMRH_
#define _SMRH_

#include <vector>
#include <list>
#include <atomic>
#include <algorithm>
#include <memory>
#include <cassert>

namespace DNFC
{

/**
 * HazardPointer
 * 
 */
template <class T, std::size_t NbPtr>
class HazardPointer
{
public:
  /**
   * Access a HazardPointer through the HazardPointerRecord assigned
   * @param index  
   */
  std::atomic<T> &operator[](std::size_t index)
  {
    std::unique_ptr<HazardPointerRecord> &myhp = getMyhp();
    assert(myhp.get() != nullptr || index >= NbPtr);
    return myhp->hp[index];
  }

  /**
     * Safely delete a node.
     * @param node  
     */
  void retire(T &node)
  {
    std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, NbPtr>::getMyhp();
    if (!myhp.get())
      return;
    myhp->rlist.push_front(node);
    if (myhp->rlist.size() >= HazardPointerManager<NbPtr>::get().getBatchSize())
    {
      HazardPointerManager<NbPtr>::get().scan();
      HazardPointerManager<NbPtr>::get().help_scan();
    }
  }

  /**
   * Subscribe and get a HazardPointerRecord set with the right amount of pointers.
   */
  HazardPointer()
  {
    HazardPointerManager<NbPtr>::get().subscribe();
  };

  /**
   * Unsubscribe and release the HazardPointerRecord.
   */
  ~HazardPointer()
  {
    HazardPointerManager<NbPtr>::get().unsubscribe();
  };

private:
  /**
   * HazardPointerRecord
   * 
   * An object that hold informations about allocated hazard pointers in a Hazard pointer manager. 
   */
  struct HazardPointerRecord
  {
    std::atomic<T> hp[NbPtr];
    std::list<T> rlist;
    std::atomic<bool> active;
    std::unique_ptr<HazardPointerRecord> next;

    HazardPointerRecord &operator=(HazardPointerRecord &&other) = delete;
    HazardPointerRecord &operator=(const HazardPointerRecord &) = delete;

    HazardPointerRecord(HazardPointerRecord *head, bool active) : next(head)
    {
      this->active.store(active, std::memory_order_relaxed);
    }
  };

  /**
   * Thread local storage hazard pointer
   */
  static std::unique_ptr<HazardPointerRecord> &getMyhp()
  {
    thread_local std::unique_ptr<HazardPointerRecord> myhp;
    return myhp;
  }

  /**
   * HazardPointerManager
   * 
   * This class should not be used through HazardPointer template class.
   * This singleton private class providea  central managment point for safe memory reclamation. 
   * It provide memory managment for system wide hazard pointers.
   */
  template <std::size_t nbPtr>
  friend class HazardPointerManager;

  template <std::size_t nbPtr>
  class HazardPointerManager
  {
  public:
    static HazardPointerManager &get()
    {
      static HazardPointerManager m;
      return m;
    }

    // Does not allow to copy or move the manager
    HazardPointerManager &operator=(HazardPointerManager &&) = delete;
    HazardPointerManager &operator=(const HazardPointerManager &) = delete;

    // Does not allow to copy construct or move construct the manager
    HazardPointerManager(const HazardPointerManager &) = delete;
    HazardPointerManager(HazardPointerManager &&) = delete;
    HazardPointerManager(){};
    ~HazardPointerManager()
    {
      delete head.load();
    }

    std::atomic<HazardPointerRecord *> head;
    std::atomic<int> nbhp;

    /**
     * Subscribe a thread to the manager. During the time the thread is subscribed, it is
     * given a HazardPointerRecord object that allow it to protect 'nbPtr' number of pointers.
     */
    void subscribe()
    {
      // First try to reuse a retire HP record
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, nbPtr>::getMyhp();
      bool expected = false;
      for (HazardPointerRecord *i = head.load(std::memory_order_relaxed); i != nullptr; i = i->next.get())
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
      nbhp.fetch_add(nbPtr, std::memory_order_relaxed);

      // Allocate and push a new record
      myhp.reset(new HazardPointerRecord(head.load(std::memory_order_relaxed),
                                         true));

      // Add the new record to the list
      HazardPointerRecord *nextPtrVal = myhp->next.get();
      while (!head.compare_exchange_weak(nextPtrVal, myhp.get(),
                                         std::memory_order_release,
                                         std::memory_order_relaxed))
        ;
    }

    /**
     * Unsubscribe the current thread from the manager. Its HazardPointerRecord is put back into the store
     * for reuse.
     */
    static void unsubscribe()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, nbPtr>::getMyhp();
      if (!myhp.get())
        return;
      // Clear hp
      for (int i = 0; i < nbPtr; i++)
        myhp->hp[i].store(nullptr, std::memory_order_release);
      myhp->active.store(false, std::memory_order_relaxed);
      myhp.release();
    }

    unsigned int getBatchSize()
    {
      return (nbhp.load(std::memory_order_relaxed) * nbPtr * 2) + nbhp.load(std::memory_order_relaxed);
    }

    /**
     * Scan function that is called to garbage collect the memory.
     */
    void scan()
    {
      // Stage 1
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, nbPtr>::getMyhp();
      std::vector<T> plist;
      for (HazardPointerRecord *i = head.load(std::memory_order_relaxed); i; i = i->next.get())
      {
        for (int j = 0; j < nbPtr; j++)
        {
          T hp = i->hp[j].load(std::memory_order_relaxed);
          if (hp != nullptr)
            plist.push_back(hp);
        }
      }
      if (plist.size() <= 0)
        return;

      // Stage 2
      std::sort(plist.begin(), plist.end());
      std::list<T> localRList = myhp->rlist;
      myhp->rlist.clear();

      // Stage 3
      for (auto &&e : localRList)
      {
        if (std::binary_search(plist.begin(), plist.end(), e))
          myhp->rlist.push_front(e);
        else
          delete e; // TODO
        return;
      }
    }

    void help_scan()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, nbPtr>::getMyhp();
      bool expected = false;
      for (HazardPointerRecord *i = head.load(std::memory_order_relaxed); i; i = i->next.get())
      {
        // Trying to lock the next non-used hazard pointer record
        if (i->active.load(std::memory_order_relaxed) ||
            !i->active.compare_exchange_strong(expected, true,
                                               std::memory_order_acquire, std::memory_order_relaxed))
          continue;

        // Inserting the rlist of the node in myhp
        for (auto &&e : i->rlist)
        {
          myhp->rlist.push_front(e);

          // scan if we reached the threshold
          if (myhp->rlist.size() >= getBatchSize())
            scan();
        }
        // Release the record
        i->rlist.clear();
        i->active.store(false, std::memory_order_relaxed);
      }
    }
  };
};
} // namespace DNFC

#endif
