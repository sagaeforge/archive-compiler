#pragma once

#include <memory>

#include "00_app/stream/Stream.hpp"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::module {

class ProgramNode : public Module, public ASTNodeDebugAspect {
  public:
    ProgramNode(stream::Stream<std::shared_ptr<Statement>> statements);

  public:
    virtual json::JsonValue create_debug_info(json::JsonAllocator &allocator) const override;
    virtual icu::UnicodeString get_type() const override;
    virtual icu::UnicodeString to_str() const override;
    virtual tokenize::Token &get_token() const override;

  public:
    stream::Stream<std::shared_ptr<Statement>> &get_statements() { return statements; }

  private:
    stream::Stream<std::shared_ptr<Statement>> statements;
};

} // namespace nugdev::compiler::ast::module