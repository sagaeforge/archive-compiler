#include "WhenExpressionNode.h"

namespace nugdev::compiler::ast::expression {

WhenExpressionNode::WhenExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> target,
                                       std::map<std::shared_ptr<Expression>, Consequence> conditions, Consequence alternative)
    : m_token(token), m_target(target), m_conditions(conditions), m_alternative(alternative) {}

const tokenize::Token &WhenExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
