#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class StringLiteral : public Expression, public ASTNodeDebugAspect {
  public:
    StringLiteral(const icu::UnicodeString &value);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    icu::UnicodeString value;
};
} // namespace nugdev::compiler::ast::expression