#pragma once
#include "kern/hir/HIR.h"
#include "kern/lir/LIR.h"
#include "kern/support/CompilationContext.h"
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kern {

class LIRBuilder {
public:
    explicit LIRBuilder(CompilationContext& ctx);

    LIRModule* build(const HIRModule* hir);

private:
    // Function lowering
    LIRFunction buildFunction(const HIRFnDecl* fn);

    // Expression lowering — returns VReg holding the result
    VReg lowerExpr(const HIRExpr* expr);
    VReg lowerIntLit(const HIRIntLitExpr* expr);
    VReg lowerFloatLit(const HIRFloatLitExpr* expr);
    VReg lowerBoolLit(const HIRBoolLitExpr* expr);
    VReg lowerStringLit(const HIRStringLitExpr* expr);
    VReg lowerIdent(const HIRIdentExpr* expr);
    VReg lowerBinOp(const HIRBinOpExpr* expr);
    VReg lowerUnaryOp(const HIRUnaryOpExpr* expr);
    VReg lowerCall(const HIRCallExpr* expr);
    VReg lowerIf(const HIRIfExpr* expr);
    VReg lowerMatch(const HIRMatchExpr* expr);
    VReg lowerBlock(const HIRBlockExpr* expr);
    VReg lowerReturn(const HIRReturnExpr* expr);
    VReg lowerStructLit(const HIRStructLitExpr* expr);
    VReg lowerFieldAccess(const HIRFieldAccessExpr* expr);
    VReg lowerEnumAccess(const HIREnumAccessExpr* expr);
    VReg lowerUnionVariant(const HIRUnionVariantExpr* expr);
    VReg lowerAddrOf(const HIRAddrOfExpr* expr);
    VReg lowerDeref(const HIRDerefExpr* expr);
    VReg lowerCast(const HIRCastExpr* expr);
    VReg lowerLoop(const HIRLoopExpr* expr);
    VReg lowerBreak(const HIRBreakExpr* expr);
    VReg lowerContinue(const HIRContinueExpr* expr);
    VReg lowerArrayLit(const HIRArrayLitExpr* expr);
    VReg lowerIndexAccess(const HIRIndexAccessExpr* expr);
    VReg lowerIndexElementPtr(const HIRIndexAccessExpr* expr);
    VReg lowerInlineAsm(const HIRInlineAsmExpr* expr);
    VReg lowerFnRef(const HIRFnRefExpr* expr);
    VReg lowerCallIndirect(const HIRCallIndirectExpr* expr);

    // And/Or short-circuit (phi-slot pattern)
    VReg lowerAndOr(VReg lhs, const HIRExpr* rhs_expr, bool is_and, SourceLocation loc);

    // Statement lowering
    void lowerStmt(const HIRStmt* stmt);

    // VReg allocation
    VReg freshVReg();

    // Instruction emission
    void emit(LIRInstr instr);

    // Block management
    uint32_t newBlock(std::string_view label);
    void switchToBlock(uint32_t block_idx);
    void emitBranch(uint32_t target);
    void emitBranchWithArgs(uint32_t target, const std::vector<VReg>& args);
    void emitCondBranch(VReg cond, uint32_t true_bb, uint32_t false_bb);

    // Global data
    uint32_t addStringGlobal(const char* data, uint32_t length);
    uint32_t addFloatGlobal(double value, bool is_f32);

    // Struct/union layout helpers
    uint32_t structFieldOffset(TypeId struct_type, std::string_view field_name);
    uint32_t structSize(TypeId struct_type);
    uint32_t structAlign(TypeId struct_type);
    int64_t enumVariantValue(TypeId enum_type, std::string_view variant_name);
    uint32_t unionVariantTag(TypeId union_type, std::string_view variant_name);

    // Finalize blocks into arena-allocated arrays
    void finalizeBlocks(LIRFunction& fn);

    CompilationContext& ctx_;

    // Per-function state
    VReg next_vreg_ = 0;
    uint32_t current_block_ = 0;

    struct BlockBuild {
        std::string_view label;
        std::vector<LIRInstr> instrs;
        std::vector<TypeId> param_types;
    };
    std::vector<BlockBuild> blocks_;

    // Variable → VReg mapping (SSA: val bindings get a vreg, var bindings get a stack slot ptr)
    std::unordered_map<std::string_view, VReg> locals_;

    // var bindings store the address vreg (Load to read, Store to write)
    std::unordered_map<std::string_view, VReg> var_addrs_;

    // Global data
    std::vector<GlobalData> globals_;

    // Global variable label → GlobalData index (for LoadGlobal/StoreGlobal)
    std::unordered_map<std::string_view, uint32_t> global_label_map_;

    // Source name → NASM label (handles link_name for extern globals)
    std::unordered_map<std::string_view, std::string_view> global_nasm_label_;

    // VTable labels (need lea instead of mov in LoadGlobal)
    std::unordered_set<std::string_view> vtable_labels_;

    // Variadic function names (for setting AL=0 in SysV ABI calls)
    std::unordered_set<std::string_view> variadic_fns_;

    // Block label counter for uniqueness
    uint32_t label_counter_ = 0;

    // Loop context (for break/continue lowering)
    uint32_t current_loop_header_ = 0;
    uint32_t current_loop_exit_ = 0;
    VReg current_loop_result_ = INVALID_VREG;

    // Labeled loop targets (for labeled break/continue)
    struct LoopTarget {
        uint32_t header_bb;
        uint32_t exit_bb;
    };
    std::unordered_map<std::string_view, LoopTarget> labeled_loops_;
};

} // namespace kern
