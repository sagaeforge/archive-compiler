#include "ArrayLiteralNode.h"

namespace nugdev::compiler::ast::expression {

ArrayLiteralNode::ArrayLiteralNode(const tokenize::Token &token, const std::vector<std::shared_ptr<Expression>> &elements)
    : m_token(token), m_elements(elements) {}

icu::UnicodeString ArrayLiteralNode::to_str() const { return icu::UnicodeString(u"["); }

json::JsonValue ArrayLiteralNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue json(json::Type::kObjectType);
    json.AddMember("type", json::JsonValue("ArrayLiteral"), allocator);
    json.AddMember("elements", json::JsonValue(json::Type::kArrayType), allocator);

    for (const auto &element : m_elements) {
        json["elements"].PushBack(element->to_json(allocator), allocator);
    }

    return json;
}

const tokenize::Token &ArrayLiteralNode::get_token() const { return m_token; }

const std::vector<std::shared_ptr<Expression>> &ArrayLiteralNode::get_elements() const { return m_elements; }

} // namespace nugdev::compiler::ast::expression
