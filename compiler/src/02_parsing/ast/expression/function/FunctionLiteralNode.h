#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class FunctionLiteralNode : public Expression {
  public:
    FunctionLiteralNode(const tokenize::Token &token, std::vector<std::shared_ptr<Expression>> parameters, std::shared_ptr<Expression> body);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::vector<std::shared_ptr<Expression>> m_parameters;
    std::shared_ptr<Expression> m_body;
};
} // namespace nugdev::compiler::ast::expression