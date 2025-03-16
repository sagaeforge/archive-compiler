#include "StringLiteralNode.h"

namespace nugdev::compiler::ast::expression {

StringLiteralNode::StringLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

const tokenize::Token &StringLiteralNode::get_token() const { return m_token; }

icu::UnicodeString StringLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
