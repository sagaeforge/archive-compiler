#include "02_parsing/ast/module/program/ProgramNode.h"
#include <memory>

namespace nugdev::compiler::ast::module {

ProgramNode::ProgramNode(stream::Stream<std::shared_ptr<Statement>> statements) : m_statements(statements) {}

json::JsonValue ProgramNode::to_json(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kObjectType);
    value.AddMember("type", json::JsonValue("Program"), allocator);
    value.AddMember("statements", json::JsonValue(json::Type::kArrayType), allocator);

    for (const auto &statement : m_statements) {
        value["statements"].PushBack(statement->to_json(allocator), allocator);
    }

    return value;
}

icu::UnicodeString ProgramNode::to_str() const { return u"Program"; }
const tokenize::Token &ProgramNode::get_token() const { return (*m_statements.current())->get_token(); }

} // namespace nugdev::compiler::ast::module
