#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class CallExpression : public Expression, public ASTNodeDebugAspect {
  public:
    CallExpression(const std::shared_ptr<Expression> &callee, const std::vector<std::shared_ptr<Expression>> &arguments);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::shared_ptr<Expression> function;
    std::vector<std::shared_ptr<Expression>> arguments;
};

} // namespace nugdev::compiler::ast::expression