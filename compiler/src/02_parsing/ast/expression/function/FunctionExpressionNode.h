#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class FunctionExpressionNode : public Expression {
  public:
    FunctionExpressionNode(const tokenize::Token &token,
                           const std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>> &parameters,
                           std::shared_ptr<Statement> body);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

    const std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>> &get_parameters() const;
    std::shared_ptr<Statement> get_body() const;

  private:
    tokenize::Token m_token;
    std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>> m_parameters;
    std::shared_ptr<Statement> m_body;
};
} // namespace nugdev::compiler::ast::expression