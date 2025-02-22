#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class IndexExpression : public Expression, public ASTNodeDebugAspect {
  public:
    IndexExpression(const std::shared_ptr<Expression> &left, const std::shared_ptr<Expression> &index);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> index;
};
} // namespace nugdev::compiler::ast::expression
