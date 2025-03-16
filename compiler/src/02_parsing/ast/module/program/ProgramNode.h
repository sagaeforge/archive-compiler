#pragma once

#include <vector>

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::module {

class ProgramNode : public Module {
  public:
    ProgramNode(std::vector<StatementPtr> statements);

  public:
    virtual const tokenize::Token &get_token() const override;

  public:
    std::vector<StatementPtr> &get_statements() { return m_statements; }

  private:
    std::vector<StatementPtr> m_statements;
};

} // namespace nugdev::compiler::ast::module