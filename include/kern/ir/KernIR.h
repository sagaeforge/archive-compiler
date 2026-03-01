#pragma once
#include "kern/support/SourceLocation.h"
#include "kern/ir/IRType.h"
#include "kern/ir/Metadata.h"
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
};

const char* irOpcodeName(IROpcode op);

struct IRInstr {
    IROpcode               op;
    ValueId                result = INVALID_VALUE;
    std::vector<ValueId>   operands;
    SourceLocation         loc;
    IRType                 type = IRType::Unknown;

    // ConstInt
    int64_t imm_value = 0;

    // Call
    std::string callee_name;

    // Branch/CondBranch
    uint32_t target_block = 0;
    uint32_t false_block = 0;
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

    // M2: type information
    std::vector<IRType>      param_types;
    IRType                   return_type = IRType::Unknown;
    FunctionMeta             meta;
};

struct IRModule {
    std::vector<IRFunction>  functions;
};

void dumpIR(const IRModule& mod, std::ostream& out);

} // namespace kern
