#include "CallExpressionNode.h"

namespace nugdev::compiler::ast::expression {

CallExpressionNode::CallExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> arguments)
    : m_token(token), m_callee(callee), m_arguments(arguments) {}

icu::UnicodeString CallExpressionNode::to_str() const { return m_token.get_literal(); }

json::JsonValue CallExpressionNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("CallExpression"), allocator);
    value.AddMember("callee", m_callee->to_json(allocator), allocator);
    value.AddMember("arguments", json::JsonValue(json::Type::kArrayType), allocator);

    for (const auto &argument : m_arguments) {
        value["arguments"].PushBack(argument->to_json(allocator), allocator);
    }

    return value;
}

const tokenize::Token &CallExpressionNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
