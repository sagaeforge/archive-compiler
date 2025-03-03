#pragma once

#include "02_parsing/ast/AST.h"

#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace nugdev::compiler::generation {

class Opcode;
class OpcodeGenerationStrategy;

class OpcodeGenerator {
  public:
    OpcodeGenerator();

  public:
    std::vector<Opcode> generate(const std::shared_ptr<ast::ASTNode> &node);

  private:
    std::unordered_map<std::function<bool(const ast::ASTNode &)>, std::shared_ptr<OpcodeGenerationStrategy>> m_strategies;
};

} // namespace nugdev::compiler::generation
