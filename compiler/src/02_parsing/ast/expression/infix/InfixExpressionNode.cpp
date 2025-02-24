#include "InfixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

InfixExpressionNode::InfixExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const icu::UnicodeString &opCode,
                                         std::shared_ptr<Expression> right)
    : m_token(token), m_left(left), m_operator(opCode), m_right(right) {}

icu::UnicodeString InfixExpressionNode::to_str() const { return icu::UnicodeString(u""); }

json::JsonValue InfixExpressionNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &InfixExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
