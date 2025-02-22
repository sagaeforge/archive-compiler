#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::statement {

class ExpressionStatement : public Statement, public ASTNodeDebugAspect {
  public:
    ExpressionStatement(const std::shared_ptr<Expression> &expression);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::shared_ptr<Expression> expression;
};

} // namespace nugdev::compiler::ast::statement
