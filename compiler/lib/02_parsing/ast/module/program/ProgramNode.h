#pragma once

#include "02_parsing/ast/ASTNode.h"

namespace nugdev::compiler::ast::module {

class ProgramNode : public Module {
public:
    ProgramNode(std::vector<StatementPtr> statements);

public:
    virtual const tokenize::Token &get_token() const override;

public:
    std::vector<StatementPtr> &get_statements();

private:
    std::vector<StatementPtr> m_statements;
};

}  // namespace nugdev::compiler::ast::module