#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class BreakStatementNode : public Statement {
  public:
    using self_t = BreakStatementNode;

  public:
    BreakStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label);

  public:
    virtual const tokenize::Token &get_token() const override;

  public:
    self_t set_label(std::shared_ptr<Expression> label);
    const std::shared_ptr<Expression> &get_label() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_label;
};

} // namespace nugdev::compiler::ast::statement
