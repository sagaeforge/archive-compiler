#pragma once

#include "02_parsing/ast/AST.h"
#include "04_generation/opcode/Register.h"

#include <functional>
#include <optional>
#include <unordered_map>

namespace nugdev::compiler::generation {

class Opcode;
class OpcodeGenerateStrategy;

class OpcodeGenerator {
  public:
    OpcodeGenerator();

  public:
    std::vector<Opcode> generate(const std::shared_ptr<ast::ASTNode> &node);

    RegisterTag generate_tag(std::optional<RegisterTag> tag = std::nullopt);
    Register &get_register(RegisterTag tag);

  private:
    std::unordered_map<std::function<bool(const ast::ASTNode &)>, std::shared_ptr<OpcodeGenerateStrategy>> m_strategies;
    std::unordered_map<RegisterTag, Register> m_registers;
};

} // namespace nugdev::compiler::generation
