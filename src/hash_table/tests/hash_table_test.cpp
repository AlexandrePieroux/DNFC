#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <pthread.h>
#include <gtest/gtest.h>

#include "../hashtable.hpp"

using namespace DNFC;

/**
 * Standard tests part
 */
class TestHashTablePolicy : public DefaultHashTablePolicy
{
    public:
    const static std::size_t BlockSize = 2;
};

TEST(HashTableTest, SimpleInsert)
{
    HashTable<int, int, TestHashTablePolicy> hm;
    int valOne = 1;
    EXPECT_TRUE(hm.insert(50, valOne));
}

TEST(HashTableTest, GetWhatWeStored)
{
    HashTable<int, int, TestHashTablePolicy> hm;
    int valOne = 1;

    hm.insert(50, valOne);
    EXPECT_EQ(hm.get(50), valOne);
}

TEST(HashTableTest, GetNonInsertedItem)
{
    HashTable<int, int, TestHashTablePolicy> hm;
    EXPECT_EQ(hm.get(50), int{});
}

TEST(HashTableTest, RemoveWhatIsInserted)
{
    HashTable<int, int, TestHashTablePolicy> hm;
    int valOne = 1;

    hm.insert(50, valOne);
    EXPECT_TRUE(hm.remove(50));
    EXPECT_EQ(hm.get(50), int{});
}

TEST(HashTableTest, RemoveNonInsertedItem)
{
    HashTable<int, int, TestHashTablePolicy> hm;
    EXPECT_FALSE(hm.remove(50));
}

TEST(HashTableTest, TriggerResizingOnInsert)
{
    HashTable<int, int, TestHashTablePolicy> hm;
    for(int i = 0; i < 16; i++)
        hm.insert(i, i);
    for(int j = 0; j < 16; j++)
        EXPECT_EQ(hm.get(j), j);
}
// Standard tests part

/**
 * Stress tests part
 */
const int nbThreads = 1024;

TEST(HashTableTest, StressInsert)
{
    std::vector<std::thread> workers;
    DNFC::HashTable<int, int> hm;

    for (int i = 0; i < nbThreads; i++)
    {
        workers.push_back(std::thread([&hm, i] {
            EXPECT_TRUE(hm.insert(i, i));
        }));
    }

    for (auto &worker : workers)
        worker.join();
}

TEST(HashTableTest, StressGet)
{
    std::vector<std::thread> workers;
    std::set<long long> thread_ids;
    DNFC::HashTable<long long, long long> hm;

    for (int i = 0; i < nbThreads; ++i)
    {
        workers.push_back(std::thread([&hm, i] {
            EXPECT_TRUE(hm.insert(i, i));
        }));
    }

    for (int i = 0; i < nbThreads; ++i)
    {
        workers.push_back(std::thread([&hm, i] {
            EXPECT_EQ(hm.get(i), i);
        }));
    }

    for (auto &worker : workers)
        worker.join();
}

TEST(HashTableTest, StressRemove)
{
    std::vector<std::thread> workers;
    DNFC::HashTable<long long, long long> hm;

    for (int i = 0; i < nbThreads; ++i)
    {
        workers.push_back(std::thread([&hm, i] {
            EXPECT_TRUE(hm.insert(i, i));
        }));
    }

    for (int i = 0; i < nbThreads; ++i)
    {
        workers.push_back(std::thread([&hm, i] {
            EXPECT_TRUE(hm.remove(i));
        }));
    }

    for (auto &worker : workers)
        worker.join();
}
//Stress tests part

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
