#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class ArrayLiteral : public Expression, public ASTNodeDebugAspect {
  public:
    ArrayLiteral(const std::vector<std::shared_ptr<Expression>> &elements);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::vector<std::shared_ptr<Expression>> elements;
};

} // namespace nugdev::compiler::ast::expression