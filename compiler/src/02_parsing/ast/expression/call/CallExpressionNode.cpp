#include "CallExpressionNode.h"

namespace nugdev::compiler::ast::expression {

CallExpressionNode::CallExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> arguments)
    : m_token(token), m_callee(callee), m_arguments(arguments) {}

const tokenize::Token &CallExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> CallExpressionNode::get_callee() const { return m_callee; }

const std::vector<std::shared_ptr<Expression>> &CallExpressionNode::get_arguments() const { return m_arguments; }

} // namespace nugdev::compiler::ast::expression
