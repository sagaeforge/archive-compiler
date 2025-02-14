#pragma once

#include "02_parsing/ASTAspect.h"
#include "02_parsing/ast.h"

namespace nugdev::compiler::ast::statement {

class ExpressionStatement : public Statement, public ASTNodeDebugAspect {
  public:
    ExpressionStatement();

  public:
    virtual rapidjson::Value create_debug_info() const override;
};

} // namespace nugdev::compiler::ast::statement
