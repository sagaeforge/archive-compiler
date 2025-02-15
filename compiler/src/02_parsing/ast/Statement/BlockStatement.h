#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::statement {

class BlockStatement : public Statement, public ASTNodeDebugAspect {
  public:
    BlockStatement(const std::vector<std::shared_ptr<Statement>> &statements);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  private:
    std::vector<std::shared_ptr<Statement>> statements;
};
} // namespace nugdev::compiler::ast::statement