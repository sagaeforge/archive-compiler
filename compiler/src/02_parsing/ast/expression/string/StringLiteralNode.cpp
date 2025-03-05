#include "StringLiteralNode.h"

namespace nugdev::compiler::ast::expression {

StringLiteralNode::StringLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

icu::UnicodeString StringLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue StringLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("StringLiteral"), allocator);
    std::string value_str;
    m_value.toUTF8String(value_str);
    json::JsonValue value_json(json::Type::kStringType);
    value_json.SetString(value_str.c_str(), allocator);
    value.AddMember("value", value_json, allocator);
    return value;
}

const tokenize::Token &StringLiteralNode::get_token() const { return m_token; }

icu::UnicodeString StringLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
