#pragma once

#include <memory>
#include <vector>

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

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