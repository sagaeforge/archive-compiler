#include "IndexExpressionNode.h"

namespace nugdev::compiler::ast::expression {

IndexExpressionNode::IndexExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, std::shared_ptr<Expression> index)
    : m_token(token), m_left(left), m_index(index) {}

const tokenize::Token &IndexExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> IndexExpressionNode::get_left() const { return m_left; }

std::shared_ptr<Expression> IndexExpressionNode::get_index() const { return m_index; }

} // namespace nugdev::compiler::ast::expression
