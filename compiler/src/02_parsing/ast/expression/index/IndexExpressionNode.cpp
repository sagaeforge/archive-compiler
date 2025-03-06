#include "IndexExpressionNode.h"

namespace nugdev::compiler::ast::expression {

IndexExpressionNode::IndexExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, std::shared_ptr<Expression> index)
    : m_token(token), m_left(left), m_index(index) {}

icu::UnicodeString IndexExpressionNode::to_str() const { return icu::UnicodeString(u""); }

json::JsonValue IndexExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("IndexExpression"), allocator);
    value.AddMember("left", m_left->to_json(allocator), allocator);
    value.AddMember("index", m_index->to_json(allocator), allocator);
    return value;
}

const tokenize::Token &IndexExpressionNode::get_token() const { return m_token; }

std::shared_ptr<Expression> IndexExpressionNode::get_left() const { return m_left; }

std::shared_ptr<Expression> IndexExpressionNode::get_index() const { return m_index; }

} // namespace nugdev::compiler::ast::expression
