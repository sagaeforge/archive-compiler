#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class BlockStatement;

class IfExpression : public Expression, public ASTNodeDebugAspect {
  public:
    IfExpression(const std::shared_ptr<Expression> &condition, const std::shared_ptr<Expression> &consequence, const std::shared_ptr<Expression> &alternative);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::shared_ptr<Expression> condition;
    std::shared_ptr<BlockStatement> consequence;
    std::shared_ptr<IfExpression> elseIfBranch;
    std::shared_ptr<BlockStatement> alternative;
};
} // namespace nugdev::compiler::ast::expression