#include "NumberLiteralNode.h"

namespace nugdev::compiler::ast::expression {

NumberLiteralNode::NumberLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

const tokenize::Token &NumberLiteralNode::get_token() const { return m_token; }

icu::UnicodeString NumberLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
