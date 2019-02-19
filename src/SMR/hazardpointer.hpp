#ifndef _SMRH_
#define _SMRH_

#include <list>
#include <vector>
#include <algorithm>
#include <atomic>
#include <boost/thread/tss.hpp>

namespace DNFC
{

// Verbose typedef
typedef void *data_pointer;

/**
 * HazardPointer
 * 
 */
template <std::size_t NbPtr>
class HazardPointer
{
public:
  /**
   * Access a HazardPointer through the HazardPointerRecord assigned
   * @param index  
   */
  std::atomic<data_pointer> &operator[](std::size_t index)
  {
    HazardPointerRecord &myhp = getMyhp();
    assert(myhp == nullptr && index >= myhp->hp.size());
    return myhp->hp[index];
  }

  /**
   * Safely delete a node.
   * @param node  
   */
  template <typename T>
  void deleteNode(T &&node)
  {
    HazardPointerRecord &myhp = getMyhp();
    if (!myhp.get())
      return;
    myhp->rlist.push_front(std::forward(node));
    if (myhp->rlist.size() >= getBatchSize())
    {
      scan();
      help_scan();
    }
  }

  /**
   * Subscribe and get a HazarPointerRecord set with the right amount of pointers.
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
   * Thread local storage hazard pointer
   */
  HazardPointerRecord *getMyhp()
  {
    thread_local HazardPointerRecord *myhp;
    return myhp;
  }

  /**
   * HazardPointerRecord
   * 
   * An object that hold informations about allocated hazard pointers in a Hazard pointer manager. 
   */
  class HazardPointerRecord
  {
  public:
    std::atomic<data_pointer> hp[NbPtr];
    std::list<data_pointer> rlist;
    std::atomic<bool> active;
    HazardPointerRecord *next;

    HazardPointerRecord &operator=(HazardPointerRecord &&other) = delete;
    HazardPointerRecord &operator=(const HazardPointerRecord &) = delete;

    HazardPointerRecord(HazardPointerRecord *head, bool active) : next(head)
    {
      this->active.store(active, std::memory_order_relaxed);
    }
    ~HazardPointerRecord(){};

  private:
    void resetHp()
    {
      if (nbHp > hpSize)
      {
        // TODO
      }
    }
  };

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
  private:
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
    HazardPointerManager() : nbhp(0){};
    ~HazardPointerManager(){};

    std::atomic<HazardPointerRecord *> head;
    std::atomic<int> nbhp;

    /**
     * Subscribe a thread to the manager. During the time the thread is subscribed, it is
     * given a HazardPointerRecord object that allow it to protect 'nbPtr' number of pointers.
     */
    static void subscribe()
    {
      // First try to reuse a retire HP record
      HazardPointerRecord &myhp = getMyhp();
      bool expected = false;
      for (HazardPointerRecord *i = head.load(std::memory_order_relaxed); i; i = i->next)
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
      nbhp.fetch_add(nbPointers, std::memory_order_relaxed);

      // Allocate and push a new record
      myhp.reset(new HazardPointerRecord(
          nbPtr,
          head.load(std::memory_order_relaxed),
          true));

      // Add the new record to the list
      while (!head.compare_exchange_weak(myhp->next, myhp.get(),
                                         std::memory_order_release,
                                         std::memory_order_relaxed))
        ;
    }

    /**
     * Unsubscribe the current thread from the manager. Its HazarPointerRecord is put back into the store
     * for reuse.
     */
    static void unsubscribe()
    {
      HazardPointerRecord &myhp = getMyhp();
      if (!myhp.get())
        return;
      // Clear hp
      for (int i = 0; i < nbPointers; i++)
        myhp->hp[i].store(nullptr, atomics::memory_order_release);
      myhp->active.store(false, std::memory_order_relaxed);
      myhp.release();
    }

    unsigned int getBatchSize()
    {
      return (nbhp.load(std::memory_order_relaxed) * nbPointers * 2) + nbhp.load(std::memory_order_relaxed);
    }

    /**
     * Scan function that is called to garbage collect the memory.
     */
    void scan()
    {
      // Stage 1
      HazardPointerRecord &myhp = getMyhp();
      std::vector<data_pointer> plist;
      for (HazardPointerRecord *i = head.load(std::memory_order_relaxed); i; i = i->next)
      {
        for (int j = 0; j < nbPointers; j++)
        {
          T hp = i->hp[j].load(std::memory_order_relaxed);
          if (hp.get())
            plist.push_back(hp);
        }
      }
      if (plist.size() <= 0)
        return;

      // Stage 2
      std::sort(plist.begin(), plist.end());
      std::list<data_pointer> localRList = myhp->rlist;
      myhp->rlist.clear();

      // Stage 3
      for (auto &&e : localRList)
      {
        if (std::binary_search(plist.begin(), plist.end(), e))
          myhp->rlist.push_front(e);
        else
          delete e;
      }
    }

    void help_scan()
    {
      HazardPointerRecord &myhp = getMyhp();
      bool expected = false;
      for (HazardPointerRecord *i = head.load(std::memory_order_relaxed); i; i = i->next)
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
