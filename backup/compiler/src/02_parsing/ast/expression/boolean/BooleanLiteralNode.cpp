#include "BooleanLiteralNode.h"
#include "02_parsing/TypeMeta.h"

namespace nugdev::compiler::ast::expression {

BooleanLiteralNode::BooleanLiteralNode(const tokenize::Token &token, bool value) : m_token(token), m_value(value) {}

const tokenize::Token &BooleanLiteralNode::get_token() const { return m_token; }

bool BooleanLiteralNode::get_value() const { return m_value; }

TypeInfo BooleanLiteralNode::get_type_info() const { return boolean; }

} // namespace nugdev::compiler::ast::expression
