#pragma once

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

namespace nugdev::compiler::ast::expression {

class BlockStatement;

class ForExpression : public Expression, public ASTNodeDebugAspect {
  public:
    ForExpression(const std::shared_ptr<Expression> &define, const std::shared_ptr<Expression> &condition, const std::shared_ptr<Expression> &increment,
                  const std::shared_ptr<BlockStatement> &body);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    // for (define; condition; increment)
    std::shared_ptr<Expression> define;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> increment;
    std::shared_ptr<BlockStatement> body;
};
} // namespace nugdev::compiler::ast::expression