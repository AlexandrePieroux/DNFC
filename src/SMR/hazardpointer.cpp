#include "hazardpointer.hpp"

template <typename T>

thread_local HazardPointer<T>::HazardPointerRecord *myhp;

inline unsigned int HazardPointer<T>::get_batch_size()
{
      return (this->nbhp.load(std::memory_order_relaxed) * this->nbpointers * 2) + this->nbhp.load(std::memory_order_relaxed);
};

void HazardPointer<T>::subscribe()
{
      // First try to reuse a retire HP record
      bool expected = false;
      for (HazardPointerRecord *i = this->head; i; i = i->next)
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
      myhp = new HazardPointerRecord();
      myhp->next = this->head.load(std::memory_order_relaxed);
      myhp->active = true;

      // Add the new record to the list
      while (!this->head.compare_exchange_weak(myhp->next, myhp,
                                               std::memory_order_release,
                                               std::memory_order_relaxed))
            ;
}

void HazardPointer<T>::unsubscribe()
{
      if (!myhp)
            return;
      myhp->hp.clear();
      myhp->active = false;
}

T *HazardPointer<T>::get_pointer(int index)
{
      if (!myhp || index >= this->nbpointers)
            return NULL;
      return &(myhp)->hp[index];
}

void HazardPointer<T>::delete_node(void *node)
{
      if (!myhp)
            return;
      myhp->rList.push_front(node);
      if (myhp->rList.size >= this->get_batch_size())
      {
            this->scan();
            this->help_scan();
      }
}

/*                Private function                     */

void HazardPointer<T>::scan()
{
      // Stage 1
      std::vector<void *> plist;
      for (HazardPointerRecord *i = this->head.load(std::memory_order_relaxed); i; i = i->next)
      {
            for (const std::atomic<T *> &e : i->hp)
            {
                  std::atomic<T *> hp = e->load(std::memory_order_relaxed);
                  if (hp)
                        plist.push_back(hp);
            }
      }
      if (plist.size <= 0)
            return;

      // Stage 2
      std::sort(plist, plist);
      std::list<T *> localRList = myhp->rList;
      myhp->rList.clear();

      // Stage 3
      for (const T* &e : localRList)
      {
            if (std::binary_search(plist.begin(), plist.end(), *e))
                  myhp->rList.push_front(*e);
            else
                  delete *e;
      }
}

void HazardPointer<T>::help_scan()
{
      bool expected = false;
      for (HazardPointerRecord *i = this->head.load(std::memory_order_relaxed; i; i = i->next)
      {
            // Trying to lock the next non-used hazard pointer record
            if (i->active.load(std::memory_order_relaxed) ||
                !i->active.compare_exchange_strong(expected, true,
                                                   std::memory_order_acquire, std::memory_order_relaxed))
                  continue;

            // Inserting the rList of the node in myhp
            for (const T* &e : i->rList)
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

/*                Private function                     */
