#pragma once

#include "01_lib/String.h"
#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/ast/core/AST.hpp"
#include "04_parsing/ast/program/ImportExport.hpp"
#include <memory>
#include <stdexcept>
#include <vector>

namespace nugdev::compiler::parsing {

/**
 * @brief Recursive descent parser for nugdev language
 *
 * Implements the complete EBNF grammar defined in the grammar folder.
 * Follows the Program → Module → Statement → Expression structure.
 */
class Parser {
public:
  /**
   * @brief Exception thrown when parsing fails
   */
  class ParseException : public std::runtime_error {
  public:
    explicit ParseException(const std::string &message)
        : std::runtime_error(message) {}
  };

  /**
   * @brief Construct a new Parser object
   *
   * @param tokens Vector of tokens from the tokenizer
   */
  explicit Parser(std::vector<tokenize::Token> tokens);

  /**
   * @brief Parse the tokens into an AST
   *
   * @return std::unique_ptr<ast::Program> Root program node
   */
  std::unique_ptr<ast::Program> parse();

private:
  // Token management
  std::vector<tokenize::Token> m_tokens;
  size_t m_current_position;

  // Helper methods for token management
  bool is_at_end() const;
  const tokenize::Token &current() const;
  const tokenize::Token &peek(size_t offset = 1) const;
  const tokenize::Token &advance();
  const tokenize::Token &previous() const;
  bool check(tokenize::TokenType type) const;
  bool match(tokenize::TokenType type);
  bool match(const std::vector<tokenize::TokenType> &types);
  const tokenize::Token &consume(tokenize::TokenType type,
                                 const std::string &message);

  // Error handling
  void error(const std::string &message) const;
  void synchronize();

  // Grammar parsing methods - following EBNF structure

  // Program structure
  std::unique_ptr<ast::Program> parse_program();
  std::unique_ptr<ast::Module> parse_module();

  // Statements (no return value)
  std::unique_ptr<ast::Statement> parse_statement();
  std::unique_ptr<ast::VariableDeclaration> parse_variable_declaration();
  std::unique_ptr<ast::VariableDeclaration>
  parse_variable_declaration(bool consume_semicolon);
  std::unique_ptr<ast::FunctionDeclaration> parse_function_declaration();
  std::unique_ptr<ast::StructDeclaration> parse_struct_declaration();
  std::unique_ptr<ast::InterfaceDeclaration> parse_interface_declaration();
  std::unique_ptr<ast::ExpressionStatement> parse_expression_statement();

  // Control flow statements
  std::unique_ptr<ast::IfStatement> parse_if_statement();
  std::unique_ptr<ast::ForStatement> parse_for_statement();
  std::unique_ptr<ast::BreakStatement> parse_break_statement();
  std::unique_ptr<ast::ContinueStatement> parse_continue_statement();
  std::unique_ptr<ast::ReturnStatement> parse_return_statement();

  // Import/Export statements
  std::unique_ptr<ast::ImportStatement> parse_import_statement();
  std::unique_ptr<ast::ExportStatement> parse_export_statement();

  // Expressions (have return value) - following operator precedence
  std::unique_ptr<ast::Expression> parse_expression();
  std::unique_ptr<ast::Expression> parse_assignment_expression();
  std::unique_ptr<ast::Expression> parse_ternary_expression();
  std::unique_ptr<ast::Expression> parse_null_coalescing_expression();
  std::unique_ptr<ast::Expression> parse_logical_or_expression();
  std::unique_ptr<ast::Expression> parse_logical_and_expression();
  std::unique_ptr<ast::Expression> parse_bitwise_or_expression();
  std::unique_ptr<ast::Expression> parse_bitwise_xor_expression();
  std::unique_ptr<ast::Expression> parse_bitwise_and_expression();
  std::unique_ptr<ast::Expression> parse_equality_expression();
  std::unique_ptr<ast::Expression> parse_relational_expression();
  std::unique_ptr<ast::Expression> parse_type_check_expression();
  std::unique_ptr<ast::Expression> parse_shift_expression();
  std::unique_ptr<ast::Expression> parse_additive_expression();
  std::unique_ptr<ast::Expression> parse_range_expression();
  std::unique_ptr<ast::Expression> parse_multiplicative_expression();
  std::unique_ptr<ast::Expression> parse_unary_expression();
  std::unique_ptr<ast::Expression> parse_postfix_expression();
  std::unique_ptr<ast::Expression> parse_primary_expression();

  // Complex expressions
  std::unique_ptr<ast::BlockExpression> parse_block_expression();
  std::unique_ptr<ast::IfExpression> parse_if_expression();
  std::unique_ptr<ast::WhenExpression> parse_when_expression();
  std::unique_ptr<ast::FunctionExpression> parse_function_expression();
  std::unique_ptr<ast::LambdaExpression> parse_lambda_expression();

  // When conditions
  std::unique_ptr<ast::WhenCondition> parse_when_condition();
  std::unique_ptr<ast::ValueCondition> parse_value_condition();
  std::unique_ptr<ast::RangeCondition> parse_range_condition();
  std::unique_ptr<ast::TypeCondition> parse_type_condition();
  std::unique_ptr<ast::GuardCondition> parse_guard_condition();
  std::unique_ptr<ast::MultipleCondition> parse_multiple_condition();

  // Literals
  std::unique_ptr<ast::Literal> parse_literal();
  std::unique_ptr<ast::NumberLiteral> parse_number_literal();
  std::unique_ptr<ast::StringLiteral> parse_string_literal();
  std::unique_ptr<ast::CharacterLiteral> parse_character_literal();
  std::unique_ptr<ast::BooleanLiteral> parse_boolean_literal();
  std::unique_ptr<ast::ArrayLiteral> parse_array_literal();
  std::unique_ptr<ast::ObjectLiteral> parse_object_literal();

  // Types
  std::unique_ptr<ast::TypeLiteral> parse_type_literal();
  std::unique_ptr<ast::FunctionType> parse_function_type();
  std::unique_ptr<ast::OptionalType> parse_optional_type();
  std::unique_ptr<ast::TupleType> parse_tuple_type();

  // Parameters and arguments
  std::vector<std::unique_ptr<ast::Parameter>> parse_parameter_list();
  std::unique_ptr<ast::Parameter> parse_parameter();
  std::vector<std::unique_ptr<ast::Expression>> parse_argument_list();

  // Struct and interface members
  std::vector<std::unique_ptr<ast::StructField>> parse_struct_field_list();
  std::unique_ptr<ast::StructField> parse_struct_field();

  // Object properties
  std::vector<std::unique_ptr<ast::ObjectProperty>>
  parse_object_property_list();
  std::unique_ptr<ast::ObjectProperty> parse_object_property();

  // Utility methods
  std::string parse_identifier();
  std::string parse_label();
  bool is_assignment_operator(tokenize::TokenType type) const;
  bool is_unary_operator(tokenize::TokenType type) const;

  // Operator conversion methods
  ast::BinaryExpression::Operator
  token_to_binary_operator(tokenize::TokenType type) const;
  ast::UnaryExpression::Operator
  token_to_unary_operator(tokenize::TokenType type) const;
  ast::AssignmentExpression::Operator
  token_to_assignment_operator(tokenize::TokenType type) const;
};

} // namespace nugdev::compiler::parsing