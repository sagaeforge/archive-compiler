#include "ReturnNode.h"

namespace nugdev::compiler::ast::statement {

ReturnNode::ReturnNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> value)
    : m_token(token), m_label(label), m_value(value) {}

json::JsonValue ReturnNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Return"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("value", m_value != nullptr ? m_value->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString ReturnNode::to_str() const { return u"Return"; }

const tokenize::Token &ReturnNode::get_token() const { return m_token; }

ReturnNode::self_t ReturnNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return this;
}

ReturnNode::self_t ReturnNode::set_value(std::shared_ptr<Expression> return_expression) {
    m_value = return_expression;
    return this;
}

} // namespace nugdev::compiler::ast::statement
