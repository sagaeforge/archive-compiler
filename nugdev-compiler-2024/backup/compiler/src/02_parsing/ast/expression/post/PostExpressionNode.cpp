#include "PostExpressionNode.h"

namespace nugdev::compiler::ast::expression {

PostExpressionNode::PostExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const icu::UnicodeString &op)
    : m_token(token), m_left(left), m_op(op) {}

const tokenize::Token &PostExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> PostExpressionNode::get_left() const { return m_left; }

icu::UnicodeString PostExpressionNode::get_operator() const { return m_op; }

} // namespace nugdev::compiler::ast::expression
