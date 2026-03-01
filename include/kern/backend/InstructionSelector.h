#pragma once
#include "kern/backend/MachIR.h"
#include "kern/lir/LIR.h"
#include "kern/support/CompilationContext.h"
#include <vector>

namespace kern {

class InstructionSelector {
    CompilationContext& ctx_;

    // Per-function state
    std::vector<std::vector<MachInstr>> block_instrs_;  // instrs per block
    uint32_t next_vreg_ = 0;

    // VReg mapping: LIR VReg → MachIR VReg (1:1 for most, but some ops create new vregs)
    // We keep the same VReg numbering from LIR

public:
    explicit InstructionSelector(CompilationContext& ctx) : ctx_(ctx) {}

    MachModule* select(const LIRModule& lir_mod);

private:
    MachFunction* selectFunction(const LIRFunction& fn);
    void selectInstr(const LIRInstr& instr, const LIRFunction& fn);

    // Helper: emit instruction to current block
    void emit(MachInstr mi);

    // Block index currently being lowered
    uint32_t current_block_ = 0;

    // Width in bits from TypeId
    uint8_t widthOf(TypeId type) const;

    // Is this type a float?
    bool isFloat(TypeId type) const;

    // Make a label for a block
    std::string_view blockLabel(const LIRFunction& fn, uint32_t block_idx) const;

    // Allocate a fresh virtual register
    VReg freshVReg() { return next_vreg_++; }

    // Select specific instruction types
    void selectConstInt(const LIRInstr& instr);
    void selectConstFloat(const LIRInstr& instr);
    void selectConstBool(const LIRInstr& instr);
    void selectConstString(const LIRInstr& instr);
    void selectGlobalRef(const LIRInstr& instr);
    void selectBinOp(const LIRInstr& instr);
    void selectFloatBinOp(const LIRInstr& instr);
    void selectICmp(const LIRInstr& instr);
    void selectFCmp(const LIRInstr& instr);
    void selectUnaryNeg(const LIRInstr& instr);
    void selectUnaryFNeg(const LIRInstr& instr);
    void selectUnaryNot(const LIRInstr& instr);
    void selectAddrOf(const LIRInstr& instr);
    void selectLoad(const LIRInstr& instr);
    void selectStore(const LIRInstr& instr);
    void selectFieldPtr(const LIRInstr& instr);
    void selectStructAlloc(const LIRInstr& instr);
    void selectBranch(const LIRInstr& instr, const LIRFunction& fn);
    void selectCondBranch(const LIRInstr& instr, const LIRFunction& fn);
    void selectRet(const LIRInstr& instr);
    void selectCall(const LIRInstr& instr);
    void selectBlockArg(const LIRInstr& instr);

    // Division: special handling for idiv/div
    void selectDiv(const LIRInstr& instr, bool is_mod);

    // Map LIR comparison op to x86 CondCode
    static CondCode mapICmpCC(LIROp op);
    static CondCode mapFCmpCC(LIROp op);
};

} // namespace kern
