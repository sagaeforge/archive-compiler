#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class BlockStatementNode : public Statement {
  public:
    BlockStatementNode(const tokenize::Token &token, std::vector<std::shared_ptr<Statement>> statements);

  public:
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual icu::UnicodeString to_str() const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    std::vector<std::shared_ptr<Statement>> m_statements;
};

} // namespace nugdev::compiler::ast::statement