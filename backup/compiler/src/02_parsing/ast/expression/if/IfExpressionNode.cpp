#include "IfExpressionNode.h"

namespace nugdev::compiler::ast::expression {

IfExpressionNode::IfExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> consequence,
                                   std::shared_ptr<Statement> alternative)
    : m_token(token), m_condition(condition), m_consequence(consequence), m_alternative(alternative) {}

const tokenize::Token &IfExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> IfExpressionNode::get_condition() const { return m_condition; }

std::shared_ptr<Statement> IfExpressionNode::get_consequence() const { return m_consequence; }

std::shared_ptr<Statement> IfExpressionNode::get_alternative() const { return m_alternative; }

} // namespace nugdev::compiler::ast::expression
