#include "parser.hpp"
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace ebnf {

Parser::Parser() : current_(0) {}

ASTNodePtr Parser::parse(const std::string &input) {
  input_ = input;
  current_ = 0;
  rules_.clear();

  int ruleCount = 0;
  while (!isAtEnd()) {
    skipWhitespace();
    if (isAtEnd())
      break;

    std::cerr << "Parsing rule #" << ruleCount << ", current=" << current_
              << std::endl;
    auto prev = current_;
    auto rule = parseRule();
    if (rule) {
      rules_[rule->getValue()] = rule;
    }
    if (current_ == prev) {
      // 파싱 실패 시 한 글자라도 넘기기 (무한루프 방지)
      current_++;
    }
    ruleCount++;
    if (ruleCount > 1000) { // 무한루프 방지 임시 코드
      std::cerr << "Too many rules, possible infinite loop!" << std::endl;
      break;
    }
  }

  return nullptr; // Return root node if needed
}

ASTNodePtr Parser::parseRule() {
  auto identifier = parseIdentifier();
  if (!identifier)
    return nullptr;

  skipWhitespace();
  if (!match('='))
    return nullptr;

  skipWhitespace();
  auto expression = parseExpression();
  if (!expression)
    return nullptr;

  skipWhitespace();
  if (!match(';'))
    return nullptr;

  auto rule = std::make_shared<ASTNode>(NodeType::Rule, identifier->getValue());
  rule->addChild(expression);
  return rule;
}

ASTNodePtr Parser::parseExpression() {
  auto term = parseTerm();
  if (!term)
    return nullptr;

  if (match('|')) {
    auto alternative = std::make_shared<ASTNode>(NodeType::Alternative);
    alternative->addChild(term);

    do {
      skipWhitespace();
      auto nextTerm = parseTerm();
      if (!nextTerm)
        break;
      alternative->addChild(nextTerm);
    } while (match('|'));

    return alternative;
  }

  return term;
}

ASTNodePtr Parser::parseTerm() {
  auto factor = parseFactor();
  if (!factor)
    return nullptr;

  auto sequence = std::make_shared<ASTNode>(NodeType::Sequence);
  sequence->addChild(factor);

  while (true) {
    skipWhitespace();
    auto nextFactor = parseFactor();
    if (!nextFactor)
      break;
    sequence->addChild(nextFactor);
  }

  return sequence->getChildren().size() == 1 ? sequence->getChildren()[0]
                                             : sequence;
}

ASTNodePtr Parser::parseFactor() {
  auto primary = parsePrimary();
  if (!primary)
    return nullptr;

  if (match('?')) {
    auto optional = std::make_shared<ASTNode>(NodeType::Optional);
    optional->addChild(primary);
    return optional;
  }

  if (match('*')) {
    auto repetition = std::make_shared<ASTNode>(NodeType::Repetition);
    repetition->addChild(primary);
    return repetition;
  }

  if (match('+')) {
    auto repetition = std::make_shared<ASTNode>(NodeType::Repetition);
    repetition->addChild(primary);
    return repetition;
  }

  return primary;
}

ASTNodePtr Parser::parsePrimary() {
  if (match('(')) {
    auto group = std::make_shared<ASTNode>(NodeType::Group);
    auto expression = parseExpression();
    if (!expression)
      return nullptr;

    skipWhitespace();
    if (!match(')'))
      return nullptr;

    group->addChild(expression);
    return group;
  }

  auto identifier = parseIdentifier();
  if (identifier)
    return identifier;

  auto string = parseString();
  if (string)
    return string;

  return nullptr;
}

ASTNodePtr Parser::parseIdentifier() {
  if (!std::isalpha(peek()) && peek() != '_')
    return nullptr;

  std::string identifier;
  while (std::isalnum(peek()) || peek() == '_') {
    identifier += advance();
  }

  return std::make_shared<ASTNode>(NodeType::NonTerminal, identifier);
}

ASTNodePtr Parser::parseString() {
  if (!match('"'))
    return nullptr;

  std::string str;
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\\') {
      advance();
      if (isAtEnd())
        return nullptr;
      str += advance();
    } else {
      str += advance();
    }
  }

  if (!match('"'))
    return nullptr;
  return std::make_shared<ASTNode>(NodeType::Terminal, str);
}

void Parser::skipWhitespace() {
  while (std::isspace(peek())) {
    advance();
  }
}

bool Parser::match(char c) {
  if (peek() == c) {
    advance();
    return true;
  }
  return false;
}

bool Parser::match(const std::string &str) {
  for (size_t i = 0; i < str.length(); ++i) {
    if (current_ + i >= input_.length() || input_[current_ + i] != str[i]) {
      return false;
    }
  }
  current_ += str.length();
  return true;
}

char Parser::peek() const {
  if (isAtEnd())
    return '\0';
  return input_[current_];
}

char Parser::advance() {
  if (isAtEnd())
    return '\0';
  return input_[current_++];
}

bool Parser::isAtEnd() const { return current_ >= input_.length(); }

} // namespace ebnf