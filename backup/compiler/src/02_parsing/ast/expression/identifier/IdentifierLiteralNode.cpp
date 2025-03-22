#include "IdentifierLiteralNode.h"

namespace nugdev::compiler::ast::expression {

IdentifierLiteralNode::IdentifierLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

const tokenize::Token &IdentifierLiteralNode::get_token() const { return m_token; }

icu::UnicodeString IdentifierLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
