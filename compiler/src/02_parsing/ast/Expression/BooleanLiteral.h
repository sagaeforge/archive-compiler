#pragma once

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

namespace nugdev::compiler::ast::expression {

class BooleanLiteral : public Expression, public ASTNodeDebugAspect {
  public:
    BooleanLiteral(bool value);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    bool value;
};

} // namespace nugdev::compiler::ast::expression