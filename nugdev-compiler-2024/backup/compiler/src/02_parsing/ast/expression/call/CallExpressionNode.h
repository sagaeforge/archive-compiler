#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class FunctionExpressionNode;

class CallExpressionNode : public Expression {
  public:
    CallExpressionNode(const tokenize::Token &token, std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> arguments);

  public:
    virtual const tokenize::Token &get_token() const override;
    virtual TypeInfo get_type_info() const override;

  public:
    std::shared_ptr<Expression> get_callee() const;
    std::shared_ptr<FunctionExpressionNode> get_target() const;
    void set_target(std::shared_ptr<FunctionExpressionNode> target);
    const std::vector<std::shared_ptr<Expression>> &get_arguments() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_callee;
    std::shared_ptr<FunctionExpressionNode> m_target;
    std::vector<std::shared_ptr<Expression>> m_arguments;
};
} // namespace nugdev::compiler::ast::expression