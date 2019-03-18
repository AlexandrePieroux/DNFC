#ifndef _MEMORYPOOLH_
#define _MEMORYPOOLH_

#include <memory>
#include <new>

namespace DNFC
{
template <typename T, std::size_t max>
struct pool
{
  public:
    pool(pool const &) = delete;
    pool &operator=(pool const &) = delete;

    pool()
    {
        allocBlock();
    }

    pool(pool &&o) noexcept
    {
        swap(o);
    }

    pool &operator=(pool &&o) noexcept
    {
        swap(o);
        return *this;
    }

    void swap(pool &other) noexcept
    {
        std::swap(head, other.m_head);
        std::swap(max, other.max);
    }

    T *alloc()
    {
        if (head == nullptr)
            allocBlock();

        node *result = head;
        head = result->next;

        return reinterpret_cast<T *>(result);
    }

    void free(T *ptr)
    {
        ptr->T::~T();
        node *newPtr = reinterpret_cast<node *>(ptr);
        newPtr->next = head;
        head = newPtr;
    }

  private:
    union node {
        alignas(alignof(T)) char data[sizeof(T)];
        node *next;
    };

    node *head = nullptr;

    void allocBlock()
    {
        uint64_t allocSize = sizeof(T) * max;
        head = reinterpret_cast<node *>(std::malloc(allocSize));
        if (head == nullptr)
            throw std::bad_alloc();

        for (int i = 1; i < allocSize; i++)
            head[i - 1].next = &head[i];

        head[allocSize - 1].next = nullptr;
    }
};
} // namespace DNFC

#endif
