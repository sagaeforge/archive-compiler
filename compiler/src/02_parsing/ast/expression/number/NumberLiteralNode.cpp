#include "NumberLiteralNode.h"

namespace nugdev::compiler::ast::expression {

NumberLiteralNode::NumberLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

icu::UnicodeString NumberLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue NumberLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("NumberLiteral"), allocator);
    std::string value_str;
    m_value.toUTF8String(value_str);
    json::JsonValue value_json(json::Type::kStringType);
    value_json.SetString(value_str.c_str(), allocator);
    value.AddMember("value", value_json, allocator);
    return value;
}

const tokenize::Token &NumberLiteralNode::get_token() const { return m_token; }

icu::UnicodeString NumberLiteralNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::expression
