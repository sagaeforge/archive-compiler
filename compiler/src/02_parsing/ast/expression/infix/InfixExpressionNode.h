#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class InfixExpressionNode : public Expression {
  public:
    InfixExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, icu::UnicodeString &opCode, std::shared_ptr<Expression> right);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_left;
    icu::UnicodeString m_operator;
    std::shared_ptr<Expression> m_right;
};
} // namespace nugdev::compiler::ast::expression