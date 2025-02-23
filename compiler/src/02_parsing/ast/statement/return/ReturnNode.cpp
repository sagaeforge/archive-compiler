#include "ReturnNode.h"

namespace nugdev::compiler::ast::statement {

ReturnNode::ReturnNode(tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> return_expression)
    : m_token(token), m_label(label), m_returnExpression(return_expression) {}

json::JsonValue ReturnNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue(json::Type::kStringType).SetString("return"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("return_expression", m_returnExpression != nullptr ? m_returnExpression->to_json(allocator) : json::JsonValue(json::Type::kNullType),
                    allocator);
    return value;
}

icu::UnicodeString ReturnNode::to_str() const { return u"Return"; }
const tokenize::Token &ReturnNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::statement
