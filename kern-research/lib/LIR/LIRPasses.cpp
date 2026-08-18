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
        case LIROp::AtomicCas128:
        case LIROp::AtomicFetchAdd:
        case LIROp::AtomicFetchSub:
        case LIROp::AtomicFetchAnd:
        case LIROp::AtomicFetchOr:
        case LIROp::AtomicFetchXor:
        case LIROp::Fence:
        case LIROp::CompilerFence:
        case LIROp::PercpuStore:
        case LIROp::PortIn:
        case LIROp::PortOut:
        case LIROp::Trap:
        case LIROp::Switch:
        case LIROp::VaStart:
        case LIROp::VaArg:
        case LIROp::Alloca:
        case LIROp::TlsLoad:
        case LIROp::TlsStore:
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
        case LIROp::ConstCString:
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
        case LIROp::AtomicCas128:
            uses.insert(instr.atomic_cas128.ptr);
            uses.insert(instr.atomic_cas128.exp_lo);
            uses.insert(instr.atomic_cas128.exp_hi);
            uses.insert(instr.atomic_cas128.des_lo);
            uses.insert(instr.atomic_cas128.des_hi);
            break;
        case LIROp::AtomicFetchAdd:
            uses.insert(instr.atomic_fetch_add.ptr);
            uses.insert(instr.atomic_fetch_add.value);
            break;
        case LIROp::AtomicFetchSub:
            uses.insert(instr.atomic_fetch_sub.ptr);
            uses.insert(instr.atomic_fetch_sub.value);
            break;
        case LIROp::AtomicFetchAnd:
        case LIROp::AtomicFetchOr:
        case LIROp::AtomicFetchXor:
            uses.insert(instr.atomic_rmw.ptr);
            uses.insert(instr.atomic_rmw.value);
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
        case LIROp::Switch:
            uses.insert(instr.switch_.scrutinee);
            break;
        case LIROp::VaStart:
            break;  // no vreg operands (fixed_param_count is a constant)
        case LIROp::VaArg:
            uses.insert(instr.va_arg.ap);
            break;
        case LIROp::Alloca:
            uses.insert(instr.alloca_.size);
            break;
        case LIROp::TlsLoad:
            uses.insert(instr.tls_load.offset);
            break;
        case LIROp::TlsStore:
            uses.insert(instr.tls_store.offset);
            uses.insert(instr.tls_store.value);
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
// CSEPass — Local Common Subexpression Elimination
// ============================================================================

// CSE key: encodes opcode + operands for equality testing
struct CSEKey {
    LIROp op;
    TypeId type;
    VReg op1, op2;        // binary operands or single unary operand
    int64_t imm;          // for FieldPtr offset
    std::string_view name; // for FnRef, GlobalRef, LoadGlobal

    bool operator==(const CSEKey& o) const {
        return op == o.op && type == o.type && op1 == o.op1 &&
               op2 == o.op2 && imm == o.imm && name == o.name;
    }
};

struct CSEKeyHash {
    size_t operator()(const CSEKey& k) const {
        size_t h = std::hash<uint32_t>{}(static_cast<uint32_t>(k.op));
        h ^= std::hash<uint32_t>{}(k.type) * 2654435761u;
        h ^= std::hash<uint32_t>{}(k.op1) * 40503u;
        h ^= std::hash<uint32_t>{}(k.op2) * 65537u;
        h ^= std::hash<int64_t>{}(k.imm) * 16777619u;
        if (!k.name.empty())
            h ^= std::hash<std::string_view>{}(k.name);
        return h;
    }
};

// Returns true if the instruction is a pure computation eligible for CSE
static bool isCSECandidate(LIROp op) {
    switch (op) {
        // Arithmetic / comparison — pure
        case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
        case LIROp::Div: case LIROp::Mod:
        case LIROp::BAnd: case LIROp::BOr: case LIROp::BXor:
        case LIROp::Shl: case LIROp::Shr:
        case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul: case LIROp::FDiv:
        case LIROp::ICmpEq: case LIROp::ICmpNe:
        case LIROp::ICmpLt: case LIROp::ICmpLe:
        case LIROp::ICmpGt: case LIROp::ICmpGe:
        case LIROp::FCmpEq: case LIROp::FCmpNe:
        case LIROp::FCmpLt: case LIROp::FCmpLe:
        case LIROp::FCmpGt: case LIROp::FCmpGe:
        // Unary
        case LIROp::Neg: case LIROp::FNeg: case LIROp::BNot: case LIROp::Not:
        case LIROp::Cast:
        // Bit intrinsics
        case LIROp::Clz: case LIROp::Ctz: case LIROp::Popcnt: case LIROp::Bswap:
        // Struct field offset (pure — address computation)
        case LIROp::FieldPtr:
        // Function ref (constant)
        case LIROp::FnRef:
            return true;
        default:
            return false;
    }
}

static CSEKey makeCSEKey(const LIRInstr& i) {
    CSEKey k{};
    k.op = i.op;
    k.type = i.type;
    k.op1 = INVALID_VREG;
    k.op2 = INVALID_VREG;
    k.imm = 0;

    switch (i.op) {
        // Binary ops
        case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
        case LIROp::Div: case LIROp::Mod:
        case LIROp::BAnd: case LIROp::BOr: case LIROp::BXor:
        case LIROp::Shl: case LIROp::Shr:
        case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul: case LIROp::FDiv:
        case LIROp::ICmpEq: case LIROp::ICmpNe:
        case LIROp::ICmpLt: case LIROp::ICmpLe:
        case LIROp::ICmpGt: case LIROp::ICmpGe:
        case LIROp::FCmpEq: case LIROp::FCmpNe:
        case LIROp::FCmpLt: case LIROp::FCmpLe:
        case LIROp::FCmpGt: case LIROp::FCmpGe:
            k.op1 = i.bin.lhs;
            k.op2 = i.bin.rhs;
            break;
        // Unary ops
        case LIROp::Neg: case LIROp::FNeg: case LIROp::BNot: case LIROp::Not:
        case LIROp::Clz: case LIROp::Ctz: case LIROp::Popcnt: case LIROp::Bswap:
            k.op1 = i.unary.operand;
            break;
        case LIROp::Cast:
            k.op1 = i.cast.operand;
            break;
        case LIROp::FieldPtr:
            k.op1 = i.field_ptr.base;
            k.imm = i.field_ptr.offset;
            break;
        case LIROp::FnRef:
            k.name = i.fn_ref.fn_name;
            break;
        default:
            break;
    }
    return k;
}

void CSEPass::run(LIRModule& module, CompilationContext& /*ctx*/) {
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];
        if (fn.is_intrinsic || fn.is_extern) continue;

        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            // Map: CSEKey → first vreg that computed this expression
            std::unordered_map<CSEKey, VReg, CSEKeyHash> expr_map;
            // Map: vregs to replace (old → new)
            std::unordered_map<VReg, VReg> replacements;

            // First pass: find CSE opportunities
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];
                if (!isCSECandidate(instr.op)) continue;
                if (instr.result == INVALID_VREG) continue;

                CSEKey key = makeCSEKey(instr);
                auto it = expr_map.find(key);
                if (it != expr_map.end()) {
                    // Duplicate! Map this result to the earlier one
                    replacements[instr.result] = it->second;
                    // Convert to dead ConstInt (DCE will remove it)
                    instr.op = LIROp::ConstInt;
                    instr.const_int.value = 0;
                } else {
                    expr_map[key] = instr.result;
                }
            }

            // Second pass: apply replacements to all operands in this block
            if (replacements.empty()) continue;
            auto repl = [&](VReg v) -> VReg {
                auto it = replacements.find(v);
                return it != replacements.end() ? it->second : v;
            };
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];
                switch (instr.op) {
                    case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
                    case LIROp::Div: case LIROp::Mod:
                    case LIROp::BAnd: case LIROp::BOr: case LIROp::BXor:
                    case LIROp::Shl: case LIROp::Shr:
                    case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul: case LIROp::FDiv:
                    case LIROp::ICmpEq: case LIROp::ICmpNe:
                    case LIROp::ICmpLt: case LIROp::ICmpLe:
                    case LIROp::ICmpGt: case LIROp::ICmpGe:
                    case LIROp::FCmpEq: case LIROp::FCmpNe:
                    case LIROp::FCmpLt: case LIROp::FCmpLe:
                    case LIROp::FCmpGt: case LIROp::FCmpGe:
                        instr.bin.lhs = repl(instr.bin.lhs);
                        instr.bin.rhs = repl(instr.bin.rhs);
                        break;
                    case LIROp::Neg: case LIROp::FNeg:
                    case LIROp::BNot: case LIROp::Not:
                    case LIROp::Clz: case LIROp::Ctz:
                    case LIROp::Popcnt: case LIROp::Bswap:
                        instr.unary.operand = repl(instr.unary.operand);
                        break;
                    case LIROp::Cast:
                        instr.cast.operand = repl(instr.cast.operand);
                        break;
                    case LIROp::Store:
                        instr.store.ptr = repl(instr.store.ptr);
                        instr.store.value = repl(instr.store.value);
                        break;
                    case LIROp::Load:
                        instr.load.ptr = repl(instr.load.ptr);
                        break;
                    case LIROp::FieldPtr:
                        instr.field_ptr.base = repl(instr.field_ptr.base);
                        break;
                    case LIROp::Ret:
                        instr.ret.value = repl(instr.ret.value);
                        break;
                    case LIROp::CondBranch:
                        instr.cond_branch.cond = repl(instr.cond_branch.cond);
                        break;
                    case LIROp::AddrOf:
                        instr.addr_of.source = repl(instr.addr_of.source);
                        break;
                    case LIROp::StoreGlobal:
                        instr.store_global.value = repl(instr.store_global.value);
                        break;
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

// Remap a VReg through a mapping table
static VReg remapVReg(VReg v, const std::unordered_map<VReg, VReg>& m) {
    if (v == INVALID_VREG) return v;
    auto it = m.find(v);
    return it != m.end() ? it->second : v;
}

// Remap all operand vregs in an instruction
static void remapInstrOperands(LIRInstr& i, const std::unordered_map<VReg, VReg>& m) {
    switch (i.op) {
        case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
        case LIROp::Div: case LIROp::Mod:
        case LIROp::AddWrap: case LIROp::SubWrap: case LIROp::MulWrap:
        case LIROp::AddSat: case LIROp::SubSat:
        case LIROp::BAnd: case LIROp::BOr: case LIROp::BXor:
        case LIROp::Shl: case LIROp::Shr:
        case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul: case LIROp::FDiv:
        case LIROp::ICmpEq: case LIROp::ICmpNe:
        case LIROp::ICmpLt: case LIROp::ICmpLe:
        case LIROp::ICmpGt: case LIROp::ICmpGe:
        case LIROp::FCmpEq: case LIROp::FCmpNe:
        case LIROp::FCmpLt: case LIROp::FCmpLe:
        case LIROp::FCmpGt: case LIROp::FCmpGe:
            i.bin.lhs = remapVReg(i.bin.lhs, m);
            i.bin.rhs = remapVReg(i.bin.rhs, m);
            break;
        case LIROp::Neg: case LIROp::FNeg:
        case LIROp::BNot: case LIROp::Not:
        case LIROp::Clz: case LIROp::Ctz:
        case LIROp::Popcnt: case LIROp::Bswap:
            i.unary.operand = remapVReg(i.unary.operand, m);
            break;
        case LIROp::Cast:
            i.cast.operand = remapVReg(i.cast.operand, m);
            break;
        case LIROp::Store:
            i.store.ptr = remapVReg(i.store.ptr, m);
            i.store.value = remapVReg(i.store.value, m);
            break;
        case LIROp::Load:
            i.load.ptr = remapVReg(i.load.ptr, m);
            break;
        case LIROp::FieldPtr:
            i.field_ptr.base = remapVReg(i.field_ptr.base, m);
            break;
        case LIROp::Ret:
            i.ret.value = remapVReg(i.ret.value, m);
            break;
        case LIROp::CondBranch:
            i.cond_branch.cond = remapVReg(i.cond_branch.cond, m);
            break;
        case LIROp::AddrOf:
            i.addr_of.source = remapVReg(i.addr_of.source, m);
            break;
        case LIROp::StoreGlobal:
            i.store_global.value = remapVReg(i.store_global.value, m);
            break;
        case LIROp::StructAlloc:
            break; // StructAlloc has no vreg operands (only size/align)
        default:
            break;
    }
}

void InliningPass::run(LIRModule& module, CompilationContext& ctx) {
    // Build function map for callee lookup
    std::unordered_map<std::string_view, const LIRFunction*> fn_map;
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        fn_map[module.functions[fi].name] = &module.functions[fi];
    }

    constexpr uint32_t AUTO_INLINE_LIMIT = 10;

    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& caller = module.functions[fi];
        if (caller.is_intrinsic || caller.is_extern) continue;

        // Iterate blocks; inline at most once per pass to keep things simple
        for (uint32_t bi = 0; bi < caller.block_count; ++bi) {
            auto& block = caller.blocks[bi];

            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& call_instr = block.instrs[ii];
                if (call_instr.op != LIROp::Call) continue;
                if (call_instr.call.is_tail) continue; // skip tail calls

                auto it = fn_map.find(call_instr.call.callee);
                if (it == fn_map.end()) continue;
                const auto* callee = it->second;

                // Eligibility checks
                if (callee->block_count != 1) continue;         // single-block only
                if (callee->is_noinline) continue;
                if (callee->is_recursive) continue;
                if (callee->is_naked || callee->is_interrupt) continue;
                if (callee->is_variadic) continue;
                uint32_t body_count = callee->blocks[0].instr_count;
                if (!callee->is_inline && body_count > AUTO_INLINE_LIMIT) continue;

                const auto& cb = callee->blocks[0]; // callee block

                // Build vreg mapping: callee vregs → caller vregs
                std::unordered_map<VReg, VReg> vmap;

                // Phase 1: map BlockArg vregs to call argument vregs
                for (uint32_t ci = 0; ci < cb.instr_count; ++ci) {
                    if (cb.instrs[ci].op == LIROp::BlockArg) {
                        uint32_t idx = cb.instrs[ci].block_arg.index;
                        if (idx < call_instr.call.arg_count) {
                            vmap[cb.instrs[ci].result] = call_instr.call.args[idx];
                        }
                    }
                }

                // Phase 2: find ret vreg and allocate fresh vregs for computations
                VReg callee_ret_vreg = INVALID_VREG;
                for (uint32_t ci = 0; ci < cb.instr_count; ++ci) {
                    auto& ci_instr = cb.instrs[ci];
                    if (ci_instr.op == LIROp::BlockArg) continue;
                    if (ci_instr.op == LIROp::Ret) {
                        callee_ret_vreg = ci_instr.ret.value;
                        continue;
                    }
                    if (ci_instr.result != INVALID_VREG) {
                        vmap[ci_instr.result] = caller.next_vreg++;
                    }
                }

                // Determine the remapped return vreg
                VReg mapped_ret = remapVReg(callee_ret_vreg, vmap);

                // Count instructions to inline (skip BlockArg and Ret)
                uint32_t inline_count = 0;
                for (uint32_t ci = 0; ci < cb.instr_count; ++ci) {
                    auto op = cb.instrs[ci].op;
                    if (op != LIROp::BlockArg && op != LIROp::Ret) inline_count++;
                }

                // Build new instruction array:
                // [before call] + [inlined body] + [after call]
                uint32_t new_count = block.instr_count - 1 + inline_count;
                auto* new_instrs = ctx.arena.makeArray<LIRInstr>(new_count);
                uint32_t ni = 0;

                // Copy instructions before the call
                for (uint32_t k = 0; k < ii; ++k) {
                    new_instrs[ni++] = block.instrs[k];
                }

                // Clone and remap callee instructions
                for (uint32_t ci = 0; ci < cb.instr_count; ++ci) {
                    auto& ci_instr = cb.instrs[ci];
                    if (ci_instr.op == LIROp::BlockArg) continue;
                    if (ci_instr.op == LIROp::Ret) continue;

                    LIRInstr cloned = ci_instr;
                    if (cloned.result != INVALID_VREG) {
                        cloned.result = remapVReg(cloned.result, vmap);
                    }
                    remapInstrOperands(cloned, vmap);
                    new_instrs[ni++] = cloned;
                }

                // Copy instructions after the call, remapping call result uses
                VReg call_result = call_instr.result;
                std::unordered_map<VReg, VReg> result_map;
                if (call_result != INVALID_VREG && mapped_ret != INVALID_VREG) {
                    result_map[call_result] = mapped_ret;
                }
                for (uint32_t k = ii + 1; k < block.instr_count; ++k) {
                    LIRInstr copied = block.instrs[k];
                    if (!result_map.empty()) {
                        remapInstrOperands(copied, result_map);
                    }
                    new_instrs[ni++] = copied;
                }

                // Replace block's instruction array
                block.instrs = new_instrs;
                block.instr_count = new_count;

                // Restart scanning this block (indices changed)
                break;
            }
        }
    }
}

// ============================================================================
// StrengthReductionPass
// ============================================================================

static bool isPowerOfTwo(int64_t val) {
    return val > 0 && (val & (val - 1)) == 0;
}

static int log2i(int64_t val) {
    int n = 0;
    while (val > 1) { val >>= 1; ++n; }
    return n;
}

void StrengthReductionPass::run(LIRModule& module, CompilationContext& ctx) {
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];

        // Collect constant integer values
        std::unordered_map<VReg, int64_t> int_consts;

        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];

                if (instr.op == LIROp::ConstInt && instr.result != INVALID_VREG) {
                    int_consts[instr.result] = instr.const_int.value;
                }

                if (instr.result == INVALID_VREG) continue;

                bool is_unsigned = instr.type < ctx.types.size() &&
                                   !ctx.types.isSigned(instr.type);

                // Unsigned div by power-of-2 → logical right shift
                if (instr.op == LIROp::Div && is_unsigned) {
                    auto rhs_it = int_consts.find(instr.bin.rhs);
                    if (rhs_it != int_consts.end() && isPowerOfTwo(rhs_it->second)) {
                        int shift = log2i(rhs_it->second);
                        // Replace divisor constant with shift amount
                        instr.op = LIROp::Shr;
                        // Need a new const for the shift amount — rewrite rhs
                        // Since we can't easily create new vregs here, check if
                        // the rhs const already has the right value or rewrite in-place
                        // Actually: the rhs vreg holds the power-of-2 value.
                        // We need it to hold the shift amount. We can modify the
                        // ConstInt instruction that defines rhs if it's only used here.
                        // For simplicity, just change the op. The ConstFoldPass after
                        // this will fold Shr with a ConstInt rhs anyway.
                        // But we need rhs to be log2(val), not val itself.
                        // Best approach: scan back for the ConstInt and change its value.
                        for (uint32_t si = 0; si < ii; ++si) {
                            auto& src = block.instrs[si];
                            if (src.op == LIROp::ConstInt && src.result == instr.bin.rhs) {
                                src.const_int.value = shift;
                                int_consts[src.result] = shift;
                                break;
                            }
                        }
                    }
                }

                // Unsigned mod by power-of-2 → bitwise AND (val & (divisor - 1))
                if (instr.op == LIROp::Mod && is_unsigned) {
                    auto rhs_it = int_consts.find(instr.bin.rhs);
                    if (rhs_it != int_consts.end() && isPowerOfTwo(rhs_it->second)) {
                        int64_t mask = rhs_it->second - 1;
                        instr.op = LIROp::BAnd;
                        for (uint32_t si = 0; si < ii; ++si) {
                            auto& src = block.instrs[si];
                            if (src.op == LIROp::ConstInt && src.result == instr.bin.rhs) {
                                src.const_int.value = mask;
                                int_consts[src.result] = mask;
                                break;
                            }
                        }
                    }
                }

                // Mul by power-of-2 → left shift (both signed and unsigned)
                if (instr.op == LIROp::Mul) {
                    auto rhs_it = int_consts.find(instr.bin.rhs);
                    if (rhs_it != int_consts.end() && isPowerOfTwo(rhs_it->second)) {
                        int shift = log2i(rhs_it->second);
                        instr.op = LIROp::Shl;
                        for (uint32_t si = 0; si < ii; ++si) {
                            auto& src = block.instrs[si];
                            if (src.op == LIROp::ConstInt && src.result == instr.bin.rhs) {
                                src.const_int.value = shift;
                                int_consts[src.result] = shift;
                                break;
                            }
                        }
                    }
                    // Also check lhs for mul commutativity
                    auto lhs_it = int_consts.find(instr.bin.lhs);
                    if (lhs_it != int_consts.end() && isPowerOfTwo(lhs_it->second) &&
                        instr.op == LIROp::Mul) {
                        int shift = log2i(lhs_it->second);
                        // Swap so lhs is the variable, rhs is the shift amount
                        VReg tmp = instr.bin.lhs;
                        instr.bin.lhs = instr.bin.rhs;
                        instr.bin.rhs = tmp;
                        instr.op = LIROp::Shl;
                        for (uint32_t si = 0; si < ii; ++si) {
                            auto& src = block.instrs[si];
                            if (src.op == LIROp::ConstInt && src.result == instr.bin.rhs) {
                                src.const_int.value = shift;
                                int_consts[src.result] = shift;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// LICMPass — Loop-Invariant Code Motion
// ============================================================================

// Check if an instruction is pure (no side effects, safe to move)
static bool isPureInstr(const LIRInstr& instr) {
    switch (instr.op) {
        case LIROp::ConstInt:
        case LIROp::ConstFloat:
        case LIROp::ConstBool:
        case LIROp::ConstString:
        case LIROp::ConstCString:
        case LIROp::Add:
        case LIROp::Sub:
        case LIROp::Mul:
        case LIROp::Div:
        case LIROp::Mod:
        case LIROp::AddWrap:
        case LIROp::SubWrap:
        case LIROp::MulWrap:
        case LIROp::AddSat:
        case LIROp::SubSat:
        case LIROp::BAnd:
        case LIROp::BOr:
        case LIROp::BXor:
        case LIROp::Shl:
        case LIROp::Shr:
        case LIROp::FAdd:
        case LIROp::FSub:
        case LIROp::FMul:
        case LIROp::FDiv:
        case LIROp::Neg:
        case LIROp::BNot:
        case LIROp::FNeg:
        case LIROp::ICmpEq:
        case LIROp::ICmpNe:
        case LIROp::ICmpLt:
        case LIROp::ICmpLe:
        case LIROp::ICmpGt:
        case LIROp::ICmpGe:
        case LIROp::FCmpEq:
        case LIROp::FCmpNe:
        case LIROp::FCmpLt:
        case LIROp::FCmpLe:
        case LIROp::FCmpGt:
        case LIROp::FCmpGe:
        case LIROp::Cast:
        case LIROp::FieldPtr:
        case LIROp::FnRef:
        case LIROp::GlobalRef:
            return true;
        default:
            return false;
    }
}

// Get operand vregs used by an instruction
static void getOperandVRegs(const LIRInstr& instr, std::vector<VReg>& out) {
    switch (instr.op) {
        case LIROp::ConstInt:
        case LIROp::ConstFloat:
        case LIROp::ConstBool:
        case LIROp::ConstString:
        case LIROp::ConstCString:
        case LIROp::FnRef:
        case LIROp::GlobalRef:
            break; // no vreg operands
        case LIROp::Neg:
        case LIROp::BNot:
        case LIROp::FNeg:
            out.push_back(instr.unary.operand);
            break;
        case LIROp::Cast:
            out.push_back(instr.cast.operand);
            break;
        case LIROp::FieldPtr:
            out.push_back(instr.field_ptr.base);
            break;
        default:
            // Binary ops use bin.lhs, bin.rhs
            if (instr.bin.lhs != INVALID_VREG) out.push_back(instr.bin.lhs);
            if (instr.bin.rhs != INVALID_VREG) out.push_back(instr.bin.rhs);
            break;
    }
}

void LICMPass::run(LIRModule& module, CompilationContext& ctx) {
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];
        if (fn.block_count < 2) continue;

        // Build CFG: predecessors for each block
        std::vector<std::vector<uint32_t>> preds(fn.block_count);
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            if (block.instr_count == 0) continue;
            auto& last = block.instrs[block.instr_count - 1];
            if (last.op == LIROp::Branch) {
                preds[last.branch.target].push_back(bi);
            } else if (last.op == LIROp::CondBranch) {
                preds[last.cond_branch.true_target].push_back(bi);
                preds[last.cond_branch.false_target].push_back(bi);
            } else if (last.op == LIROp::Switch) {
                for (uint32_t ci = 0; ci < last.switch_.case_count; ++ci) {
                    preds[last.switch_.cases[ci].target_block].push_back(bi);
                }
                preds[last.switch_.default_block].push_back(bi);
            }
        }

        // Compute dominance ordering (simple: block 0 dominates all)
        // Find back-edges: edge (src → dst) where dst <= src (loop header)
        // Identify natural loops for each back-edge
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            if (block.instr_count == 0) continue;
            auto& last = block.instrs[block.instr_count - 1];

            auto processBackEdge = [&](uint32_t src, uint32_t header) {
                if (header >= src) return; // not a back-edge
                if (header >= fn.block_count) return;

                // Collect loop body blocks via reverse BFS from src to header
                std::unordered_set<uint32_t> loop_blocks;
                loop_blocks.insert(header);
                loop_blocks.insert(src);

                std::vector<uint32_t> worklist;
                if (src != header) worklist.push_back(src);

                while (!worklist.empty()) {
                    uint32_t b = worklist.back();
                    worklist.pop_back();
                    for (uint32_t p : preds[b]) {
                        if (loop_blocks.insert(p).second) {
                            worklist.push_back(p);
                        }
                    }
                }

                // Collect all vregs defined inside the loop
                std::unordered_set<VReg> loop_defs;
                for (uint32_t lb : loop_blocks) {
                    auto& lblock = fn.blocks[lb];
                    for (uint32_t ii = 0; ii < lblock.instr_count; ++ii) {
                        if (lblock.instrs[ii].result != INVALID_VREG) {
                            loop_defs.insert(lblock.instrs[ii].result);
                        }
                    }
                    // Block parameters are also loop defs
                    for (uint32_t pi = 0; pi < lblock.param_count; ++pi) {
                        // Block params don't have explicit vregs in the block struct,
                        // but BlockArg instructions reference them. Already covered above.
                    }
                }

                // Identify loop-invariant instructions
                // An instruction is loop-invariant if:
                // 1. It is pure
                // 2. All its operand vregs are defined outside the loop
                //    (or are themselves loop-invariant constants)
                std::vector<std::pair<uint32_t, uint32_t>> to_hoist; // (block_idx, instr_idx)

                for (uint32_t lb : loop_blocks) {
                    if (lb == header) continue; // don't hoist from header itself
                    auto& lblock = fn.blocks[lb];
                    for (uint32_t ii = 0; ii < lblock.instr_count; ++ii) {
                        auto& instr = lblock.instrs[ii];
                        if (!isPureInstr(instr)) continue;
                        if (instr.result == INVALID_VREG) continue;

                        std::vector<VReg> operands;
                        getOperandVRegs(instr, operands);

                        bool all_outside = true;
                        for (VReg v : operands) {
                            if (loop_defs.count(v)) {
                                all_outside = false;
                                break;
                            }
                        }

                        if (all_outside) {
                            to_hoist.push_back({lb, ii});
                        }
                    }
                }

                if (to_hoist.empty()) return;

                // Find preheader: the unique predecessor of header that is NOT in the loop
                uint32_t preheader = UINT32_MAX;
                for (uint32_t p : preds[header]) {
                    if (!loop_blocks.count(p)) {
                        preheader = p;
                        break;
                    }
                }
                if (preheader == UINT32_MAX) return;

                // Hoist instructions to the preheader (insert before terminator)
                auto& ph = fn.blocks[preheader];
                uint32_t insert_pos = (ph.instr_count > 0) ? ph.instr_count - 1 : 0;

                // Rebuild preheader with hoisted instructions
                uint32_t new_count = ph.instr_count + static_cast<uint32_t>(to_hoist.size());
                auto* new_instrs = ctx.arena.makeArray<LIRInstr>(new_count);
                // Copy pre-terminator instructions
                for (uint32_t k = 0; k < insert_pos; ++k) {
                    new_instrs[k] = ph.instrs[k];
                }
                // Insert hoisted instructions
                for (uint32_t k = 0; k < to_hoist.size(); ++k) {
                    auto [blk_idx, ii_idx] = to_hoist[k];
                    new_instrs[insert_pos + k] = fn.blocks[blk_idx].instrs[ii_idx];
                }
                // Copy terminator
                for (uint32_t k = insert_pos; k < ph.instr_count; ++k) {
                    new_instrs[k + static_cast<uint32_t>(to_hoist.size())] = ph.instrs[k];
                }
                ph.instrs = new_instrs;
                ph.instr_count = new_count;

                // NOP out original instructions (replace with ConstInt 0 → same result vreg)
                for (auto [blk_idx, ii_idx] : to_hoist) {
                    auto& orig = fn.blocks[blk_idx].instrs[ii_idx];
                    // Mark as no-op by making it a copy of itself from the hoisted location
                    // Actually: just NOP it. DCE will clean up.
                    // Turn into a harmless ConstInt that DCE will remove
                    VReg result = orig.result;
                    TypeId type = orig.type;
                    orig.op = LIROp::ConstInt;
                    orig.result = result;
                    orig.type = type;
                    orig.const_int.value = 0;
                }
            };

            if (last.op == LIROp::Branch) {
                processBackEdge(bi, last.branch.target);
            } else if (last.op == LIROp::CondBranch) {
                processBackEdge(bi, last.cond_branch.true_target);
                processBackEdge(bi, last.cond_branch.false_target);
            }
        }
    }
}

// ============================================================================
// GlobalDCEPass — Remove entire functions that are never called
// ============================================================================

void GlobalDCEPass::run(LIRModule& module, CompilationContext& /*ctx*/) {
    if (module.fn_count == 0) return;

    // Build name → index map
    std::unordered_map<std::string_view, uint32_t> fn_index;
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        fn_index[module.functions[i].name] = i;
    }

    // Determine root functions (cannot be removed)
    std::vector<bool> is_root(module.fn_count, false);
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        auto& fn = module.functions[i];
        if (fn.is_pub || fn.is_used || fn.is_extern || fn.is_interrupt ||
            fn.is_constructor || fn.is_destructor || fn.is_weak ||
            fn.name == "main" || fn.name == "_start") {
            is_root[i] = true;
        }
        // Panic handler
        if (!fn.link_name.empty()) is_root[i] = true;
        // Functions with custom sections are usually externally referenced
        if (!fn.section_name.empty()) is_root[i] = true;
    }

    // Build call graph: for each function, collect set of callees
    std::vector<std::vector<uint32_t>> callees(module.fn_count);
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto& fn = module.functions[fi];
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            auto& block = fn.blocks[bi];
            for (uint32_t ii = 0; ii < block.instr_count; ++ii) {
                auto& instr = block.instrs[ii];
                if (instr.op == LIROp::Call) {
                    auto it = fn_index.find(instr.call.callee);
                    if (it != fn_index.end()) {
                        callees[fi].push_back(it->second);
                    }
                } else if (instr.op == LIROp::FnRef) {
                    // Function pointer references count as uses
                    auto it = fn_index.find(instr.fn_ref.fn_name);
                    if (it != fn_index.end()) {
                        callees[fi].push_back(it->second);
                    }
                }
            }
        }
    }

    // BFS from roots to find all reachable functions
    std::vector<bool> reachable(module.fn_count, false);
    std::vector<uint32_t> worklist;
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        if (is_root[i]) {
            reachable[i] = true;
            worklist.push_back(i);
        }
    }
    while (!worklist.empty()) {
        uint32_t fi = worklist.back();
        worklist.pop_back();
        for (uint32_t callee : callees[fi]) {
            if (!reachable[callee]) {
                reachable[callee] = true;
                worklist.push_back(callee);
            }
        }
    }

    // Compact: remove unreachable functions
    uint32_t write = 0;
    for (uint32_t read = 0; read < module.fn_count; ++read) {
        if (reachable[read]) {
            if (write != read) {
                module.functions[write] = module.functions[read];
            }
            ++write;
        }
    }
    module.fn_count = write;
}

} // namespace kern
