#ifndef _MEMORYPOOLH_
#define _MEMORYPOOLH_

namespace DNFC
{
template <typename T>
class MemoryPool
{
  public:
    template <typename... Args>
    T *alloc(Args &&... args)
    {
        if (freeList == nullptr)
        {
            std::unique_ptr<arena> newArena(new arena(arenaSize));
            newArena->setNext(std::move(currentArena));
            currentArena.reset(newArena.release());
            freeList = currentArena->load();
        }

        chunk *currentItem = freeList;
        freeList = currentItem->getNext();
        T *result = currentItem->load();

        new (result) T(std::forward<Args>(args)...);

        return result;
    }

    void free(T *t)
    {
        t->T::~T();
        chunk *currentItem = chunk::storageToItem(t);
        currentItem->setNext(freeList);
        freeList = currentItem;
    }

  private:
    union chunk {
      public:
        chunk *getNext() const
        {
            return next;
        }

        void setNext(chunk *n)
        {
            next = n;
        }

        T *load()
        {
            return reinterpret_cast<T *>(data);
        }

        static chunk *storageToItem(T *t)
        {
            return reinterpret_cast<chunk *>(t);
        }

      private:
        using Type = alignas(alignof(T)) char[sizeof(T)];
        Type data;
        chunk *next;
    }; // chunk

    struct arena
    {
      private:
        std::unique_ptr<chunk[]> storage;
        std::unique_ptr<arena> next;

        arena(std::size_t size) : storage(new chunk[size])
        {
            for (std::size_t i = 1; i < size; i++)
                storage[i - 1].setNext(&storage[i]);
            storage[size - 1].setNext(nullptr);
        }

        chunk *load() const
        {
            return storage.get();
        }

        void setNext(std::unique_ptr<arena> &&n)
        {
            assert(!next);
            next.reset(n.release());
        }
    }; // arena

    std::size_t arenaSize;
    std::unique_ptr<arena> currentArena;
    chunk *freeList;
};
} // namespace DNFC

#endif
