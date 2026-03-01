#pragma once
#include "kern/ir/KernIR.h"
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace kern {

class CodeGen {
public:
    explicit CodeGen(std::ostream& out);

    void emitModule(const IRModule& mod);

private:
    void emitFunction(const IRFunction& fn);
    void emitBlock(const IRFunction& fn, uint32_t block_idx);
    void emitInstr(const IRFunction& fn, const IRInstr& instr);

    // Register allocation (simplified for M1)
    std::string valReg(ValueId v);
    std::string allocReg(ValueId v);
    void freeReg(ValueId v);

    // Stack management for callee-saved regs and spills
    void emitPrologue(const std::string& name);
    void emitEpilogue();

    std::ostream& out_;

    // Simple value-to-location mapping
    struct Location {
        enum Kind { Reg, Stack };
        Kind kind;
        std::string reg;
        int32_t stack_offset = 0;
    };

    std::unordered_map<ValueId, Location> value_locs_;
    int32_t stack_offset_ = 0;
    int32_t max_stack_ = 0;

    // Available general-purpose registers (caller-saved first for temps)
    std::vector<std::string> free_regs_;
    std::vector<std::string> used_callee_saved_;

    void initRegs();
    std::string spillToStack(ValueId v);
    void ensureInReg(ValueId v, const std::string& target_reg);

    // Merge block handling
    struct MergeInfo {
        ValueId result_val;
        ValueId then_val;
        ValueId else_val;
        uint32_t then_block;
        uint32_t else_block;
    };
    std::unordered_map<uint32_t, MergeInfo> merge_blocks_;
    void collectMergeBlocks(const IRFunction& fn);

    // Track current block for merge resolution
    uint32_t current_block_idx_ = 0;
};

} // namespace kern
