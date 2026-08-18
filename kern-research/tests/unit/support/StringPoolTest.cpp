#include "kern/support/StringPool.h"
#include <gtest/gtest.h>

using namespace kern;

class StringPoolTest : public ::testing::Test {
protected:
    Arena arena;
};

TEST_F(StringPoolTest, InternReturnsStableView) {
    StringPool pool(arena);
    auto s = pool.intern("hello");
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(s.size(), 5);
}

TEST_F(StringPoolTest, InternSameStringReturnsSamePointer) {
    StringPool pool(arena);
    auto a = pool.intern("hello");
    auto b = pool.intern("hello");
    EXPECT_EQ(a.data(), b.data());  // pointer equality
    EXPECT_EQ(a, b);
}

TEST_F(StringPoolTest, InternDifferentStringsReturnsDifferentPointers) {
    StringPool pool(arena);
    auto a = pool.intern("hello");
    auto b = pool.intern("world");
    EXPECT_NE(a.data(), b.data());
    EXPECT_NE(a, b);
}

TEST_F(StringPoolTest, SizeTracksUniqueStrings) {
    StringPool pool(arena);
    EXPECT_EQ(pool.size(), 0);
    pool.intern("a");
    EXPECT_EQ(pool.size(), 1);
    pool.intern("b");
    EXPECT_EQ(pool.size(), 2);
    pool.intern("a");  // duplicate
    EXPECT_EQ(pool.size(), 2);
}

TEST_F(StringPoolTest, ContainsChecksMembership) {
    StringPool pool(arena);
    pool.intern("foo");
    EXPECT_TRUE(pool.contains("foo"));
    EXPECT_FALSE(pool.contains("bar"));
}

TEST_F(StringPoolTest, InternEmptyString) {
    StringPool pool(arena);
    auto s = pool.intern("");
    EXPECT_EQ(s, "");
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(pool.size(), 1);

    auto s2 = pool.intern("");
    EXPECT_EQ(s.data(), s2.data());
}

TEST_F(StringPoolTest, ConcatTwoStrings) {
    StringPool pool(arena);
    auto s = pool.intern("foo", "bar");
    EXPECT_EQ(s, "foobar");
    EXPECT_EQ(s.size(), 6);
}

TEST_F(StringPoolTest, ConcatTwoDeduplicates) {
    StringPool pool(arena);
    auto a = pool.intern("foobar");
    auto b = pool.intern("foo", "bar");
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.data(), b.data());  // same interned pointer
    EXPECT_EQ(pool.size(), 1);
}

TEST_F(StringPoolTest, ConcatThreeStrings) {
    StringPool pool(arena);
    auto s = pool.intern("a", "b", "c");
    EXPECT_EQ(s, "abc");
    EXPECT_EQ(s.size(), 3);
}

TEST_F(StringPoolTest, ConcatThreeDeduplicates) {
    StringPool pool(arena);
    auto a = pool.intern("abc");
    auto b = pool.intern("a", "b", "c");
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.data(), b.data());
    EXPECT_EQ(pool.size(), 1);
}

TEST_F(StringPoolTest, InternFromTemporaryBuffer) {
    StringPool pool(arena);
    std::string temp = "temporary";
    auto s = pool.intern(temp);
    temp[0] = 'X';  // mutate original
    EXPECT_EQ(s, "temporary");  // interned copy is independent
}

TEST_F(StringPoolTest, ManyStrings) {
    StringPool pool(arena);
    for (int i = 0; i < 1000; ++i) {
        auto s = std::to_string(i);
        pool.intern(s);
    }
    EXPECT_EQ(pool.size(), 1000);

    // All should still be findable
    for (int i = 0; i < 1000; ++i) {
        auto s = std::to_string(i);
        EXPECT_TRUE(pool.contains(s));
    }
}

TEST_F(StringPoolTest, ConcatWithEmptyParts) {
    StringPool pool(arena);
    auto a = pool.intern("hello", "");
    EXPECT_EQ(a, "hello");
    auto b = pool.intern("", "hello");
    EXPECT_EQ(b, "hello");
    EXPECT_EQ(a.data(), b.data());
}
