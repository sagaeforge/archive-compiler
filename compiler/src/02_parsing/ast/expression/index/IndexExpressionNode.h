#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class IndexExpressionNode : public Expression {
  public:
    IndexExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> left, std::shared_ptr<Expression> index);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_left;
    std::shared_ptr<Expression> m_index;
};
} // namespace nugdev::compiler::ast::expression