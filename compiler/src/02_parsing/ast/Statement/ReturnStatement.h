#pragma once

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

namespace nugdev::compiler::ast::statement {

class ReturnStatement : public Statement, public ASTNodeDebugAspect {
  public:
    ReturnStatement(const std::shared_ptr<Expression> &expression);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::shared_ptr<Expression> expression;
};

} // namespace nugdev::compiler::ast::statement
