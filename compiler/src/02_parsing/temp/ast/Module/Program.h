#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeAspect.h"

namespace nugdev::compiler::ast::module {

class Program : public Module, public ASTNodeDebugAspect {
  public:
    Program(const std::vector<std::unique_ptr<ASTNode>> &children, const std::vector<std::unique_ptr<ASTNode>> &attributes);

  public:
    virtual rapidjson::Value create_debug_info() const override;

  public:
    std::vector<std::shared_ptr<Statement>> &get_statements();

  private:
    std::vector<std::shared_ptr<Statement>> statements;
};

} // namespace nugdev::compiler::ast::module