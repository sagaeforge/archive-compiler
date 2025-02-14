#pragma once

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

namespace nugdev::compiler::ast::statement {

class BreakStatement : public Statement, public ASTNodeDebugAspect {
  public:
    BreakStatement();

  public:
    virtual rapidjson::Value create_debug_info() const override;
};

} // namespace nugdev::compiler::ast::statement
