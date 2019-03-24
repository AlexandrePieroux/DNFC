#ifndef _SMRH_
#define _SMRH_

#include <vector>
#include <atomic>
#include <algorithm>

namespace DNFC
{
/**
 * HazardPointer default policy
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
  /**
     * operator=(const HazardPointer& hp)
     * 
     * copy operator= with the reference to another Hazard Pointer.
     */
  HazardPointer<T, Policy> &operator=(const HazardPointer &hp)
  {
    return operator=(hp.get());
  }

  /**
     * operator=(HazardPointer&& hp)
     * 
     * move operator= with another Hazard Pointer.
     */
  HazardPointer<T, Policy> &operator=(HazardPointer &&hp)
  {
    return operator=(std::move(hp.release()));
  }

  /**
     * operator=(T* p)
     * 
     * operator= with a pointer to guard.
     */
  HazardPointer<T, Policy> &operator=(T *p)
  {
    if (!ptr.get())
    {
      ptr.reset(HazardPointer<T, Policy>::getMyhp()->guard(p));
    }
    else
    {
      if (p != ptr->ptr.load(std::memory_order_relaxed))
        ptr->ptr.exchange(p, std::memory_order_release);
    }

    return *this;
  }

  /**
     * get
     *
     * Retrieve the guarded pointer.
     */
  T *get() const
  {
    if (!ptr.get())
      return nullptr;
    return ptr->ptr.load(std::memory_order_relaxed);
  }

  /**
     * release
     *
     * Release the safety guard on the pointer.
     */
  T *release()
  {
    if (!ptr.get())
      return nullptr;
    return ptr->ptr.exchange(nullptr, std::memory_order_release);
  }

  /**
     * retire
     *
     * Safely retire the node for deletion.
     */
  void retire()
  {

    std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, Policy>::getMyhp();
    if (!myhp.get())
      return;

    myhp->retire(ptr.release());
    if (myhp->nbRetiredPtrs >= HazardPointerManager::get().getBatchSize())
    {
      HazardPointerManager::get().scan();
      HazardPointerManager::get().helpScan();
    }
  }

  /**
   * Constructor
   *
   * Automatically subscribe to the manager but don't guard any pointer.
   * The manager will prevent a thread to subscribe more than one time.
   */
  HazardPointer() : ptr(nullptr)
  {
    HazardPointerManager::get().subscribe();
  }

  /**
   * Constructor
   *
   * Automatically subscribe to the manager and guard the given pointer.
   * The manager will prevent a thread to subscribe more than one time.
   */
  HazardPointer(T *p)
  {
    HazardPointerManager::get().subscribe();
    if (ptr.get())
    {
      ptr->ptr.exchange(p, std::memory_order_release);
      return;
    }
    ptr.reset(HazardPointer<T, Policy>::getMyhp()->guard(p));
  };

  /**
   * Release the guarded pointer if there is one. If no more pointers are 
   * guarded in this thread, then it will unsubscribe from the manager.
   */
  ~HazardPointer()
  {
    if (ptr.get())
    {
      HazardPointer<T, Policy>::getMyhp()->release(ptr.release());

      if (HazardPointerManager::get().nbhp.load(std::memory_order_relaxed) <= 0)
        HazardPointerManager::get().unsubscribe();
    }
  };

private:
  /**
   * GuardedPointer
   *
   * A convenient linked list that hold a pointer on which deletion is prohibited.
   * In practice each GuardedPointer are contiguous within each GuardedPointerBlock.
   * See GuardedPointerBlock for more information.
   */
  struct GuardedPointer
  {
    std::atomic<T *> ptr;
    std::atomic<GuardedPointer *> next;

    void setPtr(T *p) { ptr.store(p, std::memory_order_release); }
    void setNext(GuardedPointer *n) { next.store(n, std::memory_order_release); }
    GuardedPointer *getNext() { return getCleared(next.load(std::memory_order_relaxed)); }

    bool isDeleted()
    {
      uintptr_t nextPtr = reinterpret_cast<uintptr_t>(next.load(std::memory_order_relaxed));
      return reinterpret_cast<GuardedPointer *>(nextPtr & 0x1);
    }

    void markAsDeleted()
    {
      uintptr_t nextPtr = reinterpret_cast<uintptr_t>(next.load(std::memory_order_relaxed));
      GuardedPointer *mrkd = reinterpret_cast<GuardedPointer *>(nextPtr | 0x1);
      next.store(mrkd, std::memory_order_release);
    }

  private:
    GuardedPointer *getCleared(GuardedPointer *p)
    {
      uintptr_t nextPtr = reinterpret_cast<uintptr_t>(p);
      return reinterpret_cast<GuardedPointer *>(nextPtr & ~0x1);
    }
  };

  /**
   * GuardedPointerBlock
   *
   * An helping structure for memory managment. This structure represent a 'chunk'
   * of memory used for GuardedPointer contiguous arrays. This will allocate the
   * 'Policy::BlockSize' number of GuardedPointer. 
   */
  struct GuardedPointerBlock
  {
    std::unique_ptr<GuardedPointerBlock> next = nullptr;

    GuardedPointer *begin() { return reinterpret_cast<GuardedPointer *>(this + 1); }
    GuardedPointer *end() { return begin() + Policy::BlockSize; }

    const GuardedPointer *begin() const { return reinterpret_cast<const GuardedPointer *>(this + 1); }
    const GuardedPointer *end() const { return begin() + Policy::BlockSize; }

    GuardedPointerBlock()
    {
      GuardedPointer *last = end() - 1;
      for (auto &&g = begin(); g != last; g = g->next)
        g->next = g + 1;
      last->next = nullptr;
    };
  };

  /**
   * HazardPointerRecord
   * 
   * An object that hold informations about allocated hazard pointers for the current
   * thread.
   */
  struct HazardPointerRecord
  {
    GuardedPointer *rlistHead = nullptr;
    int nbRetiredPtrs;

    std::atomic<bool> active;
    std::unique_ptr<HazardPointerRecord> next = nullptr;

    // Guard a given pointer
    GuardedPointer *guard(T *p)
    {
      GuardedPointer *newPtr = allocPtr();
      newPtr->setPtr(p);
      HazardPointerManager::get().nbhp.fetch_add(1, std::memory_order_relaxed);
      return newPtr;
    }

    // Put a pointer on the retire list for it to be removed
    void retire(GuardedPointer *n)
    {
      n->setNext(rlistHead);
      n->markAsDeleted();
      rlistHead = n;

      HazardPointerManager::get().nbhp.fetch_sub(1, std::memory_order_relaxed);
      nbRetiredPtrs++;
    }

    // Return the GuardedPointer to the memory pool
    const T *release(GuardedPointer *p)
    {
      const T *data = p->ptr.load(std::memory_order_relaxed);
      freePtr(p);
      return data;
    }

    // Destroy the guarded pointer
    void free(GuardedPointer *n)
    {
      delete n->ptr.load(std::memory_order_relaxed);
      n->ptr.store(nullptr, std::memory_order_release);
      freePtr(n);
    }

    // Return in the passed vector of pointers, all the Hazard Pointers in this HazardPointerRecord
    void getHps(std::vector<T *> &output)
    {
      getHpsPriv(output, blockHead.load(std::memory_order_relaxed));
    }

    HazardPointerRecord(HazardPointerRecord *head, bool active) : next(head), nbRetiredPtrs(0), blockHead(nullptr)
    {
      this->active.store(active, std::memory_order_relaxed);
    }

    ~HazardPointerRecord()
    {
      delete blockHead.load();
    }

  private:
    std::atomic<GuardedPointerBlock *> blockHead;
    GuardedPointer *flistHead = nullptr;

    // Manage pointers memory allocation
    GuardedPointer *allocPtr()
    {
      GuardedPointer *newPtr;
      if (!flistHead)
      {
        void *chunk = std::malloc(Policy::BlockSize * sizeof(GuardedPointer) + sizeof(GuardedPointerBlock));
        GuardedPointerBlock *newBlock = ::new (chunk) GuardedPointerBlock();

        GuardedPointerBlock *head = blockHead.exchange(newBlock, std::memory_order_relaxed);
        newBlock->next.reset(head);
        flistHead = newBlock->begin();
      }

      newPtr = flistHead;
      flistHead = newPtr->getNext();
      newPtr->setNext(nullptr);
      return newPtr;
    }

    // Put HazardPointer on free list for reuse
    void freePtr(GuardedPointer *p)
    {
      p->setPtr(nullptr);
      p->setNext(flistHead);
      p->markAsDeleted();
      flistHead = p;
      HazardPointerManager::get().nbhp.fetch_sub(1, std::memory_order_relaxed);
    }

    // Return a vector of all the Hazard Pointer contained in the GuardedPointerBlock linked list
    void getHpsPriv(std::vector<T *> &output, GuardedPointerBlock *block)
    {
      for (auto &&g : *block)
      {
        if (!g.isDeleted())
          output.push_back(g.ptr.load());
      }
      if (block->next)
        getHpsPriv(output, block->next.get());
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
    // Retrive the unique instance of the HazardPointerManager
    static HazardPointerManager &get()
    {
      static HazardPointerManager m;
      return m;
    }

    ~HazardPointerManager()
    {
      delete head.load(std::memory_order_relaxed);
    }

    std::atomic<HazardPointerRecord *> head;
    std::atomic<int> nbhp;

    /**
     * Subscribe a thread to the manager. During the time the thread is subscribed, it is
     * given a HazardPointerRecord object that allow it to protect 'Policy::BlockSize' pointers.
     */
    void subscribe()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, Policy>::getMyhp();

      // Prevent subbing more than once
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
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, Policy>::getMyhp();

      // Prevent unsubscribe more than once
      if (!myhp.get())
        return;

      myhp->active.store(false, std::memory_order_relaxed);
      myhp.release();
    }

    /**
     * Retrieve the threshold of the manager before scanning is required
     */
    unsigned int getBatchSize()
    {
      return nbhp.load(std::memory_order_relaxed) * 2 + Policy::BlockSize;
    }

    /**
     * Scan function that is called to garbage collect the memory.
     */
    void scan()
    {
      // Stage 1
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, Policy>::getMyhp();
      std::vector<T *> plist;
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
      GuardedPointer *localRList = myhp->rlistHead;
      myhp->rlistHead = nullptr;

      // Stage 3
      GuardedPointer *next;
      for (auto &&g = localRList; g;)
      {
        next = g->getNext();
        if (std::binary_search(plist.begin(), plist.end(), g->ptr.load(std::memory_order_relaxed)))
          myhp->retire(g);
        else
          myhp->free(g);
        g = next;
      }
    }

    void helpScan()
    {
      std::unique_ptr<HazardPointerRecord> &myhp = HazardPointer<T, Policy>::getMyhp();
      bool expected = false;
      for (auto &&i = head.load(std::memory_order_relaxed); i; i = i->next.get())
      {
        // Trying to lock the next non-used hazard pointer record
        if (i->active.load(std::memory_order_relaxed) ||
            !i->active.compare_exchange_strong(expected, true,
                                               std::memory_order_acquire, std::memory_order_relaxed))
          continue;

        // Inserting the rlist of the node in myhp
        for (auto &&g = i->rlistHead; g; g = g->getNext())
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

  // Thread local storage
  static std::unique_ptr<HazardPointerRecord> &getMyhp()
  {
    static thread_local std::unique_ptr<HazardPointerRecord> myhp;
    return myhp;
  }
};
} // namespace DNFC

#endif
