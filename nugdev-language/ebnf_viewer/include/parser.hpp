#pragma once

#include "ast.hpp"
#include <memory>
#include <string>
#include <unordered_map>


namespace ebnf {

class Parser {
public:
  Parser();
  ~Parser() = default;

  ASTNodePtr parse(const std::string &input);
  const std::unordered_map<std::string, ASTNodePtr> &getRules() const {
    return rules_;
  }

private:
  ASTNodePtr parseRule();
  ASTNodePtr parseExpression();
  ASTNodePtr parseTerm();
  ASTNodePtr parseFactor();
  ASTNodePtr parsePrimary();
  ASTNodePtr parseIdentifier();
  ASTNodePtr parseString();
  ASTNodePtr parseComment();

  void skipWhitespace();
  bool match(char c);
  bool match(const std::string &str);
  char peek() const;
  char advance();
  bool isAtEnd() const;

  std::string input_;
  size_t current_;
  std::unordered_map<std::string, ASTNodePtr> rules_;
};

} // namespace ebnf