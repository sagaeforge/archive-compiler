#include "IdentifierLiteralNode.h"

namespace nugdev::compiler::ast::expression {

IdentifierLiteralNode::IdentifierLiteralNode(const tokenize::Token &token, icu::UnicodeString value) : m_token(token), m_value(value) {}

icu::UnicodeString IdentifierLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue IdentifierLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("IdentifierLiteral"), allocator);

    std::string value_str;
    m_value.toUTF8String(value_str);
    json::JsonValue value_json(json::Type::kStringType);
    value_json.SetString(value_str.c_str(), allocator);
    value.AddMember("value", value_json, allocator);

    return value;
}

const tokenize::Token &IdentifierLiteralNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
