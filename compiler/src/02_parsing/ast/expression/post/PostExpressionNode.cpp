#include "PostExpressionNode.h"

namespace nugdev::compiler::ast::expression {

PostExpressionNode::PostExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const icu::UnicodeString &op)
    : m_token(token), m_left(left), m_op(op) {}

icu::UnicodeString PostExpressionNode::to_str() const { return icu::UnicodeString(u""); }

json::JsonValue PostExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("PostfixExpression"), allocator);
    value.AddMember("left", m_left->to_json(allocator), allocator);
    std::string operator_str;
    m_op.toUTF8String(operator_str);
    json::JsonValue operator_json(json::Type::kStringType);
    operator_json.SetString(operator_str.c_str(), allocator);
    value.AddMember("operator", operator_json, allocator);
    return value;
}

const tokenize::Token &PostExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> PostExpressionNode::get_left() const { return m_left; }

icu::UnicodeString PostExpressionNode::get_operator() const { return m_op; }

} // namespace nugdev::compiler::ast::expression
