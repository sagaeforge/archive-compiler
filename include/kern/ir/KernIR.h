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
    ConstFloat,
    Add, Sub, Mul, Div,
    FAdd, FSub, FMul, FDiv,
    ICmpEq, ICmpNe, ICmpLt, ICmpLe, ICmpGt, ICmpGe,
    FCmpEq, FCmpNe, FCmpLt, FCmpLe, FCmpGt, FCmpGe,
    Neg,
    FNeg,
    Not,
    Branch,
    CondBranch,
    Ret,
    Call,
    StructAlloc,
    FieldStore,
    FieldLoad,
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

    // ConstFloat
    double imm_float = 0.0;

    // Call
    std::string callee_name;
    bool is_tail_call = false;

    // Branch/CondBranch
    uint32_t target_block = 0;
    uint32_t false_block = 0;
};

struct IRBlock {
    std::string              label;
    std::vector<ValueId>     params;
    std::vector<IRInstr>     instrs;
    bool                     is_merge = false;
};

struct IRFunction {
    std::string              name;
    std::vector<IRBlock>     blocks;
    std::vector<ValueId>     param_values;
    std::vector<std::string> param_names;
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
