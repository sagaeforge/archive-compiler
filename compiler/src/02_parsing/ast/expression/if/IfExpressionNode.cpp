#include "IfExpressionNode.h"

namespace nugdev::compiler::ast::expression {

IfExpressionNode::IfExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> condition, std::shared_ptr<Expression> consequence,
                                   std::shared_ptr<Expression> alternative)
    : m_token(token), m_condition(condition), m_consequence(consequence), m_alternative(alternative) {}

icu::UnicodeString IfExpressionNode::to_str() const { return m_token.get_literal(); }

json::JsonValue IfExpressionNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &IfExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
