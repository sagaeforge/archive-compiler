#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <ostream>

namespace kern {

using ValueId = uint32_t;
constexpr ValueId INVALID_VALUE = UINT32_MAX;

enum class IROpcode : uint8_t {
    ConstInt,
    Add, Sub, Mul, Div,
    ICmpEq, ICmpNe, ICmpLt, ICmpLe, ICmpGt, ICmpGe,
    Neg,
    Not,
    Branch,
    CondBranch,
    Ret,
    Call,
    // Logical (short-circuit handled via branches in IR gen)
    // no separate And/Or opcodes needed
};

const char* irOpcodeName(IROpcode op);

struct IRInstr {
    IROpcode               op;
    ValueId                result = INVALID_VALUE;
    std::vector<ValueId>   operands;
    SourceLocation         loc;

    // ConstInt
    int64_t imm_value = 0;

    // Call
    std::string callee_name;

    // Branch/CondBranch
    uint32_t target_block = 0;     // Branch target, or true branch
    uint32_t false_block = 0;      // CondBranch false branch
};

struct IRBlock {
    std::string              label;
    std::vector<ValueId>     params;
    std::vector<IRInstr>     instrs;
};

struct IRFunction {
    std::string              name;
    std::vector<IRBlock>     blocks;
    std::vector<ValueId>     param_values;
    std::vector<std::string> param_names;
    std::string              return_type_name;
    uint32_t                 next_value = 0;
};

struct IRModule {
    std::vector<IRFunction>  functions;
};

void dumpIR(const IRModule& mod, std::ostream& out);

} // namespace kern
