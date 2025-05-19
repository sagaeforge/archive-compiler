#include "main.h"
#include <gtest/gtest.h>

TEST(AddTest, PositiveNumbers) {
  EXPECT_EQ(add(1, 2), 3);
  EXPECT_EQ(add(10, 20), 30);
}

TEST(AddTest, NegativeNumbers) {
  EXPECT_EQ(add(-1, -2), -3);
  EXPECT_EQ(add(-10, -20), -30);
}

TEST(AddTest, MixedNumbers) {
  EXPECT_EQ(add(-1, 1), 0);
  EXPECT_EQ(add(10, -5), 5);
}