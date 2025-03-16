#include "PrefixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

PrefixExpressionNode::PrefixExpressionNode(const tokenize::Token &token, const icu::UnicodeString &opCode, std::shared_ptr<Expression> right)
    : m_token(token), m_operator(opCode), m_right(right) {}

const tokenize::Token &PrefixExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> PrefixExpressionNode::get_right() const { return m_right; }

icu::UnicodeString PrefixExpressionNode::get_operator() const { return m_operator; }

} // namespace nugdev::compiler::ast::expression
