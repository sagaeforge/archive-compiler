#include "02_parsing/ast/module/program/ProgramNode.h"
#include <memory>

namespace nugdev::compiler::ast::module {

ProgramNode::ProgramNode(stream::Stream<std::shared_ptr<Statement>> statements) : statements(statements) {}

json::JsonValue ProgramNode::create_debug_info(json::JsonAllocator &allocator) const {
    json::JsonValue value(json::Type::kArrayType);
    for (const auto &statement : statements) {
        auto astDebugInfo = std::dynamic_pointer_cast<ASTNodeDebugAspect>(statement);
        if (astDebugInfo == nullptr) {
            continue;
        }

        value.PushBack(astDebugInfo->create_debug_info(allocator), allocator);
    }
    return value;
}

icu::UnicodeString ProgramNode::get_type() const { return u"Program"; }
icu::UnicodeString ProgramNode::to_str() const { return u"Program"; }
tokenize::Token &ProgramNode::get_token() const { return (*statements.current())->get_token(); }

} // namespace nugdev::compiler::ast::module
