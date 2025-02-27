#include "ForNode.h"

namespace nugdev::compiler::ast::statement {

ForNode::ForNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> init, std::shared_ptr<Expression> condition,
                 std::shared_ptr<Expression> post, std::shared_ptr<Statement> consequence)
    : m_token(token), m_label(label), m_init(init), m_condition(condition), m_post(post), m_consequence(consequence) {}

json::JsonValue ForNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("For"), allocator);
    value.AddMember("label", m_label != nullptr ? m_label->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("init", m_init != nullptr ? m_init->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("condition", m_condition != nullptr ? m_condition->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("post", m_post != nullptr ? m_post->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    value.AddMember("consequence", m_consequence != nullptr ? m_consequence->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
    return value;
}

icu::UnicodeString ForNode::to_str() const { return u"For"; }

const tokenize::Token &ForNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::statement
