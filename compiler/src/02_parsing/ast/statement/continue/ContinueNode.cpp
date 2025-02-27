#include "ContinueNode.h"

namespace nugdev::compiler::ast::statement {

ContinueNode::ContinueNode(const tokenize::Token &token, std::shared_ptr<Expression> label) : m_token(token), m_label(label) {}

json::JsonValue ContinueNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Continue"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString ContinueNode::to_str() const { return u"Continue"; }

const tokenize::Token &ContinueNode::get_token() const { return m_token; }

ContinueNode::self_t ContinueNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return *this;
}

} // namespace nugdev::compiler::ast::statement
