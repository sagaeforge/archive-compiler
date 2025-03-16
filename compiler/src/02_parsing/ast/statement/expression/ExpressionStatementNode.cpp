#include "ExpressionStatementNode.h"

namespace nugdev::compiler::ast::statement {

ExpressionStatementNode::ExpressionStatementNode(std::shared_ptr<Expression> expression) : m_expression(expression) {}

const tokenize::Token &ExpressionStatementNode::get_token() const { return m_expression->get_token(); }

const std::shared_ptr<Expression> &ExpressionStatementNode::get_expression() const { return m_expression; }

} // namespace nugdev::compiler::ast::statement
