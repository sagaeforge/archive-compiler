#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class NumberLiteral : public Expression, public ASTNodeDebugAspect {
  public:
    NumberLiteral(const icu::UnicodeString &value);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    icu::UnicodeString value;
};
} // namespace nugdev::compiler::ast::expression