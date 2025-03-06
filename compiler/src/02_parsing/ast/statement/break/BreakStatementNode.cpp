#include "BreakStatementNode.h"

#include "00_app/json/Json.hpp"

namespace nugdev::compiler::ast::statement {

BreakStatementNode::BreakStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label) : m_token(token), m_label(label) {}

json::JsonValue BreakStatementNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Break"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString BreakStatementNode::to_str() const { return u"Break"; }

const tokenize::Token &BreakStatementNode::get_token() const { return m_token; }

const std::shared_ptr<Expression> &BreakStatementNode::get_label() const { return m_label; }

BreakStatementNode::self_t BreakStatementNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return *this;
}

} // namespace nugdev::compiler::ast::statement
