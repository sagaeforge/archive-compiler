#include "ExpressionStatementNode.h"

namespace nugdev::compiler::ast::statement {

ExpressionStatementNode::ExpressionStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> expression)
    : m_token(token), m_expression(expression) {}

json::JsonValue ExpressionStatementNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue(json::Type::kStringType).SetString("ExpressionStatement"), allocator);
    value.AddMember("expression", m_expression->to_json(allocator), allocator);
    return value;
}

icu::UnicodeString ExpressionStatementNode::to_str() const { return u"ExpressionStatement"; }

const tokenize::Token &ExpressionStatementNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::statement
