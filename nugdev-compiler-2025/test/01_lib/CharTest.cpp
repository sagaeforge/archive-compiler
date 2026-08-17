#include <gtest/gtest.h>

#include "01_lib/Char.h"

using namespace nugdev::lib;

TEST(CharTest, isDigit) {
  EXPECT_TRUE(Char('0').isDigit());
  EXPECT_TRUE(Char('9').isDigit());
  EXPECT_FALSE(Char('a').isDigit());
  EXPECT_FALSE(Char('A').isDigit());
}

TEST(CharTest, isAlpha) {
  EXPECT_TRUE(Char('a').isAlpha());
  EXPECT_TRUE(Char('z').isAlpha());
  EXPECT_TRUE(Char('A').isAlpha());
  EXPECT_TRUE(Char('Z').isAlpha());
  EXPECT_FALSE(Char('0').isAlpha());
  EXPECT_FALSE(Char('9').isAlpha());
}

TEST(CharTest, isUpper) {
  EXPECT_TRUE(Char('A').isUpper());
  EXPECT_FALSE(Char('a').isUpper());
  EXPECT_FALSE(Char('0').isUpper());
}

TEST(CharTest, isLower) {
  EXPECT_TRUE(Char('a').isLower());
  EXPECT_FALSE(Char('A').isLower());
  EXPECT_FALSE(Char('0').isLower());
}

TEST(CharTest, isAlnum) {
  EXPECT_TRUE(Char('0').isAlnum());
  EXPECT_TRUE(Char('9').isAlnum());
  EXPECT_TRUE(Char('a').isAlnum());
  EXPECT_TRUE(Char('z').isAlnum());
  EXPECT_TRUE(Char('A').isAlnum());
  EXPECT_TRUE(Char('Z').isAlnum());
}

TEST(CharTest, isSpace) {
  EXPECT_TRUE(Char(' ').isSpace());
  EXPECT_TRUE(Char('\t').isSpace());
  EXPECT_TRUE(Char('\n').isSpace());
  EXPECT_TRUE(Char('\r').isSpace());
  EXPECT_TRUE(Char('\f').isSpace());
  EXPECT_TRUE(Char('\v').isSpace());
  EXPECT_FALSE(Char('a').isSpace());
}

TEST(CharTest, to_digit) {
  EXPECT_EQ(Char('0').to_digit(), 0);
  EXPECT_EQ(Char('9').to_digit(), 9);
}

TEST(CharTest, to_char) {
  EXPECT_EQ(Char('0').to_char(), '0');
  EXPECT_EQ(Char('9').to_char(), '9');
}

TEST(CharTest, to_upper) {
  EXPECT_EQ(Char('a').to_upper(), 'A');
  EXPECT_EQ(Char('z').to_upper(), 'Z');
  EXPECT_EQ(Char('A').to_upper(), 'A');
  EXPECT_EQ(Char('Z').to_upper(), 'Z');
}

TEST(CharTest, to_lower) {
  EXPECT_EQ(Char('a').to_lower(), 'a');
  EXPECT_EQ(Char('z').to_lower(), 'z');
  EXPECT_EQ(Char('A').to_lower(), 'a');
  EXPECT_EQ(Char('Z').to_lower(), 'z');
}

TEST(CharTest, get_char) {
  EXPECT_EQ(Char('a').get_char(), 'a');
  EXPECT_EQ(Char('z').get_char(), 'z');
}
