#include "WhenExpressionNode.h"

namespace nugdev::compiler::ast::expression {

WhenExpressionNode::WhenExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> target,
                                       std::map<std::shared_ptr<Expression>, Consequence> conditions, Consequence alternative)
    : m_token(token), m_target(target), m_conditions(conditions), m_alternative(alternative) {}

icu::UnicodeString WhenExpressionNode::to_str() const { return u"when"; }

json::JsonValue WhenExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("When"), allocator);
    value.AddMember("target", m_target != nullptr ? m_target->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);

    json::JsonValue conditionsArray(json::Type::kArrayType);
    for (const auto &[condition, consequence] : m_conditions) {
        json::JsonValue conditionObj(json::Type::kObjectType);
        conditionObj.AddMember("condition", condition->to_json(allocator), allocator);

        if (std::holds_alternative<std::shared_ptr<Expression>>(consequence)) {
            conditionObj.AddMember("consequence", std::get<std::shared_ptr<Expression>>(consequence)->to_json(allocator), allocator);
        } else if (std::holds_alternative<std::shared_ptr<Statement>>(consequence)) {
            conditionObj.AddMember("consequence", std::get<std::shared_ptr<Statement>>(consequence)->to_json(allocator), allocator);
        }

        conditionsArray.PushBack(conditionObj, allocator);
    }

    value.AddMember("conditions", conditionsArray, allocator);

    std::shared_ptr<ast::ASTNode> alternative = nullptr;
    if (std::holds_alternative<std::shared_ptr<Expression>>(m_alternative)) {
        alternative = std::get<std::shared_ptr<Expression>>(m_alternative);
    } else if (std::holds_alternative<std::shared_ptr<Statement>>(m_alternative)) {
        alternative = std::get<std::shared_ptr<Statement>>(m_alternative);
    }

    value.AddMember("alternative", alternative != nullptr ? alternative->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);

    return value;
}

const tokenize::Token &WhenExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
