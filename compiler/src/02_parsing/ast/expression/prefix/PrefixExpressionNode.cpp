#include "PrefixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

PrefixExpressionNode::PrefixExpressionNode(const tokenize::Token &token, icu::UnicodeString &opCode, std::shared_ptr<Expression> right)
    : m_token(token), m_operator(opCode), m_right(right) {}

icu::UnicodeString PrefixExpressionNode::to_str() const { return m_token.get_literal(); }

json::JsonValue PrefixExpressionNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &PrefixExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
