#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class FactoryTest : public ::testing::Test {
protected:
};

// Test literal creation
TEST_F(FactoryTest, CreateNumberLiteral) {
  auto num = ASTFactory::create_number_literal(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);

  ASSERT_NE(num, nullptr);
  EXPECT_EQ(num->get_node_type(), NodeType::NUMBER_LITERAL);
  EXPECT_EQ(num->get_literal_value(), "42");
}

TEST_F(FactoryTest, CreateStringLiteral) {
  auto str = ASTFactory::create_string_literal("hello");

  ASSERT_NE(str, nullptr);
  EXPECT_EQ(str->get_node_type(), NodeType::STRING_LITERAL);
  EXPECT_EQ(str->get_literal_value(), "hello");
}

TEST_F(FactoryTest, CreateBooleanLiteral) {
  auto boolLiteral = ASTFactory::create_boolean_literal(true);

  ASSERT_NE(boolLiteral, nullptr);
  EXPECT_EQ(boolLiteral->get_node_type(), NodeType::BOOLEAN_LITERAL);
  EXPECT_EQ(boolLiteral->get_boolean_value(), true);
}

// Expression tests
TEST_F(FactoryTest, CreateIdentifier) {
  auto id = ASTFactory::create_identifier("myVar");

  ASSERT_NE(id, nullptr);
  EXPECT_EQ(id->get_node_type(), NodeType::IDENTIFIER);
  EXPECT_EQ(id->get_name(), "myVar");
}

TEST_F(FactoryTest, CreateBinaryExpression) {
  auto left = ASTFactory::create_number_literal(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto right = ASTFactory::create_number_literal(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER);

  auto binExpr = ASTFactory::create_binary_expression(
      BinaryExpression::Operator::ADD, std::move(left), std::move(right));

  ASSERT_NE(binExpr, nullptr);
  EXPECT_EQ(binExpr->get_node_type(), NodeType::BINARY_EXPRESSION);
  EXPECT_EQ(binExpr->get_operator(), BinaryExpression::Operator::ADD);
}

TEST_F(FactoryTest, CreateUnaryExpression) {
  auto operand = ASTFactory::create_number_literal(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);

  auto unaryExpr = ASTFactory::create_unary_expression(
      UnaryExpression::Operator::MINUS, std::move(operand));

  ASSERT_NE(unaryExpr, nullptr);
  EXPECT_EQ(unaryExpr->get_node_type(), NodeType::UNARY_EXPRESSION);
  EXPECT_EQ(unaryExpr->get_operator(), UnaryExpression::Operator::MINUS);
}

// Collection tests
TEST_F(FactoryTest, CreateArrayLiteral) {
  std::vector<std::unique_ptr<Expression>> elements;
  elements.push_back(ASTFactory::create_number_literal(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER));
  elements.push_back(ASTFactory::create_number_literal(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER));

  auto arrayLiteral = ASTFactory::create_array_literal(std::move(elements));

  ASSERT_NE(arrayLiteral, nullptr);
  EXPECT_EQ(arrayLiteral->get_node_type(), NodeType::ARRAY_LITERAL);
  EXPECT_EQ(arrayLiteral->get_element_count(), 2);
}

TEST_F(FactoryTest, CreateObjectLiteral) {
  std::vector<std::unique_ptr<ObjectProperty>> properties;

  auto objectLiteral = ASTFactory::create_object_literal(std::move(properties));

  ASSERT_NE(objectLiteral, nullptr);
  EXPECT_EQ(objectLiteral->get_node_type(), NodeType::OBJECT_LITERAL);
  EXPECT_EQ(objectLiteral->get_property_count(), 0);
}

// Type tests
TEST_F(FactoryTest, CreateSimpleType) {
  auto type = ASTFactory::create_simple_type("int");

  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->get_type_name(), "int");
}

TEST_F(FactoryTest, CreateFunctionType) {
  std::vector<std::unique_ptr<TypeLiteral>> paramTypes;
  paramTypes.push_back(ASTFactory::create_simple_type("int"));
  paramTypes.push_back(ASTFactory::create_simple_type("string"));

  auto returnType = ASTFactory::create_simple_type("bool");
  auto funcType = ASTFactory::create_function_type(std::move(paramTypes),
                                                   std::move(returnType));

  ASSERT_NE(funcType, nullptr);
  EXPECT_EQ(funcType->get_parameter_types().size(), 2);
  EXPECT_EQ(funcType->get_return_type().get_type_name(), "bool");
}

TEST_F(FactoryTest, CreateOptionalType) {
  auto innerType = ASTFactory::create_simple_type("string");
  auto optionalType = ASTFactory::create_optional_type(std::move(innerType));

  ASSERT_NE(optionalType, nullptr);
  EXPECT_EQ(optionalType->get_type_name(), "string?");
}

// Parameter tests
TEST_F(FactoryTest, CreateParameter) {
  auto type = ASTFactory::create_simple_type("int");
  auto param = ASTFactory::create_parameter(Parameter::Mutability::LET, "x",
                                            std::move(type));

  ASSERT_NE(param, nullptr);
  EXPECT_EQ(param->get_parameter_name(), "x");
  EXPECT_EQ(param->get_mutability(), Parameter::Mutability::LET);
}

// Statement tests
TEST_F(FactoryTest, CreateVariableDeclaration) {
  auto type = ASTFactory::create_simple_type("int");
  auto init = ASTFactory::create_number_literal(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);

  auto varDecl = ASTFactory::create_variable_declaration(
      VariableDeclaration::Mutability::LET, "x", std::move(type),
      std::move(init));

  ASSERT_NE(varDecl, nullptr);
  EXPECT_EQ(varDecl->get_node_type(), NodeType::VARIABLE_DECLARATION);
  EXPECT_EQ(varDecl->get_variable_name(), "x");
  EXPECT_EQ(varDecl->get_mutability(), VariableDeclaration::Mutability::LET);
}

TEST_F(FactoryTest, CreateFunctionDeclaration) {
  std::vector<std::unique_ptr<Parameter>> parameters;
  auto paramType = ASTFactory::create_simple_type("int");
  parameters.push_back(ASTFactory::create_parameter(Parameter::Mutability::LET,
                                                    "x", std::move(paramType)));

  auto returnType = ASTFactory::create_simple_type("bool");
  auto funcDecl = ASTFactory::create_function_declaration(
      "myFunction", std::move(parameters), std::move(returnType));

  ASSERT_NE(funcDecl, nullptr);
  EXPECT_EQ(funcDecl->get_node_type(), NodeType::FUNCTION_DECLARATION);
  EXPECT_EQ(funcDecl->get_function_name(), "myFunction");
  EXPECT_EQ(funcDecl->get_parameters().size(), 1);
}

// Control flow tests
TEST_F(FactoryTest, CreateIfStatement) {
  auto ifStmt = ASTFactory::create_if_statement();

  ASSERT_NE(ifStmt, nullptr);
  EXPECT_EQ(ifStmt->get_node_type(), NodeType::IF_STATEMENT);
}

TEST_F(FactoryTest, CreateForStatement) {
  auto forStmt =
      ASTFactory::create_for_statement(ForStatement::ForType::C_STYLE);

  ASSERT_NE(forStmt, nullptr);
  EXPECT_EQ(forStmt->get_node_type(), NodeType::FOR_STATEMENT);
  EXPECT_EQ(forStmt->get_for_type(), ForStatement::ForType::C_STYLE);
}

TEST_F(FactoryTest, CreateBreakStatement) {
  auto breakStmt = ASTFactory::create_break_statement();

  ASSERT_NE(breakStmt, nullptr);
  EXPECT_EQ(breakStmt->get_node_type(), NodeType::BREAK_STATEMENT);
}

TEST_F(FactoryTest, CreateContinueStatement) {
  auto continueStmt = ASTFactory::create_continue_statement();

  ASSERT_NE(continueStmt, nullptr);
  EXPECT_EQ(continueStmt->get_node_type(), NodeType::CONTINUE_STATEMENT);
}

TEST_F(FactoryTest, CreateReturnStatement) {
  auto value = ASTFactory::create_number_literal(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto returnStmt = ASTFactory::create_return_statement(std::move(value));

  ASSERT_NE(returnStmt, nullptr);
  EXPECT_EQ(returnStmt->get_node_type(), NodeType::RETURN_STATEMENT);
  EXPECT_TRUE(returnStmt->has_return_value());
}

// Expression statement test
TEST_F(FactoryTest, CreateExpressionStatement) {
  auto expr = ASTFactory::create_identifier("myVar");
  auto exprStmt = ASTFactory::create_expression_statement(std::move(expr));

  ASSERT_NE(exprStmt, nullptr);
  EXPECT_EQ(exprStmt->get_node_type(), NodeType::EXPRESSION_STATEMENT);
}

// Block expression test
TEST_F(FactoryTest, CreateBlockExpression) {
  auto block = ASTFactory::create_block_expression();

  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->get_node_type(), NodeType::BLOCK_EXPRESSION);
  EXPECT_TRUE(block->is_empty());
}

// When condition tests
TEST_F(FactoryTest, CreateValueCondition) {
  auto value = ASTFactory::create_number_literal(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto condition = ASTFactory::create_value_condition(std::move(value));

  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->get_node_type(), NodeType::VALUE_CONDITION);
}

TEST_F(FactoryTest, CreateRangeCondition) {
  auto value = ASTFactory::create_identifier("x");
  auto range = ASTFactory::create_identifier("myRange");
  auto condition =
      ASTFactory::create_range_condition(std::move(value), std::move(range));

  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->get_node_type(), NodeType::RANGE_CONDITION);
}

TEST_F(FactoryTest, CreateTypeCondition) {
  auto value = ASTFactory::create_identifier("x");
  auto type = ASTFactory::create_simple_type("int");
  auto condition =
      ASTFactory::create_type_condition(std::move(value), std::move(type));

  ASSERT_NE(condition, nullptr);
  EXPECT_EQ(condition->get_node_type(), NodeType::TYPE_CONDITION);
}

// Program structure tests
TEST_F(FactoryTest, CreateProgram) {
  auto program = ASTFactory::create_program();

  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->get_node_type(), NodeType::PROGRAM);
  EXPECT_TRUE(program->is_empty());
}

TEST_F(FactoryTest, CreateModule) {
  auto module = ASTFactory::create_module("test_module");

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->get_node_type(), NodeType::MODULE);
  EXPECT_EQ(module->get_module_name(), "test_module");
  EXPECT_TRUE(module->is_empty());
}

// Batch operations test
TEST_F(FactoryTest, BatchOperations) {
  std::vector<std::unique_ptr<Expression>> expressions;

  // Create multiple expressions
  for (int i = 0; i < 10; ++i) {
    expressions.push_back(ASTFactory::create_number_literal(
        std::to_string(i), NumberLiteral::NumberType::DECIMAL_INTEGER));
  }

  auto arrayLiteral = ASTFactory::create_array_literal(std::move(expressions));

  ASSERT_NE(arrayLiteral, nullptr);
  EXPECT_EQ(arrayLiteral->get_element_count(), 10);
}

// Memory safety test
TEST_F(FactoryTest, MemorySafety) {
  // Test that factory methods return valid unique_ptr objects
  auto expr = ASTFactory::create_binary_expression(
      BinaryExpression::Operator::ADD,
      ASTFactory::create_number_literal(
          "1", NumberLiteral::NumberType::DECIMAL_INTEGER),
      ASTFactory::create_number_literal(
          "2", NumberLiteral::NumberType::DECIMAL_INTEGER));

  ASSERT_NE(expr, nullptr);
  EXPECT_EQ(expr->get_node_type(), NodeType::BINARY_EXPRESSION);

  // Test that the expression can be moved safely
  auto moved_expr = std::move(expr);
  ASSERT_NE(moved_expr, nullptr);
  EXPECT_EQ(moved_expr->get_node_type(), NodeType::BINARY_EXPRESSION);
}