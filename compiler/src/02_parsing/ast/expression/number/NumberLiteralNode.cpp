#include "NumberLiteralNode.h"

namespace nugdev::compiler::ast::expression {

NumberLiteralNode::NumberLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

icu::UnicodeString NumberLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue NumberLiteralNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &NumberLiteralNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
