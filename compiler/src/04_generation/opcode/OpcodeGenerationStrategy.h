#pragma once

#include "02_parsing/ast/AST.h"

#include <vector>

namespace nugdev::compiler::generation {

class Opcode;

class OpcodeGenerationStrategy {
  public:
    virtual ~OpcodeGenerationStrategy() = default;

  public:
    virtual std::vector<Opcode> generate(const std::shared_ptr<ast::ASTNode> &node) = 0;
};

} // namespace nugdev::compiler::generation
