#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class BlockStatementNode : public Statement {
  public:
    BlockStatementNode(std::vector<std::shared_ptr<Statement>> statements);

  public:
    virtual const tokenize::Token &get_token() const override;

  public:
    const std::vector<std::shared_ptr<Statement>> &get_statements() const;

  private:
    std::vector<std::shared_ptr<Statement>> m_statements;
};

} // namespace nugdev::compiler::ast::statement