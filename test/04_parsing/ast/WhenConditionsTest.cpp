#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class WhenConditionsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// ValueCondition Tests
TEST_F(WhenConditionsTest, ValueConditionBasic) {
  auto value = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto condition = std::make_unique<ValueCondition>(std::move(value));

  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->get_node_type(), NodeType::VALUE_CONDITION);
  EXPECT_EQ(condition->to_string(), "ValueCondition(NumberLiteral(42))");
}

TEST_F(WhenConditionsTest, ValueConditionMatches) {
  auto value1 = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto value2 = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto condition = std::make_unique<ValueCondition>(std::move(value1));

  EXPECT_TRUE(condition->matches(*value2));
}

TEST_F(WhenConditionsTest, ValueConditionNoMatch) {
  auto value1 = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto value2 = std::make_unique<NumberLiteral>(
      "24", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto condition = std::make_unique<ValueCondition>(std::move(value1));

  EXPECT_FALSE(condition->matches(*value2));
}

// RangeCondition Tests
TEST_F(WhenConditionsTest, RangeConditionBasic) {
  auto value = std::make_unique<Identifier>("x");
  auto range = std::make_unique<Identifier>("myRange");
  auto condition =
      std::make_unique<RangeCondition>(std::move(value), std::move(range));

  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->get_node_type(), NodeType::RANGE_CONDITION);
  EXPECT_EQ(condition->to_string(),
            "RangeCondition(Identifier(x) in Identifier(myRange))");
}

TEST_F(WhenConditionsTest, RangeConditionProperties) {
  auto value = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto range = std::make_unique<Identifier>("range1to10");
  auto condition =
      std::make_unique<RangeCondition>(std::move(value), std::move(range));

  EXPECT_EQ(condition->get_value().get_node_type(), NodeType::NUMBER_LITERAL);
  EXPECT_EQ(condition->get_range().get_node_type(), NodeType::IDENTIFIER);
}

// TypeCondition Tests
TEST_F(WhenConditionsTest, TypeConditionBasic) {
  auto value = std::make_unique<Identifier>("obj");
  auto type = std::make_unique<SimpleType>("String");
  auto condition =
      std::make_unique<TypeCondition>(std::move(value), std::move(type));

  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->get_node_type(), NodeType::TYPE_CONDITION);
  EXPECT_EQ(condition->to_string(),
            "TypeCondition(Identifier(obj) is SimpleType(String))");
}

TEST_F(WhenConditionsTest, TypeConditionProperties) {
  auto value = std::make_unique<Identifier>("myVar");
  auto type = std::make_unique<SimpleType>("Integer");
  auto condition =
      std::make_unique<TypeCondition>(std::move(value), std::move(type));

  EXPECT_EQ(condition->get_value().get_node_type(), NodeType::IDENTIFIER);
  EXPECT_EQ(condition->get_type().get_node_type(), NodeType::TYPE_LITERAL);
}

// GuardCondition Tests
TEST_F(WhenConditionsTest, GuardConditionBasic) {
  auto innerValue = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto innerCondition = std::make_unique<ValueCondition>(std::move(innerValue));
  auto guard = std::make_unique<BooleanLiteral>(true);
  auto guardCondition = std::make_unique<GuardCondition>(
      std::move(innerCondition), std::move(guard));

  ASSERT_NE(guardCondition, nullptr);
  EXPECT_EQ(guardCondition->get_node_type(), NodeType::GUARD_CONDITION);
}

TEST_F(WhenConditionsTest, GuardConditionProperties) {
  auto innerValue = std::make_unique<Identifier>("x");
  auto innerCondition = std::make_unique<ValueCondition>(std::move(innerValue));
  auto guard = std::make_unique<Identifier>("isValid");
  auto guardCondition = std::make_unique<GuardCondition>(
      std::move(innerCondition), std::move(guard));

  EXPECT_EQ(guardCondition->get_inner_condition().get_node_type(),
            NodeType::VALUE_CONDITION);
  EXPECT_EQ(guardCondition->get_guard().get_node_type(), NodeType::IDENTIFIER);
}

// MultipleCondition Tests
TEST_F(WhenConditionsTest, MultipleConditionEmpty) {
  auto multiCondition = std::make_unique<MultipleCondition>();

  ASSERT_NE(multiCondition, nullptr);
  EXPECT_EQ(multiCondition->get_node_type(), NodeType::MULTIPLE_CONDITION);
  EXPECT_EQ(multiCondition->get_conditions().size(), 0);
  EXPECT_EQ(multiCondition->to_string(), "MultipleCondition(0 conditions)");
}

TEST_F(WhenConditionsTest, MultipleConditionWithConditions) {
  auto multiCondition = std::make_unique<MultipleCondition>();

  // Add first condition
  auto value1 = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto condition1 = std::make_unique<ValueCondition>(std::move(value1));
  multiCondition->add_condition(std::move(condition1));

  // Add second condition
  auto value2 = std::make_unique<NumberLiteral>(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto condition2 = std::make_unique<ValueCondition>(std::move(value2));
  multiCondition->add_condition(std::move(condition2));

  EXPECT_EQ(multiCondition->get_conditions().size(), 2);
  EXPECT_EQ(multiCondition->to_string(), "MultipleCondition(2 conditions)");
}

TEST_F(WhenConditionsTest, MultipleConditionMatches) {
  auto multiCondition = std::make_unique<MultipleCondition>();

  // Add conditions for values 1, 2, 3
  for (int i = 1; i <= 3; ++i) {
    auto value = std::make_unique<NumberLiteral>(
        std::to_string(i), NumberLiteral::NumberType::DECIMAL_INTEGER);
    auto condition = std::make_unique<ValueCondition>(std::move(value));
    multiCondition->add_condition(std::move(condition));
  }

  // Test value that should match (2)
  auto testValue2 = std::make_unique<NumberLiteral>(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER);
  EXPECT_TRUE(multiCondition->matches(*testValue2));

  // Test value that should not match (5)
  auto testValue5 = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  EXPECT_FALSE(multiCondition->matches(*testValue5));
}

// Visitor Pattern Tests
TEST_F(WhenConditionsTest, ConditionsAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int valueConditionCount = 0;
    int rangeConditionCount = 0;
    int typeConditionCount = 0;
    int guardConditionCount = 0;
    int multipleConditionCount = 0;

    void visit(ValueCondition &) override { valueConditionCount++; }
    void visit(RangeCondition &) override { rangeConditionCount++; }
    void visit(TypeCondition &) override { typeConditionCount++; }
    void visit(GuardCondition &) override { guardConditionCount++; }
    void visit(MultipleCondition &) override { multipleConditionCount++; }
  };

  TestVisitor visitor;

  // Test ValueCondition
  auto value = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto valueCondition = std::make_unique<ValueCondition>(std::move(value));
  valueCondition->accept(visitor);

  // Test RangeCondition
  auto rangeValue = std::make_unique<Identifier>("x");
  auto range = std::make_unique<Identifier>("range");
  auto rangeCondition =
      std::make_unique<RangeCondition>(std::move(rangeValue), std::move(range));
  rangeCondition->accept(visitor);

  // Test MultipleCondition
  auto multiCondition = std::make_unique<MultipleCondition>();
  multiCondition->accept(visitor);

  EXPECT_EQ(visitor.valueConditionCount, 1);
  EXPECT_EQ(visitor.rangeConditionCount, 1);
  EXPECT_EQ(visitor.multipleConditionCount, 1);
}