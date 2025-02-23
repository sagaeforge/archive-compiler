#include "IdentifierLiteralNode.h"

namespace nugdev::compiler::ast::expression {

IdentifierLiteralNode::IdentifierLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

icu::UnicodeString IdentifierLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue IdentifierLiteralNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &IdentifierLiteralNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
