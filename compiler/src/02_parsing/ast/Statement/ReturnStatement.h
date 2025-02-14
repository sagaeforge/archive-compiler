#pragma once

#include "02_parsing/ASTAspect.h"
#include "02_parsing/ast.h"

namespace nugdev::compiler::ast::statement {

class ReturnStatement : public Statement, public ASTNodeDebugAspect {
  public:
    ReturnStatement();

  public:
    virtual rapidjson::Value create_debug_info() const override;
};

} // namespace nugdev::compiler::ast::statement
