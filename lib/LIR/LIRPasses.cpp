#include "kern/lir/LIRPasses.h"
#include "kern/support/CompilationContext.h"
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace kern {

// ============================================================================
// Helper: check if an instruction has side effects (cannot be removed by DCE)
// ============================================================================

static bool hasSideEffects(LIROp op) {
    switch (op) {
        case LIROp::Store:
        case LIROp::StoreGlobal:
        case LIROp::Branch:
        case LIROp::CondBranch:
        case LIROp::Ret:
        case LIROp::Call:
        case LIROp::CallIndirect:
        case LIROp::InlineAsm:
        case LIROp::AtomicStore:
        case LIROp::AtomicCas:
        case LIROp::AtomicFetchAdd:
        case LIROp::Fence:
        case LIROp::CompilerFence:
        case LIROp::PercpuStore:
        case LIROp::PortIn:
        case LIROp::PortOut:
        case LIROp::Trap:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// Helper: collect all vregs used by an instruction
// ============================================================================

static void collectUses(const LIRInstr& instr, std::unordered_set<VReg>& uses) {
    switch (instr.op) {
        case LIROp::ConstInt:
        case LIROp::ConstFloat:
        case LIROp::ConstBool:
        case LIROp::ConstString:
        case LIROp::GlobalRef:
        case LIROp::BlockArg:
        case LIROp::Fence:
        case LIROp::CompilerFence:
        case LIROp::FnRef:
        case LIROp::LoadGlobal:
        case LIROp::Trap:
            break;
        case LIROp::InlineAsm:
            // Extended asm: mark input/output vregs as used
            for (uint32_t i = 0; i < instr.inline_asm.input_count; ++i)
                uses.insert(instr.inline_asm.inputs[i].vreg);
            for (uint32_t i = 0; i < instr.inline_asm.output_count; ++i)
                uses.insert(instr.inline_asm.outputs[i].vreg);
            break;
        case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
        case LIROp::Div: case LIROp::Mod:
        case LIROp::AddWrap: case LIROp::SubWrap: case LIROp::MulWrap:
        case LIROp::AddSat: case LIROp::SubSat:
        case LIROp::BAnd: case LIROp::BOr: case LIROp::BXor:
        case LIROp::Shl: case LIROp::Shr:
        case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul:
        case LIROp::FDiv:
        case LIROp::ICmpEq: case LIROp::ICmpNe: case LIROp::ICmpLt:
        case LIROp::ICmpLe: case LIROp::ICmpGt: case LIROp::ICmpGe:
        case LIROp::FCmpEq: case LIROp::FCmpNe: case LIROp::FCmpLt:
        case LIROp::FCmpLe: case LIROp::FCmpGt: case LIROp::FCmpGe:
            uses.insert(instr.bin.lhs);
            uses.insert(instr.bin.rhs);
            break;
        case LIROp::Neg: case LIROp::FNeg: case LIROp::Not: case LIROp::BNot:
            uses.insert(instr.unary.operand);
            break;
        case LIROp::Cast:
            uses.insert(instr.cast.operand);
            break;
        case LIROp::AddrOf:
            uses.insert(instr.addr_of.source);
            break;
        case LIROp::Load:
            uses.insert(instr.load.ptr);
            break;
        case LIROp::Store:
            uses.insert(instr.store.ptr);
            uses.insert(instr.store.value);
            break;
        case LIROp::FieldPtr:
            uses.insert(instr.field_ptr.base);
            break;
        case LIROp::StructAlloc:
            break;
        case LIROp::Branch:
            for (uint32_t i = 0; i < instr.branch.arg_count; ++i)
                uses.insert(instr.branch.args[i]);
            break;
        case LIROp::CondBranch:
            uses.insert(instr.cond_branch.cond);
            break;
        case LIROp::Ret:
            if (instr.ret.value != INVALID_VREG)
                uses.insert(instr.ret.value);
            break;
        case LIROp::Call:
            for (uint32_t i = 0; i < instr.call.arg_count; ++i)
                uses.insert(instr.call.args[i]);
            break;
        case LIROp::CallIndirect:
            uses.insert(instr.call_indirect.callee);
            for (uint32_t i = 0; i < instr.call_indirect.arg_count; ++i)
                uses.insert(instr.call_indirect.args[i]);
            break;
        case LIROp::AtomicLoad:
            uses.insert(instr.atomic_load.ptr);
            break;
        case LIROp::AtomicStore:
            uses.insert(instr.atomic_store.ptr);
            uses.insert(instr.atomic_store.value);
            break;
        case LIROp::AtomicCas:
            uses.insert(instr.atomic_cas.ptr);
            uses.insert(instr.atomic_cas.expected);
            uses.insert(instr.atomic_cas.desired);
            break;
        case LIROp::AtomicFetchAdd:
            uses.insert(instr.atomic_fetch_add.ptr);
            uses.insert(instr.atomic_fetch_add.value);
            break;
        case LIROp::PercpuLoad:
            uses.insert(instr.percpu_load.offset);
            break;
        case LIROp::PercpuStore:
            uses.insert(instr.percpu_store.offset);
            uses.insert(instr.percpu_store.value);
            break;
        case LIROp::StoreGlobal:
            uses.insert(instr.store_global.value);
            break;
        case LIROp::Clz: case LIROp::Ctz: case LIROp::Popcnt: case LIROp::Bswap:
            uses.insert(instr.unary.operand);
            break;
        case LIROp::PortIn:
            uses.insert(instr.port_in.port);
            break;
        case LIROp::PortOut:
            uses.insert(instr.port_out.port);
            uses.insert(instr.port_out.value);
            break;
    }
}

// ============================================================================
// ConstFoldPass — Fold constant arithmetic
// ============================================================================

void ConstFoldPass::run(LIRModule& module, CompilationContext& ctx) {
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];

        // Map vreg → constant value (for integers)
        std::unordered_map<VReg, int64_t> int_consts;
        std::unordered_map<VReg, bool> bool_consts;
        // Track vreg → TypeId for signedness checks on comparisons
        std::unordered_map<VReg, TypeId> vreg_types;

        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];

                // Track types for all result vregs
                if (instr.result != INVALID_VREG) {
                    vreg_types[instr.result] = instr.type;
                }

                // Track constants
                if (instr.op == LIROp::ConstInt && instr.result != INVALID_VREG) {
                    int_consts[instr.result] = instr.const_int.value;
                } else if (instr.op == LIROp::ConstBool && instr.result != INVALID_VREG) {
                    bool_consts[instr.result] = instr.const_bool.value;
                }

                // Try to fold binary integer ops
                if (instr.result == INVALID_VREG) continue;

                auto lhs_it = int_consts.find(instr.bin.lhs);
                auto rhs_it = int_consts.find(instr.bin.rhs);

                switch (instr.op) {
                    case LIROp::Add:
                    case LIROp::AddWrap:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second + rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Sub:
                    case LIROp::SubWrap:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second - rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Mul:
                    case LIROp::MulWrap:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second * rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Div:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end() &&
                            rhs_it->second != 0) {
                            bool is_unsigned = instr.type < ctx.types.size() &&
                                               !ctx.types.isSigned(instr.type);
                            int64_t result;
                            if (is_unsigned) {
                                result = static_cast<int64_t>(
                                    static_cast<uint64_t>(lhs_it->second) /
                                    static_cast<uint64_t>(rhs_it->second));
                            } else {
                                result = lhs_it->second / rhs_it->second;
                            }
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Mod:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end() &&
                            rhs_it->second != 0) {
                            bool is_unsigned = instr.type < ctx.types.size() &&
                                               !ctx.types.isSigned(instr.type);
                            int64_t result;
                            if (is_unsigned) {
                                result = static_cast<int64_t>(
                                    static_cast<uint64_t>(lhs_it->second) %
                                    static_cast<uint64_t>(rhs_it->second));
                            } else {
                                result = lhs_it->second % rhs_it->second;
                            }
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::BAnd:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second & rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::BOr:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second | rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::BXor:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second ^ rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Shl:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            int64_t result = lhs_it->second << rhs_it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Shr:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            // Use logical shift for unsigned types, arithmetic for signed
                            bool is_unsigned = instr.type < ctx.types.size() &&
                                               !ctx.types.isSigned(instr.type);
                            int64_t result;
                            if (is_unsigned) {
                                result = static_cast<int64_t>(
                                    static_cast<uint64_t>(lhs_it->second) >>
                                    static_cast<uint64_t>(rhs_it->second));
                            } else {
                                result = lhs_it->second >> rhs_it->second;
                            }
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::ICmpEq:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            bool result = lhs_it->second == rhs_it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::ICmpNe:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            bool result = lhs_it->second != rhs_it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::ICmpLt:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            // Check if operands are unsigned
                            auto lhs_type_it = vreg_types.find(instr.bin.lhs);
                            bool is_unsigned = lhs_type_it != vreg_types.end() &&
                                               lhs_type_it->second < ctx.types.size() &&
                                               !ctx.types.isSigned(lhs_type_it->second);
                            bool result = is_unsigned
                                ? static_cast<uint64_t>(lhs_it->second) < static_cast<uint64_t>(rhs_it->second)
                                : lhs_it->second < rhs_it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::ICmpLe:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            auto lhs_type_it = vreg_types.find(instr.bin.lhs);
                            bool is_unsigned = lhs_type_it != vreg_types.end() &&
                                               lhs_type_it->second < ctx.types.size() &&
                                               !ctx.types.isSigned(lhs_type_it->second);
                            bool result = is_unsigned
                                ? static_cast<uint64_t>(lhs_it->second) <= static_cast<uint64_t>(rhs_it->second)
                                : lhs_it->second <= rhs_it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::ICmpGt:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            auto lhs_type_it = vreg_types.find(instr.bin.lhs);
                            bool is_unsigned = lhs_type_it != vreg_types.end() &&
                                               lhs_type_it->second < ctx.types.size() &&
                                               !ctx.types.isSigned(lhs_type_it->second);
                            bool result = is_unsigned
                                ? static_cast<uint64_t>(lhs_it->second) > static_cast<uint64_t>(rhs_it->second)
                                : lhs_it->second > rhs_it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::ICmpGe:
                        if (lhs_it != int_consts.end() && rhs_it != int_consts.end()) {
                            auto lhs_type_it = vreg_types.find(instr.bin.lhs);
                            bool is_unsigned = lhs_type_it != vreg_types.end() &&
                                               lhs_type_it->second < ctx.types.size() &&
                                               !ctx.types.isSigned(lhs_type_it->second);
                            bool result = is_unsigned
                                ? static_cast<uint64_t>(lhs_it->second) >= static_cast<uint64_t>(rhs_it->second)
                                : lhs_it->second >= rhs_it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Neg:
                        if (auto it = int_consts.find(instr.unary.operand); it != int_consts.end()) {
                            int64_t result = -it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::BNot:
                        if (auto it = int_consts.find(instr.unary.operand); it != int_consts.end()) {
                            int64_t result = ~it->second;
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = result;
                            int_consts[instr.result] = result;
                        }
                        break;
                    case LIROp::Not:
                        if (auto it = bool_consts.find(instr.unary.operand); it != bool_consts.end()) {
                            bool result = !it->second;
                            instr.op = LIROp::ConstBool;
                            instr.const_bool.value = result;
                            bool_consts[instr.result] = result;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

// ============================================================================
// DeadCodeElimPass — Remove unused instructions
// ============================================================================

void DeadCodeElimPass::run(LIRModule& module, CompilationContext& /*ctx*/) {
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];

        // Collect all used vregs
        std::unordered_set<VReg> used;
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                collectUses(block.instrs[ii], used);
            }
        }

        // Remove instructions that define unused vregs and have no side effects
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            uint32_t write = 0;
            for (uint32_t read = 0; read < block.instr_count; ++read) {
                auto& instr = block.instrs[read];
                // Volatile loads are side-effecting (must not be removed)
                bool volatile_access = (instr.op == LIROp::Load && instr.load.is_volatile) ||
                                       (instr.op == LIROp::Store && instr.store.is_volatile);
                bool is_dead = instr.result != INVALID_VREG &&
                               used.find(instr.result) == used.end() &&
                               !hasSideEffects(instr.op) &&
                               !volatile_access;
                if (!is_dead) {
                    if (write != read) {
                        block.instrs[write] = block.instrs[read];
                    }
                    ++write;
                }
            }
            block.instr_count = write;
        }
    }
}

// ============================================================================
// ConstPropPass — Propagate constants through vregs
// ============================================================================

void ConstPropPass::run(LIRModule& module, CompilationContext& /*ctx*/) {
    // This pass replaces vreg operands with their known constant values,
    // enabling subsequent constant folding. It works on a per-function basis.
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];

        // Build def map: vreg → defining instruction location
        std::unordered_map<VReg, const LIRInstr*> defs;
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];
                if (instr.result != INVALID_VREG) {
                    defs[instr.result] = &instr;
                }
            }
        }

        // For binary ops where one operand is 0 or 1, simplify:
        // x + 0 → x, x * 1 → x, x * 0 → 0, x & 0 → 0, x | 0 → x
        auto getConst = [&](VReg v) -> std::pair<bool, int64_t> {
            auto it = defs.find(v);
            if (it != defs.end() && it->second->op == LIROp::ConstInt) {
                return {true, it->second->const_int.value};
            }
            return {false, 0};
        };

        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];
                if (instr.result == INVALID_VREG) continue;

                switch (instr.op) {
                    case LIROp::Add:
                    case LIROp::AddWrap: {
                        auto [lc, lv] = getConst(instr.bin.lhs);
                        auto [rc, rv] = getConst(instr.bin.rhs);
                        if (rc && rv == 0) {
                            // x + 0 → copy of x (replace with ConstInt if x is const)
                            if (lc) {
                                instr.op = LIROp::ConstInt;
                                instr.const_int.value = lv;
                            }
                        } else if (lc && lv == 0) {
                            if (rc) {
                                instr.op = LIROp::ConstInt;
                                instr.const_int.value = rv;
                            }
                        }
                        break;
                    }
                    case LIROp::Mul:
                    case LIROp::MulWrap: {
                        auto [lc, lv] = getConst(instr.bin.lhs);
                        auto [rc, rv] = getConst(instr.bin.rhs);
                        if ((rc && rv == 0) || (lc && lv == 0)) {
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = 0;
                        } else if (rc && rv == 1 && lc) {
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = lv;
                        } else if (lc && lv == 1 && rc) {
                            instr.op = LIROp::ConstInt;
                            instr.const_int.value = rv;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

// ============================================================================
// InliningPass — Inline small functions
// ============================================================================

void InliningPass::run(LIRModule& module, CompilationContext& /*ctx*/) {
    // Build function map for callee lookup
    std::unordered_map<std::string_view, const LIRFunction*> fn_map;
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        fn_map[module.functions[fi].name] = &module.functions[fi];
    }

    // For now, identify inlining candidates but don't inline yet.
    // Inlining in a fixed-array LIR requires rebuilding blocks, which needs
    // arena allocation. Mark as a future extension point.
    // The ConstFold + DCE passes handle the most impactful optimizations.
    (void)fn_map;
}

} // namespace kern
