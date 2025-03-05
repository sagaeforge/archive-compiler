#include "BooleanLiteralNode.h"

namespace nugdev::compiler::ast::expression {

BooleanLiteralNode::BooleanLiteralNode(const tokenize::Token &token, bool value) : m_token(token), m_value(value) {}

icu::UnicodeString BooleanLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue BooleanLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("BooleanLiteral"), allocator);
    value.AddMember("value", json::JsonValue(m_value), allocator);
    return value;
}

const tokenize::Token &BooleanLiteralNode::get_token() const { return m_token; }

bool BooleanLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
