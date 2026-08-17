#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class LiteralsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// NumberLiteral Tests
TEST_F(LiteralsTest, NumberLiteralDecimalInteger) {
  auto number = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);

  EXPECT_EQ(number->get_literal_value(), "42");
  EXPECT_EQ(number->get_number_type(),
            NumberLiteral::NumberType::DECIMAL_INTEGER);
  EXPECT_EQ(number->get_node_type(), NodeType::NUMBER_LITERAL);
  EXPECT_EQ(number->to_string(), "NumberLiteral(42)");
  EXPECT_TRUE(number->is_integer());
  EXPECT_FALSE(number->is_floating_point());
}

TEST_F(LiteralsTest, NumberLiteralBinary) {
  auto number = std::make_unique<NumberLiteral>(
      "0b1010", NumberLiteral::NumberType::BINARY_INTEGER);

  EXPECT_EQ(number->get_literal_value(), "0b1010");
  EXPECT_EQ(number->get_number_type(),
            NumberLiteral::NumberType::BINARY_INTEGER);
  EXPECT_TRUE(number->is_integer());
}

TEST_F(LiteralsTest, NumberLiteralHexadecimal) {
  auto number = std::make_unique<NumberLiteral>(
      "0xFF", NumberLiteral::NumberType::HEXADECIMAL_INTEGER);

  EXPECT_EQ(number->get_literal_value(), "0xFF");
  EXPECT_EQ(number->get_number_type(),
            NumberLiteral::NumberType::HEXADECIMAL_INTEGER);
  EXPECT_TRUE(number->is_integer());
}

TEST_F(LiteralsTest, NumberLiteralFloatingPoint) {
  auto number = std::make_unique<NumberLiteral>(
      "3.14159", NumberLiteral::NumberType::FLOATING_POINT);

  EXPECT_EQ(number->get_literal_value(), "3.14159");
  EXPECT_EQ(number->get_number_type(),
            NumberLiteral::NumberType::FLOATING_POINT);
  EXPECT_FALSE(number->is_integer());
  EXPECT_TRUE(number->is_floating_point());
}

// StringLiteral Tests
TEST_F(LiteralsTest, StringLiteralSimple) {
  auto str = std::make_unique<StringLiteral>("Hello, World!",
                                             StringLiteral::StringType::SIMPLE);

  EXPECT_EQ(str->get_literal_value(), "Hello, World!");
  EXPECT_EQ(str->get_string_type(), StringLiteral::StringType::SIMPLE);
  EXPECT_EQ(str->get_node_type(), NodeType::STRING_LITERAL);
  EXPECT_EQ(str->to_string(), "StringLiteral(\"Hello, World!\")");
  EXPECT_FALSE(str->is_template());
}

TEST_F(LiteralsTest, StringLiteralRaw) {
  auto str = std::make_unique<StringLiteral>("C:\\path\\to\\file",
                                             StringLiteral::StringType::RAW);

  EXPECT_EQ(str->get_literal_value(), "C:\\path\\to\\file");
  EXPECT_EQ(str->get_string_type(), StringLiteral::StringType::RAW);
  EXPECT_FALSE(str->is_template());
}

TEST_F(LiteralsTest, StringLiteralTemplate) {
  auto str = std::make_unique<StringLiteral>(
      "Hello, ${name}!", StringLiteral::StringType::TEMPLATE);

  EXPECT_EQ(str->get_literal_value(), "Hello, ${name}!");
  EXPECT_EQ(str->get_string_type(), StringLiteral::StringType::TEMPLATE);
  EXPECT_TRUE(str->is_template());
}

// CharacterLiteral Tests
TEST_F(LiteralsTest, CharacterLiteral) {
  auto ch = std::make_unique<CharacterLiteral>('a');

  EXPECT_EQ(ch->get_literal_value(), "a");
  EXPECT_EQ(ch->get_character_value(), 'a');
  EXPECT_EQ(ch->get_node_type(), NodeType::CHARACTER_LITERAL);
}

TEST_F(LiteralsTest, CharacterLiteralEscape) {
  auto ch = std::make_unique<CharacterLiteral>('\n');

  EXPECT_EQ(ch->get_character_value(), '\n');
}

// BooleanLiteral Tests
TEST_F(LiteralsTest, BooleanLiteralTrue) {
  auto boolean = std::make_unique<BooleanLiteral>(true);

  EXPECT_TRUE(boolean->get_boolean_value());
  EXPECT_EQ(boolean->get_literal_value(), "true");
  EXPECT_EQ(boolean->get_node_type(), NodeType::BOOLEAN_LITERAL);
  EXPECT_EQ(boolean->to_string(), "BooleanLiteral(true)");
}

TEST_F(LiteralsTest, BooleanLiteralFalse) {
  auto boolean = std::make_unique<BooleanLiteral>(false);

  EXPECT_FALSE(boolean->get_boolean_value());
  EXPECT_EQ(boolean->get_literal_value(), "false");
  EXPECT_EQ(boolean->to_string(), "BooleanLiteral(false)");
}

// NullLiteral Tests
TEST_F(LiteralsTest, NullLiteral) {
  auto null = std::make_unique<NullLiteral>();

  EXPECT_EQ(null->get_literal_value(), "null");
  EXPECT_EQ(null->get_node_type(), NodeType::NULL_LITERAL);
  EXPECT_EQ(null->to_string(), "NullLiteral(null)");
}

// NoneLiteral Tests
TEST_F(LiteralsTest, NoneLiteral) {
  auto none = std::make_unique<NoneLiteral>();

  EXPECT_EQ(none->get_literal_value(), "None");
  EXPECT_EQ(none->get_node_type(), NodeType::NONE_LITERAL);
  EXPECT_EQ(none->to_string(), "NoneLiteral(None)");
}

// RangeLiteral Tests
TEST_F(LiteralsTest, RangeLiteralWithEnd) {
  auto start = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto end = std::make_unique<NumberLiteral>(
      "10", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto range = std::make_unique<RangeLiteral>(std::move(start), std::move(end));

  EXPECT_TRUE(range->has_end());
  EXPECT_EQ(range->get_node_type(), NodeType::RANGE_LITERAL);
}

TEST_F(LiteralsTest, RangeLiteralOpenEnded) {
  auto start = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto range = std::make_unique<RangeLiteral>(std::move(start), nullptr);

  EXPECT_FALSE(range->has_end());
}

// ArrayLiteral Tests
TEST_F(LiteralsTest, ArrayLiteralEmpty) {
  std::vector<std::unique_ptr<Expression>> elements;
  auto array = std::make_unique<ArrayLiteral>(std::move(elements));

  EXPECT_EQ(array->get_element_count(), 0);
  EXPECT_TRUE(array->is_empty());
  EXPECT_EQ(array->get_node_type(), NodeType::ARRAY_LITERAL);
  EXPECT_EQ(array->to_string(), "ArrayLiteral([0 elements])");
}

TEST_F(LiteralsTest, ArrayLiteralWithElements) {
  std::vector<std::unique_ptr<Expression>> elements;
  elements.push_back(std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER));
  elements.push_back(std::make_unique<NumberLiteral>(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER));
  elements.push_back(std::make_unique<NumberLiteral>(
      "3", NumberLiteral::NumberType::DECIMAL_INTEGER));

  auto array = std::make_unique<ArrayLiteral>(std::move(elements));

  EXPECT_EQ(array->get_element_count(), 3);
  EXPECT_FALSE(array->is_empty());
  EXPECT_EQ(array->to_string(), "ArrayLiteral([3 elements])");
}

// ObjectLiteral Tests
TEST_F(LiteralsTest, ObjectLiteralEmpty) {
  std::vector<std::unique_ptr<ObjectProperty>> properties;
  auto object = std::make_unique<ObjectLiteral>(std::move(properties));

  EXPECT_EQ(object->get_property_count(), 0);
  EXPECT_TRUE(object->is_empty());
  EXPECT_EQ(object->get_node_type(), NodeType::OBJECT_LITERAL);
  EXPECT_EQ(object->to_string(), "ObjectLiteral({0 properties})");
}

TEST_F(LiteralsTest, ObjectLiteralWithProperties) {
  std::vector<std::unique_ptr<ObjectProperty>> properties;

  auto key = std::make_unique<StringLiteral>("name",
                                             StringLiteral::StringType::SIMPLE);
  auto value = std::make_unique<StringLiteral>(
      "Alice", StringLiteral::StringType::SIMPLE);

  auto property = std::make_unique<ObjectProperty>(
      ObjectProperty::PropertyType::NORMAL, std::move(key), std::move(value));

  properties.push_back(std::move(property));

  auto object = std::make_unique<ObjectLiteral>(std::move(properties));

  EXPECT_EQ(object->get_property_count(), 1);
  EXPECT_FALSE(object->is_empty());
  EXPECT_EQ(object->to_string(), "ObjectLiteral({1 properties})");
}

// ObjectProperty Tests
TEST_F(LiteralsTest, ObjectPropertyNormal) {
  auto key = std::make_unique<StringLiteral>("name",
                                             StringLiteral::StringType::SIMPLE);
  auto value = std::make_unique<StringLiteral>(
      "Alice", StringLiteral::StringType::SIMPLE);

  auto property = std::make_unique<ObjectProperty>(
      ObjectProperty::PropertyType::NORMAL, std::move(key), std::move(value));

  EXPECT_EQ(property->get_property_type(),
            ObjectProperty::PropertyType::NORMAL);
  EXPECT_NE(property->get_key(), nullptr);
  EXPECT_NE(property->get_value(), nullptr);
}

TEST_F(LiteralsTest, ObjectPropertyShorthand) {
  auto property = std::make_unique<ObjectProperty>(
      ObjectProperty::PropertyType::SHORTHAND, nullptr, nullptr);

  EXPECT_EQ(property->get_property_type(),
            ObjectProperty::PropertyType::SHORTHAND);
}