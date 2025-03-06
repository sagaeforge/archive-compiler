#include "ReturnStatementNode.h"

namespace nugdev::compiler::ast::statement {

ReturnStatementNode::ReturnStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> value)
    : m_token(token), m_label(label), m_value(value) {}

json::JsonValue ReturnStatementNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Return"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("value", m_value != nullptr ? m_value->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString ReturnStatementNode::to_str() const { return u"Return"; }

const tokenize::Token &ReturnStatementNode::get_token() const { return m_token; }

ReturnStatementNode::self_t ReturnStatementNode::set_label(std::shared_ptr<Expression> label) {
    m_label = label;
    return this;
}

ReturnStatementNode::self_t ReturnStatementNode::set_value(std::shared_ptr<Expression> return_expression) {
    m_value = return_expression;
    return this;
}

const std::shared_ptr<Expression> &ReturnStatementNode::get_label() const { return m_label; }

const std::shared_ptr<Expression> &ReturnStatementNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::statement
