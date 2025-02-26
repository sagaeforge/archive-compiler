#include "FunctionLiteralNode.h"

namespace nugdev::compiler::ast::expression {

FunctionLiteralNode::FunctionLiteralNode(
    const tokenize::Token &token,
    const std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>> &parameters,
    std::shared_ptr<Statement> body)
    : m_token(token), m_parameters(parameters), m_body(body) {}

icu::UnicodeString FunctionLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue FunctionLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("FunctionLiteral"), allocator);
    value.AddMember("parameters", json::JsonValue(json::Type::kArrayType), allocator);
    value.AddMember("body", m_body->to_json(allocator), allocator);

    for (const auto &parameter : m_parameters) {
        const auto &[name, type, defaultValue] = parameter;

        json::JsonValue parameterValue(json::Type::kObjectType);
        parameterValue.AddMember("name", name->to_json(allocator), allocator);
        parameterValue.AddMember("type", type->to_json(allocator), allocator);
        parameterValue.AddMember("defaultValue", defaultValue ? defaultValue->to_json(allocator) : json::JsonValue(json::Type::kNullType), allocator);
        value["parameters"].PushBack(parameterValue, allocator);
    }

    return value;
}

const tokenize::Token &FunctionLiteralNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
