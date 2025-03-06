#include "InfixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

InfixExpressionNode::InfixExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const icu::UnicodeString &opCode,
                                         std::shared_ptr<Expression> right)
    : m_token(token), m_left(left), m_operator(opCode), m_right(right) {}

icu::UnicodeString InfixExpressionNode::to_str() const { return icu::UnicodeString(u""); }

json::JsonValue InfixExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("InfixExpression"), allocator);
    value.AddMember("left", m_left->to_json(allocator), allocator);
    std::string operator_str;
    m_operator.toUTF8String(operator_str);
    json::JsonValue operator_json(json::Type::kStringType);
    operator_json.SetString(operator_str.c_str(), allocator);
    value.AddMember("operator", operator_json, allocator);
    value.AddMember("right", m_right->to_json(allocator), allocator);
    return value;
}

const tokenize::Token &InfixExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> InfixExpressionNode::get_left() const { return m_left; }

icu::UnicodeString InfixExpressionNode::get_operator() const { return m_operator; }

std::shared_ptr<Expression> InfixExpressionNode::get_right() const { return m_right; }

} // namespace nugdev::compiler::ast::expression
