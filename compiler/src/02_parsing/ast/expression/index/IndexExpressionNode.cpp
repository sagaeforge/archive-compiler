#include "IndexExpressionNode.h"

namespace nugdev::compiler::ast::expression {

IndexExpressionNode::IndexExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, std::shared_ptr<Expression> index)
    : m_token(token), m_left(left), m_index(index) {}

icu::UnicodeString IndexExpressionNode::to_str() const { return icu::UnicodeString(u""); }

json::JsonValue IndexExpressionNode::to_json(json::JsonAllocator &allocator) const { return json::JsonValue(""); }

const tokenize::Token &IndexExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
