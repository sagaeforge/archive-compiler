#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class InfixExpressionNode : public Expression {
  public:
    InfixExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const icu::UnicodeString &opCode, std::shared_ptr<Expression> right);

  public:
    virtual const tokenize::Token &get_token() const override;

  public:
    std::shared_ptr<Expression> get_left() const;
    icu::UnicodeString get_operator() const;
    std::shared_ptr<Expression> get_right() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_left;
    icu::UnicodeString m_operator;
    std::shared_ptr<Expression> m_right;
};
} // namespace nugdev::compiler::ast::expression