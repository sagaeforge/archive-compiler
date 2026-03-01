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
