#include "LetStatementNode.h"

namespace nugdev::compiler::ast::statement {

LetStatementNode::LetStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> name, std::shared_ptr<Expression> type,
                                   std::shared_ptr<Expression> value)
    : m_token(token), m_name(name), m_type(type), m_value(value) {}

json::JsonValue LetStatementNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue root = json::JsonValue(json::Type::kObjectType);
    root.AddMember("type", json::JsonValue("Let"), allocator);
    root.AddMember("name", m_name->to_json(allocator), allocator);
    root.AddMember("type_expression", m_type != nullptr ? m_type->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    root.AddMember("value", m_value != nullptr ? m_value->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return root;
}

icu::UnicodeString LetStatementNode::to_str() const { return u""; }

const tokenize::Token &LetStatementNode::get_token() const { return m_token; }

LetStatementNode::self_t LetStatementNode::set_type(std::shared_ptr<Expression> type) {
    m_type = type;
    return *this;
}

LetStatementNode::self_t LetStatementNode::set_value(std::shared_ptr<Expression> value) {
    m_value = value;
    return *this;
}

const std::shared_ptr<Expression> &LetStatementNode::get_name() const { return m_name; }

const std::shared_ptr<Expression> &LetStatementNode::get_type() const { return m_type; }

const std::shared_ptr<Expression> &LetStatementNode::get_value() const { return m_value; }

} // namespace nugdev::compiler::ast::statement