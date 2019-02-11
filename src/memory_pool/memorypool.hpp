#ifndef _MEMORYPOOLH_
#define _MEMORYPOOLH_

namespace DNFC
{
template <typename T>
class MemoryPool
{
  public:
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
    };

    struct chunkBlob
    {
      private:
        std::unique_ptr<chunk[]> storage;
        std::unique_ptr<chunkBlob> next;

        chunkBlob(std::size_t size) : storage(new chunk[size])
        {
            for (std::size_t i = 1; i < size; i++)
                storage[i - 1].setNext(&storage[i]);
            storage[size - 1].setNext(nullptr);
        }

        chunk *load() const
        {
            return storage.get();
        }

        void setNext(std::unique_ptr<chunkBlob> &&n)
        {
            assert(!next);
            next.reset(n.release());
        }
    }; // chunkBlob
};
} // namespace DNFC

#endif
