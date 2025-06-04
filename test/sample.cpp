#include <gtest/gtest.h>
#include <iostream>

#include "03_tokenize/TokenConverter.h"
#include "03_tokenize/Tokenizer.h"
#include <04_parsing/ast/core/AST.hpp>

TEST(sample, sample) {
  nugdev::compiler::tokenize::Tokenizer tokenizer;
  nugdev::compiler::tokenize::TokenConverter converter;

  auto results =
      tokenizer.tokenize("(x, y) => x + y; (a: number) -> number => a * 2");
  for (auto token : results) {
    std::cout << token << std::endl;
  }

  EXPECT_TRUE(true);
}

TEST(ast, basic_usage) {
  using namespace nugdev::ast;

  std::cout << "=== nugdev AST System Example ===\n\n";

  // Create some literal nodes
  auto number = ASTFactory::create_number_literal(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto str = ASTFactory::create_string_literal("Hello, nugdev!");
  auto boolean = ASTFactory::create_boolean_literal(true);
  auto identifier = ASTFactory::create_identifier("x");

  std::cout << "1. Created basic nodes:\n";
  std::cout << "   - " << number->to_string() << "\n";
  std::cout << "   - " << str->to_string() << "\n";
  std::cout << "   - " << boolean->to_string() << "\n";
  std::cout << "   - " << identifier->to_string() << "\n\n";

  // Create a binary expression: x + 42
  auto binaryExpr = ASTFactory::create_binary_expression(
      BinaryExpression::Operator::ADD, ASTFactory::create_identifier("x"),
      ASTFactory::create_number_literal(
          "42", NumberLiteral::NumberType::DECIMAL_INTEGER));

  std::cout << "2. Created binary expression: " << binaryExpr->to_string()
            << "\n\n";

  // Create some type literals
  auto simpleType = TypeFactory::create_simple_type("number");
  auto optionalType = TypeFactory::create_optional_type(
      TypeFactory::create_simple_type("string"));

  std::cout << "3. Created type literals:\n";
  std::cout << "   - " << simpleType->to_string()
            << " (type: " << simpleType->get_type_name() << ")\n";
  std::cout << "   - " << optionalType->to_string()
            << " (type: " << optionalType->get_type_name() << ")\n\n";

  // Demonstrate type checking utilities
  std::cout << "4. Type checking utilities:\n";
  std::cout << "   - Is binaryExpr a BinaryExpression? "
            << (ASTUtils::is_node_type<BinaryExpression>(*binaryExpr) ? "Yes"
                                                                      : "No")
            << "\n";
  std::cout << "   - Is number a StringLiteral? "
            << (ASTUtils::is_node_type<StringLiteral>(*number) ? "Yes" : "No")
            << "\n";

  if (auto *binExpr = ASTUtils::as_node_type<BinaryExpression>(*binaryExpr)) {
    std::cout << "   - Binary expression operator: "
              << binExpr->get_operator_string() << "\n";
    std::cout << "   - Is arithmetic operator? "
              << (binExpr->is_arithmetic_operator() ? "Yes" : "No") << "\n";
  }

  std::cout << "\n=== AST example completed successfully! ===\n";

  EXPECT_TRUE(true);
}