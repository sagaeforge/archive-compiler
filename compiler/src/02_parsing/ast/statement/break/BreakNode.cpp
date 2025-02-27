#include "BreakNode.h"
#include "00_app/json/Json.hpp"

namespace nugdev::compiler::ast::statement {

BreakNode::BreakNode(const tokenize::Token &token, std::shared_ptr<Expression> label) : m_token(token), m_label(label) {}

json::JsonValue BreakNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Break"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString BreakNode::to_str() const { return u"Break"; }

const tokenize::Token &BreakNode::get_token() const { return m_token; }

BreakNode::self_t BreakNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return *this;
}

} // namespace nugdev::compiler::ast::statement
