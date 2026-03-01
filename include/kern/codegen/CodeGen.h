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

    // Register allocation
    std::string valReg(ValueId v);
    std::string allocReg(ValueId v);
    void freeReg(ValueId v);

    // Stack management
    void emitPrologue(const std::string& name);

    std::ostream& out_;
    std::ostream* out_ref_ = nullptr; // redirect for 2-pass emit
    std::ostream& out() { return out_ref_ ? *out_ref_ : out_; }

    struct Location {
        enum Kind { Reg, Stack, XmmReg };
        Kind kind;
        std::string reg;
        int32_t stack_offset = 0;
        IRType type = IRType::Unknown;
    };

    std::unordered_map<ValueId, Location> value_locs_;
    int32_t stack_offset_ = 0;
    int32_t max_stack_ = 0;

    std::vector<std::string> free_regs_;
    std::vector<std::string> used_callee_saved_;

    // XMM register pool (xmm8..xmm15 for scratch, xmm0-7 for ABI)
    std::vector<std::string> free_xmm_regs_;

    // Float constant pool
    struct FloatConst {
        std::string label;
        double value;
        bool is_f32;
    };
    std::vector<FloatConst> float_consts_;
    uint32_t float_const_counter_ = 0;
    std::string addFloatConst(double value, bool is_f32);

    // XMM register allocation
    std::string allocXmmReg(ValueId v);
    std::string valXmmReg(ValueId v);
    void freeXmmReg(ValueId v);

    void initRegs();
    std::string spillToStack(ValueId v, IRType type);
    void ensureInReg(ValueId v, const std::string& target_reg);

    // Type-aware register names
    static std::string regForWidth(const std::string& reg64, int bits);
    // Track value types
    std::unordered_map<ValueId, IRType> value_types_;
    void setValueType(ValueId v, IRType t);
    IRType getValueType(ValueId v) const;

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

    uint32_t current_block_idx_ = 0;

    // Tail call deferred epilogue
    struct TailCallSite {
        std::string label;      // ._tail_0, ._tail_1, ...
        std::string callee;     // jump target function name
    };
    std::vector<TailCallSite> tail_call_sites_;
    uint32_t tail_call_counter_ = 0;
};

} // namespace kern
