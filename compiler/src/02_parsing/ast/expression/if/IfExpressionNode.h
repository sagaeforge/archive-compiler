#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class IfExpressionNode : public Expression {
  public:
    IfExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> condition, std::shared_ptr<Expression> consequence,
                     std::shared_ptr<Expression> alternative);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Expression> m_consequence;
    std::shared_ptr<Expression> m_alternative;
};

} // namespace nugdev::compiler::ast::expression
