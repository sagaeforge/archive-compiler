#include "IfExpressionNode.h"

namespace nugdev::compiler::ast::expression {

IfExpressionNode::IfExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> consequence,
                                   std::shared_ptr<Statement> alternative)
    : m_token(token), m_condition(condition), m_consequence(consequence), m_alternative(alternative) {}

icu::UnicodeString IfExpressionNode::to_str() const { return m_token.get_literal(); }

json::JsonValue IfExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("IfExpression"), allocator);
    value.AddMember("condition", m_condition->to_json(allocator), allocator);
    value.AddMember("consequence", m_consequence->to_json(allocator), allocator);
    value.AddMember("alternative", m_alternative->to_json(allocator), allocator);
    return value;
}

const tokenize::Token &IfExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
