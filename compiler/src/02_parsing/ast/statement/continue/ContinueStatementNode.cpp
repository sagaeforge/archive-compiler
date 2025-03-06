#include "ContinueStatementNode.h"

namespace nugdev::compiler::ast::statement {

ContinueStatementNode::ContinueStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label) : m_token(token), m_label(label) {}

json::JsonValue ContinueStatementNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Continue"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString ContinueStatementNode::to_str() const { return u"Continue"; }

const tokenize::Token &ContinueStatementNode::get_token() const { return m_token; }

ContinueStatementNode::self_t ContinueStatementNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return *this;
}

const std::shared_ptr<Expression> &ContinueStatementNode::get_label() const { return m_label; }

} // namespace nugdev::compiler::ast::statement
