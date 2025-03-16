#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class ExpressionStatementNode : public Statement {
  public:
    ExpressionStatementNode(std::shared_ptr<Expression> expression);

  public:
    virtual const tokenize::Token &get_token() const override;

  public:
    const std::shared_ptr<Expression> &get_expression() const;

  private:
    std::shared_ptr<Expression> m_expression;
};

} // namespace nugdev::compiler::ast::statement