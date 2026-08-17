#include "Parser.hpp"
#include "04_parsing/ast/program/ImportExport.hpp"
#include <sstream>

namespace nugdev::compiler::parsing {

// Static EOF token to avoid repeated creation
static const tokenize::Token s_eof_token(tokenize::TokenType::EOF_TOKEN, "");

Parser::Parser(std::vector<tokenize::Token> tokens)
    : m_tokens(std::move(tokens)), m_current_position(0) {}

std::unique_ptr<ast::Program> Parser::parse() { return parse_program(); }

// ==================== Token Management ====================

bool Parser::is_at_end() const {
  return m_current_position >= m_tokens.size() ||
         current().get_type() == tokenize::TokenType::EOF_TOKEN;
}

const tokenize::Token &Parser::current() const {
  if (m_current_position >= m_tokens.size()) {
    return s_eof_token;
  }
  return m_tokens[m_current_position];
}

const tokenize::Token &Parser::peek(size_t offset) const {
  size_t pos = m_current_position + offset;
  if (pos >= m_tokens.size()) {
    return s_eof_token;
  }
  return m_tokens[pos];
}

const tokenize::Token &Parser::advance() {
  if (!is_at_end()) {
    m_current_position++;
  }
  return current();
}

const tokenize::Token &Parser::previous() const {
  if (m_current_position == 0) {
    return s_eof_token;
  }
  return m_tokens[m_current_position - 1];
}

bool Parser::check(tokenize::TokenType type) const {
  return current().get_type() == type;
}

bool Parser::match(tokenize::TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

bool Parser::match(const std::vector<tokenize::TokenType> &types) {
  for (const auto &type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }
  return false;
}

const tokenize::Token &Parser::consume(tokenize::TokenType type,
                                       const std::string &message) {
  if (check(type)) {
    return advance();
  }
  error(message);
  return s_eof_token; // Return consistent EOF token on error
}

void Parser::error(const std::string &message) const {
  std::ostringstream oss;
  oss << "Parse error at token '" << current().get_literal().to_string()
      << "': " << message;
  throw ParseException(oss.str());
}

void Parser::synchronize() {
  advance();

  while (!is_at_end()) {
    if (previous().get_type() == tokenize::TokenType::Semicolon)
      return;

    switch (current().get_type()) {
    case tokenize::TokenType::Function:
    case tokenize::TokenType::Let:
    case tokenize::TokenType::Mut:
    case tokenize::TokenType::If:
    case tokenize::TokenType::For:
    case tokenize::TokenType::Return:
    case tokenize::TokenType::Struct:
    case tokenize::TokenType::Interface:
      return;
    default:
      break;
    }
    advance();
  }
}

// ==================== Program Structure ====================

std::unique_ptr<ast::Program> Parser::parse_program() {
  auto program = ast::ASTFactory::create_program();

  while (!is_at_end()) {
    auto module = parse_module();
    if (module) {
      program->add_module(std::move(module));
    }
  }

  return program;
}

std::unique_ptr<ast::Module> Parser::parse_module() {
  auto module = ast::ASTFactory::create_module();

  while (!is_at_end()) {
    try {
      if (check(tokenize::TokenType::Import)) {
        auto import_stmt = parse_import_statement();
        // Note: ImportStatement handling would be implemented when Module
        // supports it
      } else if (check(tokenize::TokenType::Export)) {
        auto export_stmt = parse_export_statement();
        // Note: ExportStatement handling would be implemented when Module
        // supports it
      } else {
        auto statement = parse_statement();
        if (statement) {
          module->add_statement(std::move(statement));
        }
      }
    } catch (const ParseException &) {
      synchronize();
      throw;
    }
  }

  return module;
}

// ==================== Statements ====================

std::unique_ptr<ast::Statement> Parser::parse_statement() {
  if (check(tokenize::TokenType::Let) || check(tokenize::TokenType::Mut)) {
    return parse_variable_declaration();
  }

  if (check(tokenize::TokenType::Function)) {
    // Look ahead to distinguish function declaration from function expression
    size_t saved_pos = m_current_position;
    advance(); // consume 'fun'

    bool is_declaration = false;
    if (check(tokenize::TokenType::Identifier)) {
      is_declaration = true;
    }

    // Restore position
    m_current_position = saved_pos;

    if (is_declaration) {
      return parse_function_declaration();
    } else {
      // It's a function expression, parse as expression statement
      return parse_expression_statement();
    }
  }

  if (check(tokenize::TokenType::Struct)) {
    return parse_struct_declaration();
  }

  if (check(tokenize::TokenType::Interface)) {
    return parse_interface_declaration();
  }

  if (check(tokenize::TokenType::If)) {
    return parse_if_statement();
  }

  if (check(tokenize::TokenType::For)) {
    return parse_for_statement();
  }

  if (check(tokenize::TokenType::Break)) {
    return parse_break_statement();
  }

  if (check(tokenize::TokenType::Continue)) {
    return parse_continue_statement();
  }

  if (check(tokenize::TokenType::Return)) {
    return parse_return_statement();
  }

  // Expression statement
  return parse_expression_statement();
}

std::unique_ptr<ast::VariableDeclaration> Parser::parse_variable_declaration() {
  return parse_variable_declaration(true); // consume semicolon by default
}

// Overloaded version that allows controlling semicolon consumption
std::unique_ptr<ast::VariableDeclaration>
Parser::parse_variable_declaration(bool consume_semicolon) {
  // Parse mutability: mut takes precedence over let
  ast::VariableDeclaration::Mutability mutability;
  if (match(tokenize::TokenType::Mut)) {
    mutability = ast::VariableDeclaration::Mutability::MUT;
  } else if (match(tokenize::TokenType::Let)) {
    mutability = ast::VariableDeclaration::Mutability::LET;
  } else {
    error("Expected 'let' or 'mut' for variable declaration");
    mutability = ast::VariableDeclaration::Mutability::LET; // fallback
  }

  std::string name = parse_identifier();

  std::unique_ptr<ast::TypeLiteral> type = nullptr;
  std::unique_ptr<ast::Expression> initializer = nullptr;

  // Optional type annotation
  if (match(tokenize::TokenType::Colon)) {
    type = parse_type_literal();
  }

  // Optional initializer
  if (match(tokenize::TokenType::Assign)) {
    initializer = parse_expression();
  }

  // Must have either type or initializer
  if (!type && !initializer) {
    error(
        "Variable declaration must have either type annotation or initializer");
  }

  if (consume_semicolon) {
    match(tokenize::TokenType::Semicolon); // Optional semicolon
  }

  return ast::ASTFactory::create_variable_declaration(
      mutability, name, std::move(type), std::move(initializer));
}

std::unique_ptr<ast::FunctionDeclaration> Parser::parse_function_declaration() {
  consume(tokenize::TokenType::Function, "Expected 'fun'");

  std::string name = parse_identifier();

  consume(tokenize::TokenType::LeftParen, "Expected '(' after function name");
  auto parameters = parse_parameter_list();
  consume(tokenize::TokenType::RightParen, "Expected ')' after parameters");

  consume(tokenize::TokenType::Colon, "Expected ':' after function parameters");
  auto return_type = parse_type_literal();

  auto function = ast::ASTFactory::create_function_declaration(
      name, std::move(parameters), std::move(return_type));

  // Parse function body
  if (match(tokenize::TokenType::Assign)) {
    // Single expression body
    auto body_expr = parse_expression();
    function->set_expression_body(std::move(body_expr));
  } else {
    // Block body
    auto block = ast::ASTFactory::create_block_expression();
    consume(tokenize::TokenType::LeftBrace,
            "Expected '{' or '=' for function body");

    while (!check(tokenize::TokenType::RightBrace) && !is_at_end()) {
      auto stmt = parse_statement();
      block->add_statement(std::move(stmt));
    }

    consume(tokenize::TokenType::RightBrace,
            "Expected '}' after function body");
    function->set_block_body(std::move(block));
  }

  return function;
}

std::unique_ptr<ast::ExpressionStatement> Parser::parse_expression_statement() {
  auto expr = parse_expression();
  match(tokenize::TokenType::Semicolon); // Optional semicolon
  return ast::ASTFactory::create_expression_statement(std::move(expr));
}

// ==================== Expressions ====================

std::unique_ptr<ast::Expression> Parser::parse_expression() {
  return parse_assignment_expression();
}

std::unique_ptr<ast::Expression> Parser::parse_assignment_expression() {
  auto expr = parse_ternary_expression();

  if (is_assignment_operator(current().get_type())) {
    auto op = token_to_assignment_operator(current().get_type());
    advance();
    auto right = parse_assignment_expression();

    return std::make_unique<ast::AssignmentExpression>(op, std::move(expr),
                                                       std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_ternary_expression() {
  auto expr = parse_null_coalescing_expression();

  if (match(tokenize::TokenType::Question)) {
    auto then_expr = parse_expression();
    consume(tokenize::TokenType::Colon, "Expected ':' in ternary expression");
    auto else_expr = parse_expression();

    return std::make_unique<ast::TernaryExpression>(
        std::move(expr), std::move(then_expr), std::move(else_expr));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_null_coalescing_expression() {
  auto expr = parse_logical_or_expression();

  while (match(tokenize::TokenType::NullCoalescing)) {
    auto right = parse_logical_or_expression();
    expr = ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::NULL_COALESCING, std::move(expr),
        std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_logical_or_expression() {
  auto expr = parse_logical_and_expression();

  while (match(tokenize::TokenType::LogicalOr)) {
    auto right = parse_logical_and_expression();
    expr = ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::LOGICAL_OR, std::move(expr),
        std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_logical_and_expression() {
  auto expr = parse_bitwise_or_expression();

  while (match(tokenize::TokenType::LogicalAnd)) {
    auto right = parse_bitwise_or_expression();
    expr = ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::LOGICAL_AND, std::move(expr),
        std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_bitwise_or_expression() {
  auto expr = parse_bitwise_xor_expression();

  while (match(tokenize::TokenType::Pipe)) {
    auto right = parse_bitwise_xor_expression();
    expr = ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::BITWISE_OR, std::move(expr),
        std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_bitwise_xor_expression() {
  auto expr = parse_bitwise_and_expression();

  while (match(tokenize::TokenType::Caret)) {
    auto right = parse_bitwise_and_expression();
    expr = ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::BITWISE_XOR, std::move(expr),
        std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_bitwise_and_expression() {
  auto expr = parse_equality_expression();

  while (match(tokenize::TokenType::Ampersand)) {
    auto right = parse_equality_expression();
    expr = ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::BITWISE_AND, std::move(expr),
        std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_equality_expression() {
  auto expr = parse_relational_expression();

  while (match({tokenize::TokenType::Equal, tokenize::TokenType::NotEqual})) {
    auto op = (previous().get_type() == tokenize::TokenType::Equal)
                  ? ast::BinaryExpression::Operator::EQUAL
                  : ast::BinaryExpression::Operator::NOT_EQUAL;
    auto right = parse_relational_expression();
    expr = ast::ASTFactory::create_binary_expression(op, std::move(expr),
                                                     std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_relational_expression() {
  auto expr = parse_type_check_expression();

  while (match({tokenize::TokenType::LessThan, tokenize::TokenType::GreaterThan,
                tokenize::TokenType::LessThanEqual,
                tokenize::TokenType::GreaterThanEqual})) {
    auto op = token_to_binary_operator(previous().get_type());
    auto right = parse_type_check_expression();
    expr = ast::ASTFactory::create_binary_expression(op, std::move(expr),
                                                     std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_type_check_expression() {
  auto expr = parse_shift_expression();

  if (match(tokenize::TokenType::Is)) {
    auto type = parse_type_literal();
    // Create a type check binary expression
    // Note: The right operand is a placeholder since type checking
    // requires special handling in semantic analysis
    return ast::ASTFactory::create_binary_expression(
        ast::BinaryExpression::Operator::TYPE_CHECK, std::move(expr),
        std::make_unique<ast::Identifier>("type_check_placeholder"));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_shift_expression() {
  auto expr = parse_additive_expression();

  while (match({tokenize::TokenType::BitwiseShiftLeft,
                tokenize::TokenType::BitwiseShiftRight})) {
    auto op = token_to_binary_operator(previous().get_type());
    auto right = parse_additive_expression();
    expr = ast::ASTFactory::create_binary_expression(op, std::move(expr),
                                                     std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_range_expression() {
  auto expr = parse_multiplicative_expression();

  if (match(tokenize::TokenType::Range)) {
    // Check for unbounded range (expr..)
    if (check(tokenize::TokenType::Comma) ||
        check(tokenize::TokenType::RightParen) ||
        check(tokenize::TokenType::RightBracket) ||
        check(tokenize::TokenType::RightBrace) ||
        check(tokenize::TokenType::Semicolon) ||
        check(tokenize::TokenType::Arrow) || is_at_end()) {
      // Unbounded range - create binary expression with null right operand
      expr = ast::ASTFactory::create_binary_expression(
          ast::BinaryExpression::Operator::RANGE, std::move(expr), nullptr);
    } else {
      // Bounded range - parse the right side
      auto right = parse_multiplicative_expression();
      expr = ast::ASTFactory::create_binary_expression(
          ast::BinaryExpression::Operator::RANGE, std::move(expr),
          std::move(right));
    }
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_additive_expression() {
  auto expr = parse_range_expression();

  while (match({tokenize::TokenType::Plus, tokenize::TokenType::Minus})) {
    auto op = (previous().get_type() == tokenize::TokenType::Plus)
                  ? ast::BinaryExpression::Operator::ADD
                  : ast::BinaryExpression::Operator::SUBTRACT;
    auto right = parse_range_expression();
    expr = ast::ASTFactory::create_binary_expression(op, std::move(expr),
                                                     std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_multiplicative_expression() {
  auto expr = parse_unary_expression();

  while (match({tokenize::TokenType::Asterisk, tokenize::TokenType::Slash,
                tokenize::TokenType::Percent})) {
    auto op = token_to_binary_operator(previous().get_type());
    auto right = parse_unary_expression();
    expr = ast::ASTFactory::create_binary_expression(op, std::move(expr),
                                                     std::move(right));
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_unary_expression() {
  if (is_unary_operator(current().get_type())) {
    auto op = token_to_unary_operator(current().get_type());
    advance();
    auto operand = parse_unary_expression();
    return ast::ASTFactory::create_unary_expression(op, std::move(operand));
  }

  return parse_postfix_expression();
}

std::unique_ptr<ast::Expression> Parser::parse_postfix_expression() {
  auto expr = parse_primary_expression();

  while (true) {
    if (match(tokenize::TokenType::Increment)) {
      expr = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::POST_INCREMENT,
          std::move(expr));
    } else if (match(tokenize::TokenType::Decrement)) {
      expr = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::POST_DECREMENT,
          std::move(expr));
    } else if (match(tokenize::TokenType::NullAssertion)) {
      // Handle null assertion operator !!
      // For now, treat as a special postfix operator
      expr = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::POST_INCREMENT, // Placeholder
          std::move(expr));
    } else if (match(tokenize::TokenType::Dot)) {
      std::string member = parse_identifier();
      // Using PostfixExpression for member access
      auto postfix = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::MEMBER_ACCESS, std::move(expr));
      postfix->set_member_name(member);
      expr = std::move(postfix);
    } else if (match(tokenize::TokenType::NullSafeAccess)) {
      std::string member = parse_identifier();
      // Using PostfixExpression for null-safe member access
      auto postfix = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::SAFE_MEMBER_ACCESS,
          std::move(expr));
      postfix->set_member_name(member);
      expr = std::move(postfix);
    } else if (match(tokenize::TokenType::LeftBracket)) {
      auto index = parse_expression();
      consume(tokenize::TokenType::RightBracket,
              "Expected ']' after array index");
      // Using PostfixExpression for array access
      auto postfix = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::ARRAY_ACCESS, std::move(expr));
      postfix->set_index_expression(std::move(index));
      expr = std::move(postfix);
    } else if (match(tokenize::TokenType::LeftParen)) {
      auto args = parse_argument_list();
      consume(tokenize::TokenType::RightParen, "Expected ')' after arguments");
      // Using PostfixExpression for function call
      auto postfix = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::FUNCTION_CALL, std::move(expr));
      postfix->set_arguments(std::move(args));
      expr = std::move(postfix);
    } else if (match(tokenize::TokenType::As)) {
      // Check for safe cast (as?)
      bool is_safe_cast = match(tokenize::TokenType::Question);
      auto type = parse_type_literal();
      auto postfix = ast::ASTFactory::create_postfix_expression(
          ast::PostfixExpression::OperatorType::CAST, std::move(expr));
      postfix->set_cast_type(std::move(type));
      postfix->set_safe_cast(is_safe_cast);
      expr = std::move(postfix);
    } else {
      break;
    }
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::parse_primary_expression() {
  if (check(tokenize::TokenType::Identifier)) {
    // Check if this is a labeled expression: identifier @ expression
    size_t saved_pos = m_current_position;
    std::string name = parse_identifier();

    if (match(tokenize::TokenType::At)) {
      // This is a labeled expression: name @ expression
      auto expr = parse_primary_expression(); // Parse the expression after @
      // For now, just return the expression (labels would need special AST
      // support)
      return expr;
    } else {
      // Restore position and parse as regular identifier
      m_current_position = saved_pos;
      std::string identifier_name = parse_identifier();
      return ast::ASTFactory::create_identifier(identifier_name);
    }
  }

  if (check(tokenize::TokenType::Number)) {
    return parse_number_literal();
  }

  if (check(tokenize::TokenType::String)) {
    return parse_string_literal();
  }

  if (check(tokenize::TokenType::Character)) {
    // For now, treat character literals as string literals until AST is updated
    auto token = consume(tokenize::TokenType::Character, "Expected character");
    std::string value = token.get_literal().to_string();
    return ast::ASTFactory::create_string_literal(value);
  }

  if (check(tokenize::TokenType::True) || check(tokenize::TokenType::False)) {
    return parse_boolean_literal();
  }

  if (match(tokenize::TokenType::Null)) {
    return ast::ASTFactory::create_null_literal();
  }

  if (match(tokenize::TokenType::None)) {
    // For now, treat None as null until AST is updated
    return ast::ASTFactory::create_null_literal();
  }

  if (match(tokenize::TokenType::LeftParen)) {
    // Check if this is a lambda expression by looking ahead
    size_t saved_pos = m_current_position;
    bool is_lambda = false;

    // Check for lambda patterns: (), (params...) =>
    if (check(tokenize::TokenType::RightParen)) {
      advance(); // consume ')'
      if (check(tokenize::TokenType::FatArrow)) {
        is_lambda = true;
      }
    } else if (check(tokenize::TokenType::Let) ||
               check(tokenize::TokenType::Mut)) {
      // Has parameters, look for => after parameter list
      int paren_count = 1;
      while (paren_count > 0 && !is_at_end()) {
        if (check(tokenize::TokenType::LeftParen)) {
          paren_count++;
        } else if (check(tokenize::TokenType::RightParen)) {
          paren_count--;
        }
        advance();
      }
      if (check(tokenize::TokenType::FatArrow)) {
        is_lambda = true;
      }
    }

    // Restore position
    m_current_position = saved_pos;

    if (is_lambda) {
      return parse_lambda_expression();
    } else {
      // Regular parenthesized expression
      auto expr = parse_expression();
      consume(tokenize::TokenType::RightParen, "Expected ')' after expression");
      return expr;
    }
  }

  if (check(tokenize::TokenType::LeftBracket)) {
    return parse_array_literal();
  }

  if (check(tokenize::TokenType::LeftBrace)) {
    // Need to determine if this is a block expression or object literal
    // Look ahead to see what comes after the opening brace
    size_t saved_pos = m_current_position;
    advance(); // consume '{'

    bool is_object_literal = false;

    if (!check(tokenize::TokenType::RightBrace)) {
      // Check for object property patterns
      if (check(tokenize::TokenType::Identifier) ||
          check(tokenize::TokenType::String) ||
          check(tokenize::TokenType::Character)) {
        advance();
        if (check(tokenize::TokenType::Colon)) {
          is_object_literal = true;
        }
      } else if (check(tokenize::TokenType::LeftBracket)) {
        // Computed property [key]: value
        is_object_literal = true;
      } else if (check(tokenize::TokenType::Spread)) {
        // Spread operator ...obj
        is_object_literal = true;
      }
    } else {
      // Empty braces {} - treat as empty object literal
      is_object_literal = true;
    }

    // Restore position
    m_current_position = saved_pos;

    if (is_object_literal) {
      return parse_object_literal();
    } else {
      return parse_block_expression();
    }
  }

  if (check(tokenize::TokenType::If)) {
    return parse_if_expression();
  }

  if (check(tokenize::TokenType::When)) {
    return parse_when_expression();
  }

  if (check(tokenize::TokenType::Function)) {
    return parse_function_expression();
  }

  error("Expected expression");
  return nullptr;
}

// ==================== Literals ====================

std::unique_ptr<ast::NumberLiteral> Parser::parse_number_literal() {
  auto token = consume(tokenize::TokenType::Number, "Expected number");
  std::string value = token.get_literal().to_string();

  // Determine number type based on format
  ast::NumberLiteral::NumberType type =
      ast::NumberLiteral::NumberType::DECIMAL_INTEGER;
  if (value.find('.') != std::string::npos) {
    type = ast::NumberLiteral::NumberType::FLOATING_POINT;
  } else if (value.substr(0, 2) == "0x" || value.substr(0, 2) == "0X") {
    type = ast::NumberLiteral::NumberType::HEXADECIMAL_INTEGER;
  } else if (value.substr(0, 2) == "0b" || value.substr(0, 2) == "0B") {
    type = ast::NumberLiteral::NumberType::BINARY_INTEGER;
  } else if (value.substr(0, 2) == "0o" || value.substr(0, 2) == "0O") {
    type = ast::NumberLiteral::NumberType::OCTAL_INTEGER;
  }

  return ast::ASTFactory::create_number_literal(value, type);
}

std::unique_ptr<ast::StringLiteral> Parser::parse_string_literal() {
  auto token = consume(tokenize::TokenType::String, "Expected string");
  std::string value = token.get_literal().to_string();

  return ast::ASTFactory::create_string_literal(value);
}

std::unique_ptr<ast::BooleanLiteral> Parser::parse_boolean_literal() {
  if (match(tokenize::TokenType::True)) {
    return ast::ASTFactory::create_boolean_literal(true);
  } else if (match(tokenize::TokenType::False)) {
    return ast::ASTFactory::create_boolean_literal(false);
  }

  error("Expected boolean literal");
  return nullptr;
}

std::unique_ptr<ast::ArrayLiteral> Parser::parse_array_literal() {
  consume(tokenize::TokenType::LeftBracket, "Expected '['");

  std::vector<std::unique_ptr<ast::Expression>> elements;

  if (!check(tokenize::TokenType::RightBracket)) {
    do {
      elements.push_back(parse_expression());
    } while (match(tokenize::TokenType::Comma));
  }

  consume(tokenize::TokenType::RightBracket,
          "Expected ']' after array elements");
  return ast::ASTFactory::create_array_literal(std::move(elements));
}

std::unique_ptr<ast::ObjectLiteral> Parser::parse_object_literal() {
  consume(tokenize::TokenType::LeftBrace, "Expected '{'");

  auto properties = parse_object_property_list();

  consume(tokenize::TokenType::RightBrace,
          "Expected '}' after object properties");
  return ast::ASTFactory::create_object_literal(std::move(properties));
}

// ==================== Utility Methods ====================

std::string Parser::parse_identifier() {
  auto token = consume(tokenize::TokenType::Identifier, "Expected identifier");
  return token.get_literal().to_string();
}

bool Parser::is_assignment_operator(tokenize::TokenType type) const {
  return type == tokenize::TokenType::Assign ||
         type == tokenize::TokenType::PlusAssign ||
         type == tokenize::TokenType::MinusAssign ||
         type == tokenize::TokenType::AsteriskAssign ||
         type == tokenize::TokenType::SlashAssign ||
         type == tokenize::TokenType::PercentAssign ||
         type == tokenize::TokenType::AmpersandAssign ||
         type == tokenize::TokenType::PipeAssign ||
         type == tokenize::TokenType::CaretAssign ||
         type == tokenize::TokenType::TildeAssign;
}

bool Parser::is_unary_operator(tokenize::TokenType type) const {
  return type == tokenize::TokenType::Plus ||
         type == tokenize::TokenType::Minus ||
         type == tokenize::TokenType::Exclamation ||
         type == tokenize::TokenType::LogicalNot ||
         type == tokenize::TokenType::Tilde ||
         type == tokenize::TokenType::Increment ||
         type == tokenize::TokenType::Decrement;
}

ast::BinaryExpression::Operator
Parser::token_to_binary_operator(tokenize::TokenType type) const {
  switch (type) {
  case tokenize::TokenType::Plus:
    return ast::BinaryExpression::Operator::ADD;
  case tokenize::TokenType::Minus:
    return ast::BinaryExpression::Operator::SUBTRACT;
  case tokenize::TokenType::Asterisk:
    return ast::BinaryExpression::Operator::MULTIPLY;
  case tokenize::TokenType::Slash:
    return ast::BinaryExpression::Operator::DIVIDE;
  case tokenize::TokenType::Percent:
    return ast::BinaryExpression::Operator::MODULO;
  case tokenize::TokenType::Equal:
    return ast::BinaryExpression::Operator::EQUAL;
  case tokenize::TokenType::NotEqual:
    return ast::BinaryExpression::Operator::NOT_EQUAL;
  case tokenize::TokenType::LessThan:
    return ast::BinaryExpression::Operator::LESS_THAN;
  case tokenize::TokenType::GreaterThan:
    return ast::BinaryExpression::Operator::GREATER_THAN;
  case tokenize::TokenType::LessThanEqual:
    return ast::BinaryExpression::Operator::LESS_EQUAL;
  case tokenize::TokenType::GreaterThanEqual:
    return ast::BinaryExpression::Operator::GREATER_EQUAL;
  case tokenize::TokenType::LogicalAnd:
    return ast::BinaryExpression::Operator::LOGICAL_AND;
  case tokenize::TokenType::LogicalOr:
    return ast::BinaryExpression::Operator::LOGICAL_OR;
  case tokenize::TokenType::Ampersand:
    return ast::BinaryExpression::Operator::BITWISE_AND;
  case tokenize::TokenType::Pipe:
    return ast::BinaryExpression::Operator::BITWISE_OR;
  case tokenize::TokenType::Caret:
    return ast::BinaryExpression::Operator::BITWISE_XOR;
  case tokenize::TokenType::BitwiseShiftLeft:
    return ast::BinaryExpression::Operator::LEFT_SHIFT;
  case tokenize::TokenType::BitwiseShiftRight:
    return ast::BinaryExpression::Operator::RIGHT_SHIFT;
  case tokenize::TokenType::Range:
    return ast::BinaryExpression::Operator::RANGE;
  case tokenize::TokenType::NullCoalescing:
    return ast::BinaryExpression::Operator::NULL_COALESCING;
  case tokenize::TokenType::Is:
    return ast::BinaryExpression::Operator::TYPE_CHECK;
  case tokenize::TokenType::In:
    return ast::BinaryExpression::Operator::IN;
  default:
    error("Invalid binary operator");
    return ast::BinaryExpression::Operator::ADD; // fallback
  }
}

ast::UnaryExpression::Operator
Parser::token_to_unary_operator(tokenize::TokenType type) const {
  switch (type) {
  case tokenize::TokenType::Plus:
    return ast::UnaryExpression::Operator::PLUS;
  case tokenize::TokenType::Minus:
    return ast::UnaryExpression::Operator::MINUS;
  case tokenize::TokenType::Exclamation:
    return ast::UnaryExpression::Operator::LOGICAL_NOT;
  case tokenize::TokenType::LogicalNot:
    return ast::UnaryExpression::Operator::LOGICAL_NOT;
  case tokenize::TokenType::Tilde:
    return ast::UnaryExpression::Operator::BITWISE_NOT;
  case tokenize::TokenType::Increment:
    return ast::UnaryExpression::Operator::PRE_INCREMENT;
  case tokenize::TokenType::Decrement:
    return ast::UnaryExpression::Operator::PRE_DECREMENT;
  default:
    error("Invalid unary operator");
    return ast::UnaryExpression::Operator::PLUS; // fallback
  }
}

ast::AssignmentExpression::Operator
Parser::token_to_assignment_operator(tokenize::TokenType type) const {
  switch (type) {
  case tokenize::TokenType::Assign:
    return ast::AssignmentExpression::Operator::ASSIGN;
  case tokenize::TokenType::PlusAssign:
    return ast::AssignmentExpression::Operator::ADD_ASSIGN;
  case tokenize::TokenType::MinusAssign:
    return ast::AssignmentExpression::Operator::SUB_ASSIGN;
  case tokenize::TokenType::AsteriskAssign:
    return ast::AssignmentExpression::Operator::MUL_ASSIGN;
  case tokenize::TokenType::SlashAssign:
    return ast::AssignmentExpression::Operator::DIV_ASSIGN;
  case tokenize::TokenType::PercentAssign:
    return ast::AssignmentExpression::Operator::MOD_ASSIGN;
  case tokenize::TokenType::AmpersandAssign:
    return ast::AssignmentExpression::Operator::BITWISE_AND_ASSIGN;
  case tokenize::TokenType::PipeAssign:
    return ast::AssignmentExpression::Operator::BITWISE_OR_ASSIGN;
  case tokenize::TokenType::CaretAssign:
    return ast::AssignmentExpression::Operator::BITWISE_XOR_ASSIGN;
  case tokenize::TokenType::TildeAssign:
    return ast::AssignmentExpression::Operator::BITWISE_NOT_ASSIGN;
  default:
    error("Invalid assignment operator");
    return ast::AssignmentExpression::Operator::ASSIGN; // fallback
  }
}

// ==================== Control Flow Statements ====================

std::unique_ptr<ast::BreakStatement> Parser::parse_break_statement() {
  consume(tokenize::TokenType::Break, "Expected 'break'");

  std::string label = "";
  if (match(tokenize::TokenType::At)) {
    label = parse_identifier();
  }

  match(tokenize::TokenType::Semicolon); // Optional semicolon
  return ast::ASTFactory::create_break_statement(label);
}

std::unique_ptr<ast::ContinueStatement> Parser::parse_continue_statement() {
  consume(tokenize::TokenType::Continue, "Expected 'continue'");

  std::string label = "";
  if (match(tokenize::TokenType::At)) {
    label = parse_identifier();
  }

  match(tokenize::TokenType::Semicolon); // Optional semicolon
  return ast::ASTFactory::create_continue_statement(label);
}

std::unique_ptr<ast::ReturnStatement> Parser::parse_return_statement() {
  consume(tokenize::TokenType::Return, "Expected 'return'");

  std::string label = "";
  std::unique_ptr<ast::Expression> value = nullptr;

  if (match(tokenize::TokenType::At)) {
    label = parse_identifier();
  }

  // Optional return value
  if (!check(tokenize::TokenType::Semicolon) && !is_at_end()) {
    value = parse_expression();
  }

  match(tokenize::TokenType::Semicolon); // Optional semicolon
  return ast::ASTFactory::create_return_statement(std::move(value), label);
}

// ==================== Complex Expressions ====================

std::unique_ptr<ast::BlockExpression> Parser::parse_block_expression() {
  consume(tokenize::TokenType::LeftBrace, "Expected '{'");

  auto block = ast::ASTFactory::create_block_expression();

  while (!check(tokenize::TokenType::RightBrace) && !is_at_end()) {
    // Block expression can contain statements and a final expression
    if (check(tokenize::TokenType::Let) || check(tokenize::TokenType::Mut) ||
        check(tokenize::TokenType::Function) ||
        check(tokenize::TokenType::If) || check(tokenize::TokenType::For) ||
        check(tokenize::TokenType::Break) ||
        check(tokenize::TokenType::Continue) ||
        check(tokenize::TokenType::Return)) {
      auto stmt = parse_statement();
      if (stmt) {
        block->add_statement(std::move(stmt));
      }
    } else {
      // Final expression (no semicolon means it's the return value)
      auto expr = parse_expression();
      if (expr) {
        block->set_result_expression(std::move(expr));
      }
      break;
    }
  }

  consume(tokenize::TokenType::RightBrace, "Expected '}' after block");
  return block;
}

std::unique_ptr<ast::IfExpression> Parser::parse_if_expression() {
  consume(tokenize::TokenType::If, "Expected 'if'");
  consume(tokenize::TokenType::LeftParen, "Expected '(' after 'if'");

  auto condition = parse_expression();
  consume(tokenize::TokenType::RightParen, "Expected ')' after if condition");

  auto then_block = parse_block_expression();

  auto if_expr = std::make_unique<ast::IfExpression>(std::move(condition),
                                                     std::move(then_block));

  if (match(tokenize::TokenType::Else)) {
    auto else_expr =
        parse_expression(); // Can be another IfExpression or BlockExpression
    if_expr->set_else_block(std::move(else_expr));
  }

  return if_expr;
}

// ==================== Type Parsing ====================

std::unique_ptr<ast::TypeLiteral> Parser::parse_type_literal() {
  std::unique_ptr<ast::TypeLiteral> type = nullptr;

  if (check(tokenize::TokenType::Identifier)) {
    std::string type_name = parse_identifier();
    type = ast::ASTFactory::create_simple_type(type_name);
  } else if (match(tokenize::TokenType::LeftParen)) {
    // Function type or tuple type
    std::vector<std::unique_ptr<ast::TypeLiteral>> types;

    if (!check(tokenize::TokenType::RightParen)) {
      do {
        types.push_back(parse_type_literal());
      } while (match(tokenize::TokenType::Comma));
    }

    consume(tokenize::TokenType::RightParen, "Expected ')' after type list");

    // Check if it's a function type (has -> after)
    if (match(tokenize::TokenType::Arrow)) {
      auto return_type = parse_type_literal();
      type = ast::ASTFactory::create_function_type(std::move(types),
                                                   std::move(return_type));
    } else {
      // It's a tuple type - 일단 첫 번째 타입만 반환 (TupleType이 없음)
      if (!types.empty()) {
        type = std::move(types[0]);
      } else {
        error("Empty tuple type not supported");
        return nullptr;
      }
    }
  } else {
    error("Expected type literal");
    return nullptr;
  }

  // Handle optional types recursively (Type?, Type??, etc.)
  while (match(tokenize::TokenType::Question)) {
    type = ast::ASTFactory::create_optional_type(std::move(type));
  }

  return type;
}

// ==================== Parameter and Argument Lists ====================

std::vector<std::unique_ptr<ast::Parameter>> Parser::parse_parameter_list() {
  std::vector<std::unique_ptr<ast::Parameter>> parameters;

  if (!check(tokenize::TokenType::RightParen)) {
    do {
      auto param = parse_parameter();
      if (param) {
        parameters.push_back(std::move(param));
      }
    } while (match(tokenize::TokenType::Comma));
  }

  return parameters;
}

std::unique_ptr<ast::Parameter> Parser::parse_parameter() {
  // Parse mutability
  auto mutability = ast::Parameter::Mutability::LET;
  if (match(tokenize::TokenType::Mut)) {
    mutability = ast::Parameter::Mutability::MUT;
  } else {
    consume(tokenize::TokenType::Let, "Expected 'let' or 'mut' in parameter");
  }

  std::string name = parse_identifier();
  consume(tokenize::TokenType::Colon, "Expected ':' after parameter name");
  auto type = parse_type_literal();

  std::unique_ptr<ast::Expression> default_value = nullptr;
  if (match(tokenize::TokenType::Assign)) {
    default_value = parse_expression();
  }

  return ast::ASTFactory::create_parameter(mutability, name, std::move(type),
                                           std::move(default_value));
}

std::vector<std::unique_ptr<ast::Expression>> Parser::parse_argument_list() {
  std::vector<std::unique_ptr<ast::Expression>> arguments;

  if (!check(tokenize::TokenType::RightParen)) {
    do {
      arguments.push_back(parse_expression());
    } while (match(tokenize::TokenType::Comma));
  }

  return arguments;
}

// ==================== Object Properties ====================

std::vector<std::unique_ptr<ast::ObjectProperty>>
Parser::parse_object_property_list() {
  std::vector<std::unique_ptr<ast::ObjectProperty>> properties;

  if (!check(tokenize::TokenType::RightBrace)) {
    do {
      auto property = parse_object_property();
      if (property) {
        properties.push_back(std::move(property));
      }
    } while (match(tokenize::TokenType::Comma));
  }

  return properties;
}

std::unique_ptr<ast::ObjectProperty> Parser::parse_object_property() {
  // Check for spread operator
  if (match(tokenize::TokenType::Spread)) {
    auto expr = parse_expression();
    auto property = std::make_unique<ast::ObjectProperty>(
        ast::ObjectProperty::PropertyType::SPREAD);
    property->set_value(std::move(expr));
    return property;
  }

  // Check for computed property [key]: value
  if (match(tokenize::TokenType::LeftBracket)) {
    auto key = parse_expression();
    consume(tokenize::TokenType::RightBracket,
            "Expected ']' after computed property key");
    consume(tokenize::TokenType::Colon,
            "Expected ':' after computed property key");
    auto value = parse_expression();

    auto property = std::make_unique<ast::ObjectProperty>(
        ast::ObjectProperty::PropertyType::COMPUTED);
    property->set_key(std::move(key));
    property->set_value(std::move(value));
    return property;
  }

  // String and Character literal properties: "key": value or 'k': value
  if (check(tokenize::TokenType::String) ||
      check(tokenize::TokenType::Character)) {
    auto key_literal = (current().get_type() == tokenize::TokenType::String)
                           ? parse_string_literal()
                           : std::unique_ptr<ast::Expression>(
                                 ast::ASTFactory::create_string_literal(
                                     advance().get_literal().to_string()));

    consume(tokenize::TokenType::Colon,
            "Expected ':' after string/character property key");
    auto value = parse_expression();

    auto property = std::make_unique<ast::ObjectProperty>(
        ast::ObjectProperty::PropertyType::NORMAL);
    property->set_key(std::move(key_literal));
    property->set_value(std::move(value));
    return property;
  }

  // Normal property: identifier: value or shorthand
  if (check(tokenize::TokenType::Identifier)) {
    std::string key_name = parse_identifier();

    if (match(tokenize::TokenType::Colon)) {
      // Normal property
      auto key = ast::ASTFactory::create_identifier(key_name);
      auto value = parse_expression();

      auto property = std::make_unique<ast::ObjectProperty>(
          ast::ObjectProperty::PropertyType::NORMAL);
      property->set_key(std::move(key));
      property->set_value(std::move(value));
      return property;
    } else {
      // Shorthand property
      auto property = std::make_unique<ast::ObjectProperty>(
          ast::ObjectProperty::PropertyType::SHORTHAND);
      auto identifier = ast::ASTFactory::create_identifier(key_name);
      property->set_key(std::move(identifier));
      return property;
    }
  }

  error("Expected object property");
  return nullptr;
}

// ==================== Utility Methods ====================

std::string Parser::parse_label() {
  consume(tokenize::TokenType::At, "Expected '@' at start of label");
  std::string label = parse_identifier();
  return label;
}

// ==================== Stub implementations ====================
// These would need full implementation based on specific AST node requirements

std::unique_ptr<ast::StructDeclaration> Parser::parse_struct_declaration() {
  consume(tokenize::TokenType::Struct, "Expected 'struct'");

  std::string struct_name = parse_identifier();

  consume(tokenize::TokenType::LeftBrace, "Expected '{' after struct name");

  auto fields = parse_struct_field_list();

  consume(tokenize::TokenType::RightBrace, "Expected '}' after struct fields");
  match(tokenize::TokenType::Semicolon); // Optional semicolon

  return std::make_unique<ast::StructDeclaration>(struct_name,
                                                  std::move(fields));
}

std::unique_ptr<ast::InterfaceDeclaration>
Parser::parse_interface_declaration() {
  consume(tokenize::TokenType::Interface, "Expected 'interface'");

  std::string interface_name = parse_identifier();

  consume(tokenize::TokenType::LeftBrace, "Expected '{' after interface name");

  // Parse interface members
  std::vector<std::unique_ptr<ast::InterfaceDeclaration::Member>> members;

  while (!check(tokenize::TokenType::RightBrace) && !is_at_end()) {
    std::string member_name = parse_identifier();
    consume(tokenize::TokenType::Colon,
            "Expected ':' after interface member name");
    auto member_type = parse_type_literal();

    auto member = std::make_unique<ast::InterfaceDeclaration::Member>(
        ast::InterfaceDeclaration::Member::Type::PROPERTY, member_name,
        std::move(member_type));

    members.push_back(std::move(member));

    if (!match(tokenize::TokenType::Comma)) {
      break;
    }
  }

  consume(tokenize::TokenType::RightBrace,
          "Expected '}' after interface members");
  match(tokenize::TokenType::Semicolon); // Optional semicolon

  return std::make_unique<ast::InterfaceDeclaration>(interface_name,
                                                     std::move(members));
}

std::vector<std::unique_ptr<ast::StructField>>
Parser::parse_struct_field_list() {
  std::vector<std::unique_ptr<ast::StructField>> fields;

  if (!check(tokenize::TokenType::RightBrace)) {
    do {
      auto field = parse_struct_field();
      if (field) {
        fields.push_back(std::move(field));
      }
    } while (match(tokenize::TokenType::Comma));
  }

  return fields;
}

std::unique_ptr<ast::StructField> Parser::parse_struct_field() {
  std::string field_name = parse_identifier();
  consume(tokenize::TokenType::Colon, "Expected ':' after struct field name");
  auto field_type = parse_type_literal();

  std::unique_ptr<ast::Expression> default_value = nullptr;
  if (match(tokenize::TokenType::Assign)) {
    default_value = parse_expression();
  }

  return std::make_unique<ast::StructField>(field_name, std::move(field_type),
                                            std::move(default_value));
}

std::unique_ptr<ast::IfStatement> Parser::parse_if_statement() {
  consume(tokenize::TokenType::If, "Expected 'if'");

  auto if_stmt = ast::ASTFactory::create_if_statement();

  // Parse condition (optional parentheses)
  std::unique_ptr<ast::Expression> condition = nullptr;
  if (match(tokenize::TokenType::LeftParen)) {
    condition = parse_expression();
    consume(tokenize::TokenType::RightParen, "Expected ')' after if condition");
  } else {
    condition = parse_expression();
  }

  // Parse then block
  auto then_block = parse_block_expression();
  if_stmt->add_if_branch(std::move(condition), std::move(then_block));

  // Parse elif branches
  while (match(tokenize::TokenType::Elif)) {
    std::unique_ptr<ast::Expression> elif_condition = nullptr;
    if (match(tokenize::TokenType::LeftParen)) {
      elif_condition = parse_expression();
      consume(tokenize::TokenType::RightParen,
              "Expected ')' after elif condition");
    } else {
      elif_condition = parse_expression();
    }

    auto elif_block = parse_block_expression();
    if_stmt->add_else_if_branch(std::move(elif_condition),
                                std::move(elif_block));
  }

  // Parse else branch
  if (match(tokenize::TokenType::Else)) {
    auto else_block = parse_block_expression();
    if_stmt->add_else_branch(std::move(else_block));
  }

  return if_stmt;
}

std::unique_ptr<ast::ForStatement> Parser::parse_for_statement() {
  consume(tokenize::TokenType::For, "Expected 'for'");

  // Check if it's infinite loop
  if (check(tokenize::TokenType::LeftBrace)) {
    auto for_stmt = ast::ASTFactory::create_for_statement(
        ast::ForStatement::ForType::INFINITE);
    auto body = parse_block_expression();
    for_stmt->set_body(std::move(body));
    return for_stmt;
  }

  consume(tokenize::TokenType::LeftParen, "Expected '(' after 'for'");

  // Check for for-in loop
  if (check(tokenize::TokenType::Identifier)) {
    size_t saved_pos = m_current_position;
    std::string identifier = parse_identifier();

    if (match(tokenize::TokenType::In)) {
      // For-in loop
      auto for_stmt = ast::ASTFactory::create_for_statement(
          ast::ForStatement::ForType::FOR_IN);
      for_stmt->set_iterator_variable(identifier);

      auto iterable = parse_expression();
      for_stmt->set_iterable_expression(std::move(iterable));

      consume(tokenize::TokenType::RightParen,
              "Expected ')' after for-in clause");
      auto body = parse_block_expression();
      for_stmt->set_body(std::move(body));

      return for_stmt;
    } else {
      // Backtrack
      m_current_position = saved_pos;
    }
  }

  // Parse C-style or while-style loop
  std::unique_ptr<ast::Statement> init_stmt = nullptr;
  std::unique_ptr<ast::Expression> condition = nullptr;
  std::unique_ptr<ast::Expression> increment = nullptr;

  // Check if there's an init statement
  if (check(tokenize::TokenType::Let) || check(tokenize::TokenType::Mut)) {
    init_stmt = parse_variable_declaration(false); // Don't consume semicolon

    if (match(tokenize::TokenType::Semicolon)) {
      // C-style loop with condition and increment
      if (!check(tokenize::TokenType::Semicolon)) {
        condition = parse_expression();
      }

      consume(tokenize::TokenType::Semicolon, "Expected ';' after condition");

      if (!check(tokenize::TokenType::RightParen)) {
        increment = parse_expression();
      }

      auto for_stmt = ast::ASTFactory::create_for_statement(
          ast::ForStatement::ForType::C_STYLE);
      for_stmt->set_init_statement(std::move(init_stmt));
      if (condition) {
        for_stmt->set_condition(std::move(condition));
      }
      if (increment) {
        for_stmt->set_increment_expression(std::move(increment));
      }

      consume(tokenize::TokenType::RightParen, "Expected ')' after for clause");
      auto body = parse_block_expression();
      for_stmt->set_body(std::move(body));

      return for_stmt;
    } else {
      // This should not happen for C-style for loop
      error("Expected ';' after for loop initialization");
    }
  } else {
    // While-style loop (just condition)
    condition = parse_expression();

    auto for_stmt = ast::ASTFactory::create_for_statement(
        ast::ForStatement::ForType::WHILE);
    for_stmt->set_condition(std::move(condition));

    consume(tokenize::TokenType::RightParen,
            "Expected ')' after for condition");
    auto body = parse_block_expression();
    for_stmt->set_body(std::move(body));

    return for_stmt;
  }

  error("Invalid for loop syntax");
  return nullptr;
}

std::unique_ptr<ast::WhenExpression> Parser::parse_when_expression() {
  consume(tokenize::TokenType::When, "Expected 'when'");
  consume(tokenize::TokenType::LeftParen, "Expected '(' after 'when'");

  auto scrutinee = parse_expression();
  consume(tokenize::TokenType::RightParen,
          "Expected ')' after when expression");

  auto when_expr = std::make_unique<ast::WhenExpression>(std::move(scrutinee));

  consume(tokenize::TokenType::LeftBrace, "Expected '{' after when expression");

  // Parse when branches
  while (!check(tokenize::TokenType::RightBrace) && !is_at_end()) {
    if (match(tokenize::TokenType::Else)) {
      consume(tokenize::TokenType::Arrow, "Expected '->' after 'else'");
      auto else_expr = parse_expression();
      when_expr->set_else_branch(std::move(else_expr));
      break;
    } else {
      // Parse multiple conditions separated by commas
      auto first_condition = parse_when_condition();

      // Skip additional conditions for now (until AST supports them)
      while (match(tokenize::TokenType::Comma)) {
        auto unused_condition = parse_when_condition();
      }

      consume(tokenize::TokenType::Arrow, "Expected '->' after when condition");
      auto expr = parse_expression();

      when_expr->add_branch(std::move(first_condition), std::move(expr));

      match(tokenize::TokenType::Comma); // Optional comma between branches
    }
  }

  consume(tokenize::TokenType::RightBrace, "Expected '}' after when branches");
  return when_expr;
}

std::unique_ptr<ast::FunctionExpression> Parser::parse_function_expression() {
  consume(tokenize::TokenType::Function, "Expected 'fun'");
  consume(tokenize::TokenType::LeftParen, "Expected '(' after 'fun'");

  auto parameters = parse_parameter_list();
  consume(tokenize::TokenType::RightParen, "Expected ')' after parameters");

  consume(tokenize::TokenType::Colon, "Expected ':' after parameters");
  auto return_type = parse_type_literal();

  auto func_expr = std::make_unique<ast::FunctionExpression>(
      std::move(parameters), std::move(return_type));

  if (match(tokenize::TokenType::Assign)) {
    // Expression body
    auto body_expr = parse_expression();
    func_expr->set_expression_body(std::move(body_expr));
  } else {
    // Block body
    auto body_block = parse_block_expression();
    func_expr->set_block_body(std::move(body_block));
  }

  return func_expr;
}

std::unique_ptr<ast::LambdaExpression> Parser::parse_lambda_expression() {
  // Note: The opening '(' has already been consumed in parse_primary_expression
  auto parameters = parse_parameter_list();
  consume(tokenize::TokenType::RightParen,
          "Expected ')' after lambda parameters");

  consume(tokenize::TokenType::FatArrow,
          "Expected '=>' after lambda parameters");

  auto lambda_expr =
      std::make_unique<ast::LambdaExpression>(std::move(parameters));

  if (check(tokenize::TokenType::LeftBrace)) {
    // Block body
    auto body_block = parse_block_expression();
    lambda_expr->set_block_body(std::move(body_block));
  } else {
    // Expression body
    auto body_expr = parse_expression();
    lambda_expr->set_expression_body(std::move(body_expr));
  }

  return lambda_expr;
}

// ==================== When Conditions ====================

std::unique_ptr<ast::WhenCondition> Parser::parse_when_condition() {
  auto value = parse_expression();

  // Check for range condition (value in range)
  if (match(tokenize::TokenType::In)) {
    auto range = parse_expression();
    return ast::ASTFactory::create_range_condition(std::move(value),
                                                   std::move(range));
  }

  // Check for type condition (value is Type)
  if (match(tokenize::TokenType::Is)) {
    auto type = parse_type_literal();
    return ast::ASTFactory::create_type_condition(std::move(value),
                                                  std::move(type));
  }

  // Create value condition
  auto value_condition =
      ast::ASTFactory::create_value_condition(std::move(value));

  // Check for guard condition (condition if guard)
  if (match(tokenize::TokenType::If)) {
    auto guard = parse_expression();
    return ast::ASTFactory::create_guard_condition(std::move(value_condition),
                                                   std::move(guard));
  }

  return value_condition;
}

std::unique_ptr<ast::ValueCondition> Parser::parse_value_condition() {
  auto value = parse_expression();
  return ast::ASTFactory::create_value_condition(std::move(value));
}

std::unique_ptr<ast::RangeCondition> Parser::parse_range_condition() {
  auto value = parse_expression();
  consume(tokenize::TokenType::In, "Expected 'in' for range condition");
  auto range = parse_expression();
  return ast::ASTFactory::create_range_condition(std::move(value),
                                                 std::move(range));
}

std::unique_ptr<ast::TypeCondition> Parser::parse_type_condition() {
  auto value = parse_expression();
  consume(tokenize::TokenType::Is, "Expected 'is' for type condition");
  auto type = parse_type_literal();
  return ast::ASTFactory::create_type_condition(std::move(value),
                                                std::move(type));
}

std::unique_ptr<ast::GuardCondition> Parser::parse_guard_condition() {
  auto base_condition = parse_when_condition();
  consume(tokenize::TokenType::If, "Expected 'if' for guard condition");
  auto guard = parse_expression();
  return ast::ASTFactory::create_guard_condition(std::move(base_condition),
                                                 std::move(guard));
}

std::unique_ptr<ast::MultipleCondition> Parser::parse_multiple_condition() {
  auto multiple = ast::ASTFactory::create_multiple_condition();

  do {
    auto condition = parse_when_condition();
    multiple->add_condition(std::move(condition));
  } while (match(tokenize::TokenType::Comma));

  return multiple;
}

// ==================== Import/Export Statements ====================

std::unique_ptr<ast::ImportStatement> Parser::parse_import_statement() {
  consume(tokenize::TokenType::Import, "Expected 'import'");

  // Parse string literal for import path
  auto path_token =
      consume(tokenize::TokenType::String, "Expected import path string");
  std::string import_path = path_token.get_literal().to_string();

  std::string alias = "";
  if (match(tokenize::TokenType::As)) {
    alias = parse_identifier();
  }

  match(tokenize::TokenType::Semicolon); // Optional semicolon

  return std::make_unique<ast::ImportStatement>(import_path, alias);
}

std::unique_ptr<ast::ExportStatement> Parser::parse_export_statement() {
  consume(tokenize::TokenType::Export, "Expected 'export'");

  // Check if we're at the end or have an invalid token for export
  if (is_at_end() ||
      (!check(tokenize::TokenType::Let) && !check(tokenize::TokenType::Mut) &&
       !check(tokenize::TokenType::Function) &&
       !check(tokenize::TokenType::Struct) &&
       !check(tokenize::TokenType::Interface))) {
    error("Expected statement after 'export'");
  }

  // Parse the statement to be exported
  auto exported_statement = parse_statement();

  return std::make_unique<ast::ExportStatement>(std::move(exported_statement));
}

} // namespace nugdev::compiler::parsing