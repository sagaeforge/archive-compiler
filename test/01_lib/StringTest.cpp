#include "01_lib/String.h"

#include <gtest/gtest.h>

namespace nugdev::test {

TEST(String, Constructor) {
  nugdev::lib::String str;
  EXPECT_EQ(str.length(), 0);
}

TEST(String, ConstructorWithChar) {
  nugdev::lib::String str('a');
  EXPECT_EQ(str.length(), 1);
  EXPECT_EQ(str.charAt(0), 'a');
}

TEST(String, ConstructorWithString) {
  nugdev::lib::String str("hello");
  EXPECT_EQ(str.length(), 5);
  EXPECT_EQ(str.charAt(0), 'h');
}

TEST(String, ConstructorWithWString) {
  nugdev::lib::String str(L"hello");
  EXPECT_EQ(str.length(), 5);
  EXPECT_EQ(str.charAt(0), 'h');
}

TEST(String, ConstructorWithStringView) {
  nugdev::lib::String str("hello");
  EXPECT_EQ(str.length(), 5);
  EXPECT_EQ(str.charAt(0), 'h');
}

TEST(String, ConstructorWithUnicodeString) {
  nugdev::lib::String str(u"hello");
  EXPECT_EQ(str.length(), 5);
  EXPECT_EQ(str.charAt(0), 'h');
}

TEST(String, ConstructorWithVectorOfStringAndDelimiter) {
  std::vector<nugdev::lib::String> strs = {"hello", "world"};
  nugdev::lib::String str(strs, " ");
  EXPECT_EQ(str.length(), 11);
  EXPECT_EQ(str.charAt(0), 'h');
}

TEST(String, ToString) {
  nugdev::lib::String str("hello");
  EXPECT_EQ(str.to_string(), "hello");
}

TEST(String, ToVector) {
  nugdev::lib::String str("hello");
  std::vector<nugdev::lib::String::char_t> vec = str.to_vector();
  EXPECT_EQ(vec.size(), 5);
  EXPECT_EQ(vec[0], 'h');
}

TEST(String, Slice) {
  nugdev::lib::String str("hello");
  nugdev::lib::String slice = str.slice(0, 5);
  EXPECT_EQ(slice.length(), 5);
}

TEST(String, Split) {
  nugdev::lib::String str("hello world");
  std::vector<nugdev::lib::String> vec = str.split(" ");
  EXPECT_EQ(vec.size(), 2);
  EXPECT_EQ(vec[0], "hello");
  EXPECT_EQ(vec[1], "world");
}

} // namespace nugdev::test
