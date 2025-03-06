#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class PrefixExpressionNode : public Expression {
  public:
    PrefixExpressionNode(const tokenize::Token &token, const icu::UnicodeString &opCode, std::shared_ptr<Expression> right);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

    std::shared_ptr<Expression> get_right() const;
    icu::UnicodeString get_operator() const;

  private:
    tokenize::Token m_token;
    icu::UnicodeString m_operator;
    std::shared_ptr<Expression> m_right;
};
} // namespace nugdev::compiler::ast::expression