#include "PrefixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

PrefixExpressionNode::PrefixExpressionNode(const tokenize::Token &token, const icu::UnicodeString &opCode, std::shared_ptr<Expression> right)
    : m_token(token), m_operator(opCode), m_right(right) {}

icu::UnicodeString PrefixExpressionNode::to_str() const { return m_token.get_literal(); }

json::JsonValue PrefixExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("PrefixExpression"), allocator);
    std::string operator_str;
    m_operator.toUTF8String(operator_str);
    json::JsonValue operator_json(json::Type::kStringType);
    operator_json.SetString(operator_str.c_str(), allocator);
    value.AddMember("operator", operator_json, allocator);
    value.AddMember("right", m_right->to_json(allocator), allocator);
    return value;
}

const tokenize::Token &PrefixExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> PrefixExpressionNode::get_right() const { return m_right; }

icu::UnicodeString PrefixExpressionNode::get_operator() const { return m_operator; }

} // namespace nugdev::compiler::ast::expression
