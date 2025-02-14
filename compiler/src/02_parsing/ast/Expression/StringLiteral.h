#pragma once

#include <unicode/unistr.h>

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

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