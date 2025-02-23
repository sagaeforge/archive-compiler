#include "CallExpressionNode.h"

namespace nugdev::compiler::ast::expression {

CallExpressionNode::CallExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> arguments)
    : m_token(token), m_callee(callee), m_arguments(arguments) {}

icu::UnicodeString CallExpressionNode::to_str() const { return m_token.get_literal(); }

json::JsonValue CallExpressionNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &CallExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
