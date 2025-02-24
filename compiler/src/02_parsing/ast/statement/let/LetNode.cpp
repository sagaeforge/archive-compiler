#include "LetNode.h"

namespace nugdev::compiler::ast::statement {

LetNode::LetNode(const tokenize::Token &token, std::shared_ptr<Expression> name, std::shared_ptr<Expression> type, std::shared_ptr<Expression> value)
    : m_token(token), m_name(name), m_type(type), m_value(value) {}

json::JsonValue LetNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue root = json::JsonValue(json::Type::kObjectType);
    root.AddMember("name", m_name->to_json(allocator), allocator);
    root.AddMember("type", m_type->to_json(allocator), allocator);
    root.AddMember("value", m_value->to_json(allocator), allocator);
    return root;
}

icu::UnicodeString LetNode::to_str() const { return u""; }

const tokenize::Token &LetNode::get_token() const { return m_token; }

LetNode::self_t LetNode::set_type(std::shared_ptr<Expression> type) {
    m_type = type;
    return *this;
}

LetNode::self_t LetNode::set_value(std::shared_ptr<Expression> value) {
    m_value = value;
    return *this;
}

} // namespace nugdev::compiler::ast::statement