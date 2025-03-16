#include "BooleanLiteralNode.h"

namespace nugdev::compiler::ast::expression {

BooleanLiteralNode::BooleanLiteralNode(const tokenize::Token &token, bool value) : m_token(token), m_value(value) {}

const tokenize::Token &BooleanLiteralNode::get_token() const { return m_token; }

bool BooleanLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
