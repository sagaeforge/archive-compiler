#pragma once

#include <memory>
#include <vector>

#include "02_parsing/AST.h"
#include "02_parsing/ASTAspect.h"

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