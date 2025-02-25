#include "FunctionLiteralNode.h"

namespace nugdev::compiler::ast::expression {

FunctionLiteralNode::FunctionLiteralNode(const tokenize::Token &token, std::vector<std::shared_ptr<Expression>> parameters, std::shared_ptr<Statement> body)
    : m_token(token), m_parameters(parameters), m_body(body) {}

icu::UnicodeString FunctionLiteralNode::to_str() const { return m_token.get_literal(); }

json::JsonValue FunctionLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("FunctionLiteral"), allocator);
    value.AddMember("parameters", json::JsonValue(json::Type::kArrayType), allocator);
    value.AddMember("body", m_body->to_json(allocator), allocator);

    for (const auto &parameter : m_parameters) {
        value["parameters"].PushBack(parameter->to_json(allocator), allocator);
    }

    return value;
}

const tokenize::Token &FunctionLiteralNode::get_token() const { return m_token; }

} // namespace nugdev::compiler::ast::expression
