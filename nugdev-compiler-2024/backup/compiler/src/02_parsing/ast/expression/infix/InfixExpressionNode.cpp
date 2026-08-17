#include "InfixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

InfixExpressionNode::InfixExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const lib::String &opCode,
                                         std::shared_ptr<Expression> right)
    : m_token(token), m_left(left), m_operator(opCode), m_right(right) {}

const tokenize::Token &InfixExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> InfixExpressionNode::get_left() const { return m_left; }

lib::String InfixExpressionNode::get_operator() const { return m_operator; }

std::shared_ptr<Expression> InfixExpressionNode::get_right() const { return m_right; }

} // namespace nugdev::compiler::ast::expression
