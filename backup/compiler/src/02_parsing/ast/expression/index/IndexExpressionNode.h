#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class IndexExpressionNode : public Expression {
  public:
    IndexExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, std::shared_ptr<Expression> index);

  public:
    virtual const tokenize::Token &get_token() const override;

    std::shared_ptr<Expression> get_left() const;
    std::shared_ptr<Expression> get_index() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_left;
    std::shared_ptr<Expression> m_index;
};
} // namespace nugdev::compiler::ast::expression