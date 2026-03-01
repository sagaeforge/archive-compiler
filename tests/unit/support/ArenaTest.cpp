#include "kern/support/Arena.h"
#include <gtest/gtest.h>

using namespace kern;

TEST(ArenaTest, BasicAllocation) {
    Arena arena;
    int* p = arena.make<int>(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST(ArenaTest, MultipleAllocations) {
    Arena arena;
    for (int i = 0; i < 1000; ++i) {
        int* p = arena.make<int>(i);
        EXPECT_EQ(*p, i);
    }
}

TEST(ArenaTest, ArrayAllocation) {
    Arena arena;
    int* arr = arena.makeArray<int>(100);
    ASSERT_NE(arr, nullptr);
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(arr[i], i);
    }
}

struct LargeStruct {
    char data[256];
    int value;
};

TEST(ArenaTest, LargeAllocation) {
    Arena arena;
    auto* p = arena.make<LargeStruct>();
    p->value = 99;
    EXPECT_EQ(p->value, 99);
}

// --- Move constructor ---
TEST(ArenaTest, MoveConstructor) {
    Arena original;
    int* p1 = original.make<int>(42);
    int* p2 = original.make<int>(99);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);

    Arena moved(std::move(original));
    // Data allocated in original is now accessible through moved
    EXPECT_EQ(*p1, 42);
    EXPECT_EQ(*p2, 99);
    // Can still allocate from the moved-to arena
    int* p3 = moved.make<int>(7);
    EXPECT_EQ(*p3, 7);
}
