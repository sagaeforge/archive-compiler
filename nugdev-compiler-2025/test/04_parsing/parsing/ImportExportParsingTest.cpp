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
 * @brief Test class for Import/Export parsing functionality
 */
class ImportExportParsingTest : public ::testing::Test {
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

// ==================== Import Statements ====================

TEST_F(ImportExportParsingTest, SimpleImportStatement) {
  // Test import "module.nug"
  auto simple_import_tokens = create_tokens(
      {{TokenType::Import, "import"}, {TokenType::String, "\"module.nug\""}});

  auto result = safe_parse(simple_import_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, ImportWithAlias) {
  // Test import "module.nug" as ModuleName
  auto import_with_alias_tokens =
      create_tokens({{TokenType::Import, "import"},
                     {TokenType::String, "\"module.nug\""},
                     {TokenType::As, "as"},
                     {TokenType::Identifier, "ModuleName"}});

  auto result = safe_parse(import_with_alias_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, ImportWithSemicolon) {
  // Test import "module.nug";
  auto import_with_semicolon_tokens =
      create_tokens({{TokenType::Import, "import"},
                     {TokenType::String, "\"module.nug\""},
                     {TokenType::Semicolon, ";"}});

  auto result = safe_parse(import_with_semicolon_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, ImportWithAliasAndSemicolon) {
  // Test import "module.nug" as ModuleName;
  auto import_complete_tokens =
      create_tokens({{TokenType::Import, "import"},
                     {TokenType::String, "\"module.nug\""},
                     {TokenType::As, "as"},
                     {TokenType::Identifier, "ModuleName"},
                     {TokenType::Semicolon, ";"}});

  auto result = safe_parse(import_complete_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, MultipleImports) {
  // Test multiple import statements
  auto multiple_imports_tokens =
      create_tokens({{TokenType::Import, "import"},
                     {TokenType::String, "\"first.nug\""},
                     {TokenType::Semicolon, ";"},
                     {TokenType::Import, "import"},
                     {TokenType::String, "\"second.nug\""},
                     {TokenType::As, "as"},
                     {TokenType::Identifier, "Second"},
                     {TokenType::Semicolon, ";"},
                     {TokenType::Import, "import"},
                     {TokenType::String, "\"third.nug\""}});

  auto result = safe_parse(multiple_imports_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Export Statements ====================

TEST_F(ImportExportParsingTest, ExportVariable) {
  // Test export let x = 42
  auto export_var_tokens = create_tokens({{TokenType::Export, "export"},
                                          {TokenType::Let, "let"},
                                          {TokenType::Identifier, "x"},
                                          {TokenType::Assign, "="},
                                          {TokenType::Number, "42"}});

  auto result = safe_parse(export_var_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, ExportFunction) {
  // Test export fun add(let a: Int, let b: Int): Int = a + b
  auto export_func_tokens = create_tokens(
      {{TokenType::Export, "export"},  {TokenType::Function, "fun"},
       {TokenType::Identifier, "add"}, {TokenType::LeftParen, "("},
       {TokenType::Let, "let"},        {TokenType::Identifier, "a"},
       {TokenType::Colon, ":"},        {TokenType::Identifier, "Int"},
       {TokenType::Comma, ","},        {TokenType::Let, "let"},
       {TokenType::Identifier, "b"},   {TokenType::Colon, ":"},
       {TokenType::Identifier, "Int"}, {TokenType::RightParen, ")"},
       {TokenType::Colon, ":"},        {TokenType::Identifier, "Int"},
       {TokenType::Assign, "="},       {TokenType::Identifier, "a"},
       {TokenType::Plus, "+"},         {TokenType::Identifier, "b"}});

  auto result = safe_parse(export_func_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, ExportStruct) {
  // Test export struct Point { x: Int, y: Int }
  auto export_struct_tokens = create_tokens({{TokenType::Export, "export"},
                                             {TokenType::Struct, "struct"},
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

  auto result = safe_parse(export_struct_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ImportExportParsingTest, ExportInterface) {
  // Test export interface Drawable { draw: () -> Void }
  auto export_interface_tokens =
      create_tokens({{TokenType::Export, "export"},
                     {TokenType::Interface, "interface"},
                     {TokenType::Identifier, "Drawable"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Identifier, "draw"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(export_interface_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Import/Export Combined ====================

TEST_F(ImportExportParsingTest, ImportThenExport) {
  // Test import followed by export
  auto import_export_tokens =
      create_tokens({{TokenType::Import, "import"},
                     {TokenType::String, "\"utils.nug\""},
                     {TokenType::As, "as"},
                     {TokenType::Identifier, "Utils"},
                     {TokenType::Semicolon, ";"},
                     {TokenType::Export, "export"},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "result"},
                     {TokenType::Assign, "="},
                     {TokenType::Number, "42"}});

  auto result = safe_parse(import_export_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Error Cases ====================

TEST_F(ImportExportParsingTest, ImportErrorCases) {
  // Test import without string literal
  auto missing_string_tokens = create_tokens({
      {TokenType::Import, "import"}, {TokenType::Identifier, "module"}
      // Should be string literal
  });
  expect_parse_failure(missing_string_tokens);

  // Test import with invalid alias
  auto invalid_alias_tokens = create_tokens({
      {TokenType::Import, "import"},
      {TokenType::String, "\"module.nug\""},
      {TokenType::As, "as"},
      {TokenType::Number, "123"} // Should be identifier
  });
  expect_parse_failure(invalid_alias_tokens);

  // Test import with missing alias after 'as'
  auto missing_alias_tokens = create_tokens({
      {TokenType::Import, "import"},
      {TokenType::String, "\"module.nug\""},
      {TokenType::As, "as"} // Missing identifier
  });
  expect_parse_failure(missing_alias_tokens);
}

TEST_F(ImportExportParsingTest, ExportErrorCases) {
  // Test export without statement
  auto export_only_tokens = create_tokens({
      {TokenType::Export, "export"} // Missing statement to export
  });
  expect_parse_failure(export_only_tokens);

  // Test export with invalid statement
  auto export_invalid_tokens = create_tokens({
      {TokenType::Export, "export"}, {TokenType::Number, "42"}
      // Can't export a plain expression
  });
  expect_parse_failure(export_invalid_tokens);
}

// ==================== Different Import Path Formats ====================

TEST_F(ImportExportParsingTest, DifferentImportPaths) {
  std::vector<std::string> valid_paths = {
      "\"module.nug\"",
      "\"./module.nug\"",
      "\"../module.nug\"",
      "\"path/to/module.nug\"",
      "\"@stdlib/collections.nug\"",
      "\"very/long/path/to/deeply/nested/module.nug\""};

  for (const auto &path : valid_paths) {
    auto tokens = create_tokens(
        {{TokenType::Import, "import"}, {TokenType::String, path}});

    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse import path: " << path;
  }
}

// ==================== Complex Module Structure ====================

TEST_F(ImportExportParsingTest, ComplexModuleStructure) {
  // Test complex module with imports, exports, and regular statements
  auto complex_module_tokens =
      create_tokens({// Imports
                     {TokenType::Import, "import"},
                     {TokenType::String, "\"std/collections.nug\""},
                     {TokenType::As, "as"},
                     {TokenType::Identifier, "Collections"},
                     {TokenType::Semicolon, ";"},

                     {TokenType::Import, "import"},
                     {TokenType::String, "\"./utils.nug\""},
                     {TokenType::Semicolon, ";"},

                     // Regular code
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "internal"},
                     {TokenType::Assign, "="},
                     {TokenType::Number, "42"},
                     {TokenType::Semicolon, ";"},

                     // Exports
                     {TokenType::Export, "export"},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "publicVar"},
                     {TokenType::Assign, "="},
                     {TokenType::String, "\"hello\""},
                     {TokenType::Semicolon, ";"},

                     {TokenType::Export, "export"},
                     {TokenType::Function, "fun"},
                     {TokenType::Identifier, "publicFunc"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::Assign, "="},
                     {TokenType::Identifier, "internal"}});

  auto result = safe_parse(complex_module_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

} // namespace nugdev::test