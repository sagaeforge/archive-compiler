#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class PrefixExpression : public Expression, public ASTNodeDebugAspect {
  public:
    PrefixExpression(const icu::UnicodeString &op_code, const std::shared_ptr<Expression> &right);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    icu::UnicodeString op_code;
    std::shared_ptr<Expression> right;
};

} // namespace nugdev::compiler::ast::expression