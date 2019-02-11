#ifndef _SMRH_
#define _SMRH_

#include <list>
#include <vector>
#include <algorithm>
#include <atomic>
#include <boost/thread/tss.hpp>

namespace DNFC
{

typedef void *data_pointer;

/**
 * HazardPointerManager
 * 
 * This class should not be used through HazardPointer template class.
 * This singleton private class providea  central managment point for safe memory reclamation. 
 * It provide memory managment for system wide hazard pointers.
 */
class HazardPointerManager
{
private:
  template <std::size_t nbPtr>
  friend class HazardPointer;

  /**
   * HazardPointerRecord
   * 
   * An object that hold informations about allocated hazard pointers in a Hazard pointer manager. 
   */
  class HazardPointerRecord
  {
  public:
    std::unique_ptr<std::atomic<data_pointer>[]> hp;
    int hpSize;
    std::list<data_pointer> rlist;
    std::atomic<bool> active;
    HazardPointerRecord *next;

    std::atomic<data_pointer> &operator[](std::size_t index)
    {
      assert(index >= hpSize);
      return hp[index];
    }

    HazardPointerRecord &operator=(HazardPointerRecord &&other) = delete;
    HazardPointerRecord &operator=(const HazardPointerRecord &) = delete;

    HazardPointerRecord(std::size_t nbHp, HazardPointerRecord *head, bool active) : next(head),
                                                                                    hpSize(nbHp)
    {
      this->active.store(active, std::memory_order_relaxed);
    }
    ~HazardPointerRecord(){};

  private:
    void resetHp(std::size_t nbHp)
    {
      if (nbHp > hpSize)
      {
      }
    }
  };

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

  std::atomic<std::unique_ptr<HazardPointerRecord>> head;
  std::atomic<int> nbhp;

  /**
   * Subscribe a thread to the manager. During the time the thread is subscribed, it is
   * given a HazardPointerRecord object that allow it to protect 'nbHp' number of pointers.
   * */
  static void subscribe(std::size_t nbHp)
  {
    // First try to reuse a retire HP record
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = getMyhp();
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
        nbHp,
        head.load(std::memory_order_relaxed),
        true));

    // Add the new record to the list
    while (!head.compare_exchange_weak(myhp->next, myhp.get(),
                                       std::memory_order_release,
                                       std::memory_order_relaxed))
      ;
  }

  /**
   * 
   */
  static void unsubscribe()
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = getMyhp();
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
   * 
   */
  void scan()
  {
    // Stage 1
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = getMyhp();
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
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = getMyhp();
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

/**
 * HazardPointer
 * 
 */
template <std::size_t NbPtr>
class HazardPointer
{
public:
  /**
   *  
   */
  static constexpr const std::size_t capacity = NbPtr;

  /**
   * 
   * @param index  
   */
  std::atomic<data_pointer> &operator[](std::size_t index)
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = getMyhp();
    assert(index >= myhp->hp.size());
    return myhp->hp[index];
  }

  /**
   * 
   * @param node  
   */
  template <typename T>
  void deleteNode(T &&node)
  {
    boost::thread_specific_ptr<HazardPointerRecord> &myhp = getMyhp();
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
   * 
   */
  HazardPointer()
  {
    HazardPointerManager::get()->subscribe(capacity);
  };

  /**
   * 
   */
  ~HazardPointer()
  {
    HazardPointerManager::get()->unsubscribe();
  };

private:
  static boost::thread_specific_ptr<HazardPointerRecord> &getMyhp()
  {
    static boost::thread_specific_ptr<HazardPointerRecord> myhp;
    return myhp;
  }
};
} // namespace DNFC

#endif
