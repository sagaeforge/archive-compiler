#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::statement {

class LetStatement : public Statement, public ASTNodeDebugAspect {
  public:
    LetStatement();

  public:
    virtual rapidjson::Value create_debug_info() const override;
};

} // namespace nugdev::compiler::ast::statement
