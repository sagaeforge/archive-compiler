#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Test class for Struct and Interface parsing functionality
 */
class StructInterfaceParsingTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}

  std::vector<Token> create_tokens(
      const std::vector<std::pair<TokenType, std::string>> &token_specs) {
    std::vector<Token> tokens;
    for (const auto &spec : token_specs) {
      tokens.emplace_back(spec.first, nugdev::lib::String(spec.second));
    }
    tokens.emplace_back(TokenType::EOF_TOKEN, nugdev::lib::String(""));
    return tokens;
  }

  std::unique_ptr<Program> safe_parse(const std::vector<Token> &tokens) {
    try {
      Parser parser(tokens);
      return parser.parse();
    } catch (const Parser::ParseException &e) {
      ADD_FAILURE() << "Parse error: " << e.what();
      return nullptr;
    }
  }

  void expect_parse_failure(const std::vector<Token> &tokens) {
    Parser parser(tokens);
    EXPECT_THROW(parser.parse(), Parser::ParseException);
  }
};

// ==================== Struct Declarations ====================

TEST_F(StructInterfaceParsingTest, SimpleStructDeclaration) {
  // Test struct Point { x: Int, y: Int }
  auto simple_struct_tokens = create_tokens({{TokenType::Struct, "struct"},
                                             {TokenType::Identifier, "Point"},
                                             {TokenType::LeftBrace, "{"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "Int"},
                                             {TokenType::Comma, ","},
                                             {TokenType::Identifier, "y"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "Int"},
                                             {TokenType::RightBrace, "}"}});

  auto result = safe_parse(simple_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, StructWithOptionalSemicolon) {
  // Test struct Point { x: Int, y: Int };
  auto struct_with_semicolon_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Point"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "y"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightBrace, "}"},
                     {TokenType::Semicolon, ";"}});

  auto result = safe_parse(struct_with_semicolon_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, EmptyStruct) {
  // Test struct Empty {}
  auto empty_struct_tokens = create_tokens({{TokenType::Struct, "struct"},
                                            {TokenType::Identifier, "Empty"},
                                            {TokenType::LeftBrace, "{"},
                                            {TokenType::RightBrace, "}"}});

  auto result = safe_parse(empty_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, StructWithSingleField) {
  // Test struct Single { value: String }
  auto single_field_struct_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Single"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "value"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(single_field_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, StructWithOptionalTypes) {
  // Test struct User { name: String, age: Int? }
  auto optional_field_struct_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "User"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "name"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "age"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Question, "?"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(optional_field_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, StructWithFunctionTypes) {
  // Test struct Calculator { add: (Int, Int) -> Int, subtract: (Int, Int) ->
  // Int }
  auto function_field_struct_tokens = create_tokens(
      {{TokenType::Struct, "struct"},  {TokenType::Identifier, "Calculator"},
       {TokenType::LeftBrace, "{"},    {TokenType::Identifier, "add"},
       {TokenType::Colon, ":"},        {TokenType::LeftParen, "("},
       {TokenType::Identifier, "Int"}, {TokenType::Comma, ","},
       {TokenType::Identifier, "Int"}, {TokenType::RightParen, ")"},
       {TokenType::Arrow, "->"},       {TokenType::Identifier, "Int"},
       {TokenType::Comma, ","},        {TokenType::Identifier, "subtract"},
       {TokenType::Colon, ":"},        {TokenType::LeftParen, "("},
       {TokenType::Identifier, "Int"}, {TokenType::Comma, ","},
       {TokenType::Identifier, "Int"}, {TokenType::RightParen, ")"},
       {TokenType::Arrow, "->"},       {TokenType::Identifier, "Int"},
       {TokenType::RightBrace, "}"}});

  auto result = safe_parse(function_field_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, StructWithDefaultValues) {
  // Test struct Config { port: Int = 8080, host: String = "localhost" }
  auto default_values_struct_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Config"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "port"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Assign, "="},
                     {TokenType::Number, "8080"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "host"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Assign, "="},
                     {TokenType::String, "\"localhost\""},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(default_values_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, StructWithMixedFieldTypes) {
  // Test struct Complex { id: Int, name: String?, callback: () -> Void,
  // enabled: Bool = true }
  auto complex_struct_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Complex"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "id"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "name"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Question, "?"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "callback"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "enabled"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::Assign, "="},
                     {TokenType::True, "true"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(complex_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Interface Declarations ====================

TEST_F(StructInterfaceParsingTest, SimpleInterfaceDeclaration) {
  // Test interface Drawable { draw: () -> Void }
  auto simple_interface_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Drawable"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "draw"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(simple_interface_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, InterfaceWithOptionalSemicolon) {
  // Test interface Drawable { draw: () -> Void };
  auto interface_with_semicolon_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Drawable"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "draw"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::RightBrace, "}"},
                     {TokenType::Semicolon, ";"}});

  auto result = safe_parse(interface_with_semicolon_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, EmptyInterface) {
  // Test interface Empty {}
  auto empty_interface_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Empty"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(empty_interface_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, InterfaceWithMultipleMethods) {
  // Test interface Repository { save: (T) -> Void, findById: (Int) -> T?,
  // delete: (Int) -> Bool }
  auto multi_method_interface_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Repository"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "save"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "T"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "findById"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "T"},
                     {TokenType::Question, "?"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "delete"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(multi_method_interface_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, InterfaceWithPropertyMembers) {
  // Test interface Identifiable { id: Int, name: String }
  auto property_interface_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Identifiable"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "id"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "name"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(property_interface_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, InterfaceWithMixedMembers) {
  // Test interface Service { name: String, process: (String) -> Bool, status:
  // () -> String }
  auto mixed_interface_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Service"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "name"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "process"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "status"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(mixed_interface_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Multiple Declarations ====================

TEST_F(StructInterfaceParsingTest, MultipleStructDeclarations) {
  // Test multiple struct declarations
  auto multiple_structs_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Point"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightBrace, "}"},
                     {TokenType::Semicolon, ";"},
                     {TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Rectangle"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "width"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "height"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(multiple_structs_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, MultipleInterfaceDeclarations) {
  // Test multiple interface declarations
  auto multiple_interfaces_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Readable"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "read"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightBrace, "}"},
                     {TokenType::Semicolon, ";"},
                     {TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Writable"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "write"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(multiple_interfaces_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(StructInterfaceParsingTest, MixedStructAndInterface) {
  // Test struct and interface declarations together
  auto mixed_declarations_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "User"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "name"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightBrace, "}"},
                     {TokenType::Semicolon, ";"},
                     {TokenType::Interface, "interface"},
                     {TokenType::Identifier, "UserService"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "createUser"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "User"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(mixed_declarations_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Error Cases ====================

TEST_F(StructInterfaceParsingTest, StructErrorCases) {
  // Missing struct name
  auto missing_name_tokens = create_tokens({{TokenType::Struct, "struct"},
                                            {TokenType::LeftBrace, "{"},
                                            {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_name_tokens);

  // Missing opening brace
  auto missing_brace_tokens = create_tokens({{TokenType::Struct, "struct"},
                                             {TokenType::Identifier, "Point"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "Int"}});
  expect_parse_failure(missing_brace_tokens);

  // Missing field type
  auto missing_field_type_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "Point"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_field_type_tokens);

  // Missing colon in field
  auto missing_colon_tokens = create_tokens({{TokenType::Struct, "struct"},
                                             {TokenType::Identifier, "Point"},
                                             {TokenType::LeftBrace, "{"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Identifier, "Int"},
                                             {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_colon_tokens);
}

TEST_F(StructInterfaceParsingTest, InterfaceErrorCases) {
  // Missing interface name
  auto missing_interface_name_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_interface_name_tokens);

  // Missing opening brace
  auto missing_interface_brace_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Drawable"},
                     {TokenType::Identifier, "draw"},
                     {TokenType::Colon, ":"}});
  expect_parse_failure(missing_interface_brace_tokens);

  // Missing member type
  auto missing_member_type_tokens =
      create_tokens({{TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Service"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "process"},
                     {TokenType::Colon, ":"},
                     {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_member_type_tokens);
}

// ==================== Complex Nested Types ====================

TEST_F(StructInterfaceParsingTest, StructWithNestedFunctionTypes) {
  // Test struct with complex nested function types
  auto nested_function_struct_tokens =
      create_tokens({{TokenType::Struct, "struct"},
                     {TokenType::Identifier, "AsyncProcessor"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "process"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(nested_function_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

} // namespace nugdev::test