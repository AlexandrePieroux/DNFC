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
class DefaultHazardPointerPolicy
{
public:
  const static int BlockSize = 4;
};

template <class T, class Policy = DefaultHazardPointerPolicy>
class HazardPointer
{
public:
  HazardPointer<T> &operator=(HazardPointer &hp)
  {
    return operator=(hp.release());
  }

  HazardPointer<T> &operator=(HazardPointer &&hp)
  {
    return operator=(std::move(hp.release()));
  }

  HazardPointer<T> &operator=(const T *p)
  {
    if (!ptr.get())
    {
      ptr.reset(HazardPointer<T>::getMyhp()->guard(p));
    }
    else
    {
      if (p != ptr->ptr.load(std::memory_order_relaxed))
        ptr->ptr.exchange(p, std::memory_order_release);
    }

    return *this;
  }

  HazardPointer<T> &operator=(const T **p)
  {
    return operator=(std::move(p));
  }

  /**
     * Retrieve the guarded pointer
     */
  const T *get()
  {
    if (!ptr)
      return nullptr;
    return ptr->ptr.load(std::memory_order_relaxed);
  }

  /**
     * Release the safety guard on the pointer.
     */
  const T *release()
  {
    std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T>::getMyhp();
    return myhp->release(ptr.release());
  }

  /**
     * Safely retire the node for deletion.
     */
  void retire()
  {
    std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T>::getMyhp();
    if (!myhp.get())
      return;
    myhp->retire(ptr.release());
    if (myhp->nbRetiredPtrs >= HazardPointerManager::get().getBatchSize())
    {
      HazardPointerManager::get().scan();
      HazardPointerManager::get().helpScan();
    }
  }

  HazardPointer() : ptr(nullptr)
  {
    HazardPointerManager::get().subscribe();
  }

  /**
   * Subscribe and get a HazardPointerRecord set with the right amount of pointers.
   */
  HazardPointer(const T *p)
  {
    HazardPointerManager::get().subscribe();
    ptr.reset(HazardPointer<T>::getMyhp()->guard(p));
  };

  /**
   * Unsubscribe and release the HazardPointerRecord.
   */
  ~HazardPointer()
  {
    if (ptr.get())
      HazardPointer<T>::getMyhp()->release(ptr.release());

    if (HazardPointerManager::get().nbhp.load(std::memory_order_relaxed) <= 0)
      HazardPointerManager::get().unsubscribe();
  };

private:
  /**
   * GuardedPointer
   *
   * A convenient double linked list that hold a pointer on which deletion is prohibited.
   */
  struct GuardedPointer
  {
    std::atomic<const T *> ptr;
    std::atomic<GuardedPointer *> next;

    void setPtr(const T *p) { ptr.store(p, std::memory_order_release); }
    void setNext(GuardedPointer *n) { next.store(n, std::memory_order_release); }
  };

  /**
   * GuardedPointerBlock
   *
   * An helping structure for memory managment.
   */
  struct GuardedPointerBlock
  {
    GuardedPointerBlock *next = nullptr;
    GuardedPointer *data;

    GuardedPointer *begin() { return data; }
    GuardedPointer *end() { return begin() + Policy::BlockSize; }

    const GuardedPointer *begin() const { return data; }
    const GuardedPointer *end() const { return begin() + Policy::BlockSize; }

    GuardedPointerBlock() : data(reinterpret_cast<GuardedPointer *>(std::malloc(Policy::BlockSize)))
    {
      for (auto &&g = this->begin(); g != this->end() - 1; g = g->next)
        g->next = g + 1;
    }
  };

  /**
   * HazardPointerRecord
   * 
   * An object that hold informations about allocated hazard pointers in a Hazard pointer manager. 
   */
  struct HazardPointerRecord
  {
    std::unique_ptr<GuardedPointer> rlistHead;
    std::atomic<int> nbRetiredPtrs;

    std::atomic<bool> active;
    std::unique_ptr<HazardPointerRecord> next;

    // Guard a given pointer
    GuardedPointer *guard(const T *p)
    {
      GuardedPointer *newPtr = allocPtr();
      newPtr->setPtr(p);

      HazardPointerManager::get().nbhp.fetch_add(1, std::memory_order_relaxed);
      return newPtr;
    }

    // Put a pointer on the retire list for it to be removed
    void retire(GuardedPointer *n)
    {
      n->setNext(rlistHead.release());
      rlistHead.reset(n);

      HazardPointerManager::get().nbhp.fetch_sub(1, std::memory_order_relaxed);
      nbRetiredPtrs.fetch_add(1, std::memory_order_relaxed);
    }

    // Destroy the guarded pointer
    void free(GuardedPointer *n)
    {
      n->ptr.load(std::memory_order_relaxed)->T::~T();
      freePtr(n);
    }

    // Return the GuardedPointer to the memory pool
    const T *release(GuardedPointer *p)
    {
      const T *data = p->ptr.load(std::memory_order_relaxed);
      freePtr(p);
      return data;
    }

    // Return in the passed vector of pointers, all the Hazard Pointers in this HazardPointerRecord
    void getHps(std::vector<const T *> &output)
    {
      getHpsPriv(output, blockHead.load(std::memory_order_relaxed));
    }

    HazardPointerRecord(HazardPointerRecord *head, bool active) : next(head), nbRetiredPtrs(0), blockHead(nullptr)
    {
      this->active.store(active, std::memory_order_relaxed);
    }

  private:
    std::atomic<GuardedPointerBlock *> blockHead;
    std::unique_ptr<GuardedPointer> flistHead;

    // Manage pointers memory allocation
    GuardedPointer *allocPtr()
    {
      GuardedPointer *newPtr;
      if (!flistHead.get())
      {
        GuardedPointerBlock *newBlock = new GuardedPointerBlock();
        GuardedPointerBlock *head = blockHead.exchange(newBlock, std::memory_order_relaxed);
        newBlock->next = head;
        flistHead.reset(newBlock->begin());
      }

      newPtr = flistHead.release();
      flistHead.reset(newPtr->next);

      return newPtr;
    }

    // Put HazardPointer on free list for reuse
    void freePtr(GuardedPointer *p)
    {
      GuardedPointer *head = flistHead.release();
      p->setPtr(nullptr);
      p->setNext(head);
      flistHead.reset(p);
    }

    void getHpsPriv(std::vector<const T *> &output, GuardedPointerBlock *block)
    {
      for (auto &&g : *block)
        output.push_back(g.ptr);
      if (block->next)
        getHpsPriv(output, block->next);
    }
  };

  /**
   * HazardPointerManager
   * 
   * This singleton private class provide a central managment point for safe memory reclamation. 
   * It provide memory managment for system wide hazard pointers.
   */
  friend class HazardPointerManager;
  class HazardPointerManager
  {
  public:
    static HazardPointerManager &get()
    {
      static HazardPointerManager m;
      return m;
    }

    HazardPointerManager(){};
    ~HazardPointerManager()
    {
      delete head.load();
    }

    std::atomic<HazardPointerRecord *> head;
    std::atomic<int> nbhp;

    /**
     * Subscribe a thread to the manager. During the time the thread is subscribed, it is
     * given a HazardPointerRecord object that allow it to protect number of pointers.
     */
    void subscribe()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T>::getMyhp();

      // Prevent subbing twice
      if (myhp.get())
        return;

      // First try to reuse a retire HP record
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
      nbhp.fetch_add(Policy::BlockSize, std::memory_order_relaxed);

      // Allocate and push a new record
      myhp.reset(new HazardPointerRecord(head.load(std::memory_order_relaxed), true));

      // Add the new record to the list
      HazardPointerRecord *nextPtrVal = myhp->next.get();
      while (!head.compare_exchange_weak(nextPtrVal, myhp.get(),
                                         std::memory_order_release,
                                         std::memory_order_relaxed))
        ;
    }

    /**
     * Unsubscribe the current thread from the manager.
     */
    static void unsubscribe()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T>::getMyhp();

      // Prevent call when already unsubscribed
      if (!myhp.get())
        return;

      myhp->active.store(false, std::memory_order_relaxed);
      myhp.release();
    }

    /**
     * Retrieve the threashold of the manager
     */
    unsigned int getBatchSize()
    {
      return (nbhp.load(std::memory_order_relaxed) * 2) + nbhp.load(std::memory_order_relaxed);
    }

    /**
     * Scan function that is called to garbage collect the memory.
     */
    void scan()
    {
      // Stage 1
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T>::getMyhp();
      std::vector<const T *> plist;
      for (auto &&i = head.load(std::memory_order_relaxed); i; i = i->next.get())
      {
        if (!i->active.load(std::memory_order_relaxed))
          continue;
        i->getHps(plist);
      }
      if (plist.size() <= 0)
        return;

      // Stage 2
      std::sort(plist.begin(), plist.end());
      GuardedPointer *localRList = myhp->rlistHead.release();

      // Stage 3
      for (auto &&g = localRList; g; g = g->next.load(std::memory_order_relaxed))
      {
        if (std::binary_search(plist.begin(), plist.end(), g->ptr.load(std::memory_order_relaxed)))
          myhp->retire(g);
        else
          myhp->free(g);
      }
    }

    void helpScan()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T>::getMyhp();
      bool expected = false;
      for (auto &&i = head.load(std::memory_order_relaxed); i; i = i->next.get())
      {
        // Trying to lock the next non-used hazard pointer record
        if (i->active.load(std::memory_order_relaxed) ||
            !i->active.compare_exchange_strong(expected, true,
                                               std::memory_order_acquire, std::memory_order_relaxed))
          continue;

        // Inserting the rlist of the node in myhp
        for (auto &&g = i->rlistHead.release(); g; g = g->next.load(std::memory_order_relaxed))
        {
          myhp->retire(g);

          // scan if we reached the threshold
          if (myhp->nbRetiredPtrs >= HazardPointerManager::get().getBatchSize())
            scan();
        }
        // Release the record
        i->active.store(false, std::memory_order_relaxed);
      }
    }
  };

  // Pointer guarded by the hazard pointer
  std::unique_ptr<GuardedPointer> ptr;

  // Thread local storage hazard pointer
  static std::unique_ptr<HazardPointerRecord> &getMyhp()
  {
    static thread_local std::unique_ptr<HazardPointerRecord> myhp;
    return myhp;
  }
};
} // namespace DNFC

#endif
