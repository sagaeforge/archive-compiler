#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class PostExpressionNode : public Expression {
  public:
    PostExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, const icu::UnicodeString &op);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

    std::shared_ptr<Expression> get_left() const;
    icu::UnicodeString get_operator() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_left;
    icu::UnicodeString m_op;
};

} // namespace nugdev::compiler::ast::expression
