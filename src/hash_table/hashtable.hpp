#ifndef _HASH_TABLEH_
#define _HASH_TABLEH_

#include <cstddef>
#include <cmath>
#include "../SMR/hazardpointer.hpp"

namespace DNFC
{

/**
 * HashTable default policy
 */
class DefaultHashTablePolicy
{
  public:
    const static std::size_t BlockSize = 64; // Should be a power of two
    const static std::size_t MaxFailCount = 4;
};

template <typename Key, typename Data, typename Policy = DefaultHashTablePolicy>
class HashTable
{
  public:
    bool insert(Key key, Data data)
    {
        std::size_t hash = hashKey(key);
        Node *insertThis = new Node(data, hash);
        ArrayNode local = head;

        DNFC::HazardPointer<Node> nodeHP;
        Item current;

        for (int R = 0; R < keySize; R += arrayNodePow)
        {
            std::size_t failCount = 0;
            int pos = hash & (Policy::BlockSize - 1);
            hash = hash >> arrayNodePow;

            std::atomic<Item> &item = local[pos];
            for (;;)
            {
                try
                {
                    current = item.load(std::memory_order_relaxed);
                    if (!current)
                    {
                        if (item.compare_exchange_strong(current, insertThis,
                                                         std::memory_order_acquire, std::memory_order_relaxed))
                            return true;
                        else
                            throw ContentionException();
                    }
                    else if (isArrayNode(current))
                    {
                        local = toArrayNode(current);
                        break;
                    }
                    else
                    {
                        guard(nodeHP, item, toNode(current));
                        if (isMarked(current))
                        {
                            if (item.compare_exchange_strong(current, insertThis,
                                                             std::memory_order_acquire, std::memory_order_relaxed))
                            {
                                nodeHP.retire();
                                return true;
                            }
                            else
                            {
                                nodeHP.release();
                                throw ContentionException();
                            }
                        }
                        else if (toNode(current)->hash == insertThis->hash)
                        {
                            nodeHP.release();
                            delete insertThis;
                            return false;
                        }
                        else
                        {
                            local = expandTable(item, R);
                            nodeHP.release();
                        }
                    }
                }
                catch (ContentionException &e)
                {
                    if (failCount++ > Policy::MaxFailCount)
                    {
                        local = expandTable(item, R);
                        failCount = 0;
                    }
                    continue;
                }
            }
        }
        return false;
    }

    Data get(Key key)
    {
        std::size_t hashValue = hashKey(key);
        std::size_t hash = hashValue;
        ArrayNode local = head;
        DNFC::HazardPointer<Node> nodeHP;
        Item current;

        for (int R = 0; R < keySize; R += arrayNodePow)
        {
            std::size_t failCount = 0;
            int pos = hash & (Policy::BlockSize - 1);
            hash = hash >> arrayNodePow;

            std::atomic<Item> &item = local[pos];
            for (;;)
            {
                try
                {
                    current = item.load(std::memory_order_relaxed);
                    if (isArrayNode(current))
                    {
                        local = toArrayNode(current);
                        break;
                    }
                    else
                    {
                        if (isMarked(current))
                            return Data{};

                        Node *node = toNode(current);
                        guard(nodeHP, item, node);
                        if (node && node->hash == hashValue)
                        {

                            Data res = node->data;
                            nodeHP.release();
                            return res;
                        }
                        else
                            return Data{};
                    }
                }
                catch (ContentionException &e)
                {
                    if (failCount++ > Policy::MaxFailCount)
                    {
                        local = expandTable(item, R);
                        failCount = 0;
                    }
                    continue;
                }
            }
        }
        return Data{};
    }

    bool remove(Key key)
    {
        std::size_t hashValue = hashKey(key);
        std::size_t hash = hashValue;
        ArrayNode local = head;
        DNFC::HazardPointer<Node> nodeHP;
        Item current;

        for (int R = 0; R < keySize; R += arrayNodePow)
        {
            std::size_t failCount = 0;
            int pos = hash & (Policy::BlockSize - 1);
            hash = hash >> arrayNodePow;

            std::atomic<Item> &item = local[pos];
            for (;;)
            {
                try
                {
                    current = item.load(std::memory_order_relaxed);
                    if (!current)
                        return false;
                    else if (isArrayNode(current))
                    {
                        local = toArrayNode(current);
                        break;
                    }
                    else
                    {
                        guard(nodeHP, item, toNode(current));
                        if (toNode(current)->hash == hashValue && markToDelete(item, current))
                        {
                            uintptr_t ptrCast = reinterpret_cast<uintptr_t>(current);
                            Item ptrMarked = reinterpret_cast<Item>(ptrCast | 0x1);
                            if (item.compare_exchange_strong(ptrMarked, nullptr,
                                                             std::memory_order_acquire, std::memory_order_relaxed))
                                nodeHP.retire();
                            return true;
                        }
                    }
                }
                catch (ContentionException &e)
                {
                    if (failCount++ > Policy::MaxFailCount)
                    {
                        local = expandTable(item, R);
                        failCount = 0;
                    }
                    continue;
                }
                return false;
            }
        }
        return false;
    }

    HashTable<Key, Data, Policy>() : keySize(sizeof(Key) * 8),
                                   head(new std::atomic<Item>[Policy::BlockSize]())
    {
        std::size_t policyBlockSize = Policy::BlockSize;
        int arrayNodePowTmp = 0;
        while (policyBlockSize >>= 1)
            ++arrayNodePowTmp;
        arrayNodePow = arrayNodePowTmp;
    }

  private:
    friend class DefaultHashTablePolicy;

    using Item = void *;
    using ArrayNode = std::atomic<Item> *;
    struct Node
    {
        Data data;
        std::size_t hash;
        std::atomic<int> accessCount;

        Node(Data d, std::size_t h) : data(d), hash(h), accessCount(0) {}
    };

    /**
     * Traversal operations
     */
    bool isArrayNode(Item ptr)
    {
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(ptr);
        return (ptrCast & 0x2) == 2;
    }

    Node *toNode(Item ptr)
    {
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(ptr);
        return reinterpret_cast<Node *>(ptrCast & ~0x3);
    }

    ArrayNode toArrayNode(Item ptr)
    {
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(ptr);
        return reinterpret_cast<ArrayNode>(ptrCast & ~0x3);
    }

    /**
     * Memory operations
     */
    struct ContentionException : public std::exception
    {
    };

    void guard(DNFC::HazardPointer<Node> &hp, std::atomic<Item> &item, Node *expected)
    {
        hp = toNode(item.load(std::memory_order_relaxed));
        if (hp.get() != expected)
            throw ContentionException();
    }

    bool markToDelete(std::atomic<Item> &item, Item expected)
    {
        // Mark the pointer as being an ArrayNode
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(expected);
        Item ptrMarked = reinterpret_cast<Item>(ptrCast | 0x1);
        if (item.compare_exchange_strong(expected, ptrMarked,
                                         std::memory_order_acquire, std::memory_order_relaxed))
            return true;
        return false;
    }

    bool isMarked(Item ptr)
    {
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(ptr);
        return (ptrCast & 0x1);
    }

    Item unmark(Item ptr)
    {
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(ptr);
        return reinterpret_cast<Item>(ptrCast & ~0x1);
    }

    /**
     * Expand operations
     */
    ArrayNode expandTable(std::atomic<Item> &ptr, int depth)
    {
        // Checking that expansion need to be done
        Item ptrValue = ptr.load(std::memory_order_relaxed);
        if (isArrayNode(ptrValue))
            return toArrayNode(ptr.load(std::memory_order_relaxed));

        // Create block and reinsert the node in it
        ArrayNode newBlock = new std::atomic<Item>[Policy::BlockSize]();
        std::size_t hash = toNode(ptrValue)->hash >> (depth + arrayNodePow);
        int pos = hash & (Policy::BlockSize - 1);
        newBlock[pos] = ptrValue;

        // Mark the pointer as being an ArrayNode
        uintptr_t ptrCast = reinterpret_cast<uintptr_t>(newBlock);
        Item ptrMarked = reinterpret_cast<Item>(ptrCast | 0x2);

        // Try to insert it in
        Item expected = ptr.load(std::memory_order_relaxed);
        if (ptr.compare_exchange_strong(expected, ptrMarked,
                                        std::memory_order_acquire, std::memory_order_relaxed))
            return newBlock;

        // Attempt failed
        delete[] newBlock;
        return toArrayNode(ptr.load(std::memory_order_relaxed));
    }

    ArrayNode head;
    std::size_t arrayNodePow;
    std::size_t keySize;
    std::hash<Key> hashKey;
};
} // namespace DNFC

#endif
