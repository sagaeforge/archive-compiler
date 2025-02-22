#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::expression {

class IdentifierLiteral : public Expression, public ASTNodeDebugAspect {
  public:
    IdentifierLiteral(const std::string &name);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::string name;
};

} // namespace nugdev::compiler::ast::expression