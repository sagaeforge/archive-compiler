#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class IdentifierLiteral;
class BlockStatement;

class FunctionLiteral : public Expression, public ASTNodeDebugAspect {
  public:
    FunctionLiteral(const std::vector<std::shared_ptr<Expression>> &parameters, const std::shared_ptr<Expression> &body);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::vector<std::shared_ptr<IdentifierLiteral>> parameters;
    std::shared_ptr<BlockStatement> body;
};
} // namespace nugdev::compiler::ast::expression