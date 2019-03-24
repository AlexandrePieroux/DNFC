#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>

#include "../hazardpointer.hpp"

using namespace DNFC;

class TestPolicy : DNFC::DefaultHazardPointerPolicy
{
  public:
    const static int BlockSize = 2;
};

TEST(HazardPointer, Declaration)
{
    DNFC::HazardPointer<int, TestPolicy> pointer;
    EXPECT_EQ(pointer.get(), nullptr);
}

TEST(HazardPointer, Get)
{
    int *val = new int(1);
    DNFC::HazardPointer<int, TestPolicy> pointer(val);
    EXPECT_EQ(pointer.get(), val);
}

TEST(HazardPointer, SetPointerByCopy)
{
    int *val = new int(1);
    DNFC::HazardPointer<int, TestPolicy> pointer;
    pointer = val;
    EXPECT_EQ(pointer.get(), val);
}

TEST(HazardPointer, SetPointerByMoving)
{
    int *val = new int(1);
    DNFC::HazardPointer<int, TestPolicy> pointer;
    pointer = std::move(val);
    EXPECT_EQ(pointer.get(), val);
}

TEST(HazardPointer, Release)
{
    int *val = new int(1);
    DNFC::HazardPointer<int, TestPolicy> pointer(val);
    pointer.release();
    EXPECT_EQ(pointer.get(), nullptr);
    EXPECT_EQ(*val, 1);
}

TEST(HazardPointer, Retire)
{
    int *val = new int(1);
    DNFC::HazardPointer<int, TestPolicy> pointer(val);
    pointer.retire();
    EXPECT_EQ(pointer.get(), nullptr);
}

TEST(HazardPointer, SetPointerWithAnotherHazardPointer)
{
    int *val = new int(1);
    DNFC::HazardPointer<int, TestPolicy> pointer;
    DNFC::HazardPointer<int, TestPolicy> another(val);
    pointer = another;
    EXPECT_EQ(pointer.get(), val);
}

TEST(HazardPointer, TriggerNewMemoryBlockCreation)
{
    int *valOne = new int(1);
    int *valTwo = new int(2);
    int *valThree = new int(3);
    int *valFour = new int(4);
    int *valFive = new int(5);
    int *valSix = new int(6);

    DNFC::HazardPointer<int, TestPolicy> one(valOne);
    DNFC::HazardPointer<int, TestPolicy> two(valTwo);
    DNFC::HazardPointer<int, TestPolicy> three(valThree);
    DNFC::HazardPointer<int, TestPolicy> four(valFour);
    DNFC::HazardPointer<int, TestPolicy> five(valFive);
    DNFC::HazardPointer<int, TestPolicy> six(valSix);

    EXPECT_EQ(one.get(), valOne);
    EXPECT_EQ(two.get(), valTwo);
    EXPECT_EQ(three.get(), valThree);
    EXPECT_EQ(four.get(), valFour);
    EXPECT_EQ(five.get(), valFive);
    EXPECT_EQ(six.get(), valSix);
}

TEST(HazardPointer, PreventDeletionOfGuardedPointerWhileScanning)
{
    int *valOne = new int(1);

    DNFC::HazardPointer<int, TestPolicy> one(valOne);
    DNFC::HazardPointer<int, TestPolicy> two(new int(2));
    DNFC::HazardPointer<int, TestPolicy> three(new int(3));
    DNFC::HazardPointer<int, TestPolicy> four(new int(4));
    DNFC::HazardPointer<int, TestPolicy> five(new int(5));

    std::mutex m;
    std::condition_variable cv;
    bool notified = false;
    std::thread worker([&valOne, &notified, &m, &cv]()
    {
        DNFC::HazardPointer<int, TestPolicy> threadHP(valOne);

        std::unique_lock<std::mutex> lk(m);
        lk.unlock();
        cv.notify_one();

        while(!notified)
            cv.wait(lk);
    });

    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk);

        one.retire();
        two.retire();
        three.retire();
        four.retire();
        five.retire();

        notified = true;
        cv.notify_one();
    }
    worker.join();

    EXPECT_EQ(*valOne, 1);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
