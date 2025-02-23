#pragma once

#include <memory>

#include "00_app/stream/Stream.hpp"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::module {

class ProgramNode : public Module {
  public:
    ProgramNode(stream::Stream<std::shared_ptr<Statement>> statements);

  public:
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual icu::UnicodeString to_str() const override;
    virtual const tokenize::Token &get_token() const override;

  public:
    stream::Stream<std::shared_ptr<Statement>> &get_statements() { return m_statements; }

  private:
    stream::Stream<std::shared_ptr<Statement>> m_statements;
};

} // namespace nugdev::compiler::ast::module