#include "kern/backend/InstructionSelector.h"
#include <cassert>

namespace kern {

// ============================================================================
// Helpers
// ============================================================================

void InstructionSelector::emit(MachInstr mi) {
    block_instrs_[current_block_].push_back(mi);
}

uint8_t InstructionSelector::widthOf(TypeId type) const {
    if (type >= ctx_.types.size()) return 64;
    const auto& info = ctx_.types.get(type);
    if (info.kind == TypeKind::Primitive) {
        switch (info.primitive.prim) {
            case PrimitiveKind::I8:
            case PrimitiveKind::U8:    return 8;
            case PrimitiveKind::I16:
            case PrimitiveKind::U16:   return 16;
            case PrimitiveKind::I32:
            case PrimitiveKind::U32:
            case PrimitiveKind::F32:   return 32;
            case PrimitiveKind::I64:
            case PrimitiveKind::U64:
            case PrimitiveKind::F64:   return 64;
            case PrimitiveKind::Bool:  return 8;
            case PrimitiveKind::Unit:  return 0;
            default: return 64;
        }
    }
    return 64;  // pointers, structs, etc.
}

bool InstructionSelector::isFloat(TypeId type) const {
    if (type >= ctx_.types.size()) return false;
    const auto& info = ctx_.types.get(type);
    if (info.kind == TypeKind::Primitive) {
        return info.primitive.prim == PrimitiveKind::F32 ||
               info.primitive.prim == PrimitiveKind::F64;
    }
    return false;
}

std::string_view InstructionSelector::blockLabel(const LIRFunction& fn,
                                                  uint32_t block_idx) const {
    if (block_idx < fn.block_count) {
        return fn.blocks[block_idx].label;
    }
    return "";
}

// ============================================================================
// Top-level selection
// ============================================================================

MachModule* InstructionSelector::select(const LIRModule& lir_mod) {
    auto* mod = ctx_.arena.make<MachModule>();
    mod->fn_count = lir_mod.fn_count;
    mod->functions = ctx_.arena.makeArray<MachFunction>(lir_mod.fn_count);

    for (uint32_t i = 0; i < lir_mod.fn_count; ++i) {
        auto* mf = selectFunction(lir_mod.functions[i]);
        mod->functions[i] = *mf;
    }

    return mod;
}

MachFunction* InstructionSelector::selectFunction(const LIRFunction& fn) {
    auto* mf = ctx_.arena.make<MachFunction>();
    mf->name = fn.name;
    mf->is_intrinsic = fn.is_intrinsic;
    mf->next_vreg = fn.next_vreg;
    next_vreg_ = fn.next_vreg;

    if (fn.is_intrinsic || fn.block_count == 0) {
        mf->block_count = 0;
        mf->blocks = nullptr;
        mf->stack_size = 0;
        return mf;
    }

    // Allocate instruction vectors per block
    block_instrs_.clear();
    block_instrs_.resize(fn.block_count);

    // Select instructions for each block
    for (uint32_t b = 0; b < fn.block_count; ++b) {
        current_block_ = b;
        const auto& block = fn.blocks[b];
        for (uint32_t i = 0; i < block.instr_count; ++i) {
            selectInstr(block.instrs[i], fn);
        }
    }

    // Build MachBlocks
    mf->block_count = fn.block_count;
    mf->blocks = ctx_.arena.makeArray<MachBlock>(fn.block_count);

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        auto& mb = mf->blocks[b];
        mb.label = fn.blocks[b].label;
        auto& instrs = block_instrs_[b];
        mb.instr_count = static_cast<uint32_t>(instrs.size());
        mb.instrs = ctx_.arena.makeArray<MachInstr>(mb.instr_count);
        for (uint32_t i = 0; i < mb.instr_count; ++i) {
            mb.instrs[i] = instrs[i];
        }
    }

    mf->stack_size = 0;  // computed by RegisterAllocator later
    mf->next_vreg = next_vreg_;
    return mf;
}

// ============================================================================
// Instruction dispatch
// ============================================================================

void InstructionSelector::selectInstr(const LIRInstr& instr,
                                       const LIRFunction& fn) {
    switch (instr.op) {
        case LIROp::ConstInt:    selectConstInt(instr); break;
        case LIROp::ConstFloat:  selectConstFloat(instr); break;
        case LIROp::ConstBool:   selectConstBool(instr); break;
        case LIROp::ConstString: selectConstString(instr); break;
        case LIROp::GlobalRef:   selectGlobalRef(instr); break;

        case LIROp::Add:
        case LIROp::Sub:
        case LIROp::Mul:         selectBinOp(instr); break;
        case LIROp::Div:         selectDiv(instr, false); break;
        case LIROp::Mod:         selectDiv(instr, true); break;

        case LIROp::FAdd:
        case LIROp::FSub:
        case LIROp::FMul:
        case LIROp::FDiv:        selectFloatBinOp(instr); break;

        case LIROp::ICmpEq:
        case LIROp::ICmpNe:
        case LIROp::ICmpLt:
        case LIROp::ICmpLe:
        case LIROp::ICmpGt:
        case LIROp::ICmpGe:      selectICmp(instr); break;

        case LIROp::FCmpEq:
        case LIROp::FCmpNe:
        case LIROp::FCmpLt:
        case LIROp::FCmpLe:
        case LIROp::FCmpGt:
        case LIROp::FCmpGe:      selectFCmp(instr); break;

        case LIROp::Neg:         selectUnaryNeg(instr); break;
        case LIROp::FNeg:        selectUnaryFNeg(instr); break;
        case LIROp::Not:         selectUnaryNot(instr); break;

        case LIROp::AddrOf:      selectAddrOf(instr); break;
        case LIROp::Load:        selectLoad(instr); break;
        case LIROp::Store:       selectStore(instr); break;
        case LIROp::FieldPtr:    selectFieldPtr(instr); break;
        case LIROp::StructAlloc: selectStructAlloc(instr); break;

        case LIROp::Branch:      selectBranch(instr, fn); break;
        case LIROp::CondBranch:  selectCondBranch(instr, fn); break;
        case LIROp::Ret:         selectRet(instr); break;
        case LIROp::Call:        selectCall(instr); break;
        case LIROp::BlockArg:    selectBlockArg(instr); break;
    }
}

// ============================================================================
// Constants
// ============================================================================

void InstructionSelector::selectConstInt(const LIRInstr& instr) {
    // mov vreg, imm
    uint8_t w = widthOf(instr.type);
    if (w == 0) return;  // Unit
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::immediate(instr.const_int.value), w));
}

void InstructionSelector::selectConstBool(const LIRInstr& instr) {
    // mov vreg, 0/1
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::immediate(instr.const_bool.value ? 1 : 0), 8));
}

void InstructionSelector::selectConstFloat(const LIRInstr& instr) {
    // Handled same as GlobalRef — float constants are in .rodata
    // But ConstFloat with no global ref: materialize via mov to GPR then movq
    // In practice, LIRBuilder puts floats into globals. This is fallback.
    (void)instr;
    // No-op for now; LIRBuilder should always use GlobalRef for floats
}

void InstructionSelector::selectConstString(const LIRInstr& instr) {
    // String literal → reference to global
    // lea vreg, [rel _str_N]
    // This will be expanded later with the actual label
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::immediate(instr.const_string.global_index), 64));
}

void InstructionSelector::selectGlobalRef(const LIRInstr& instr) {
    // Load float from .rodata
    // movsd/movss vreg, [rel label]
    uint8_t w = widthOf(instr.type);
    MachInstr mi(w == 32 ? X86Op::Movss : X86Op::Movsd);
    mi.width = w;
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::immediate(instr.global_ref.global_index);
    emit(mi);
}

// ============================================================================
// Integer Binary Ops
// ============================================================================

void InstructionSelector::selectBinOp(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg dst = instr.result;
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;

    // x86 pattern: mov dst, lhs; op dst, rhs
    emit(makeMov(MachOperand::virt(dst), MachOperand::virt(lhs), w));

    X86Op op;
    switch (instr.op) {
        case LIROp::Add: op = X86Op::Add; break;
        case LIROp::Sub: op = X86Op::Sub; break;
        case LIROp::Mul: op = X86Op::IMul; break;
        default: op = X86Op::Add; break;
    }

    emit(makeAlu(op, MachOperand::virt(dst), MachOperand::virt(rhs), w));
}

void InstructionSelector::selectDiv(const LIRInstr& instr, bool is_mod) {
    uint8_t w = widthOf(instr.type);
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;
    VReg dst = instr.result;

    // Check if signed
    bool is_signed = true;
    if (instr.type < ctx_.types.size()) {
        const auto& info = ctx_.types.get(instr.type);
        if (info.kind == TypeKind::Primitive) {
            auto p = info.primitive.prim;
            is_signed = !(p == PrimitiveKind::U8 || p == PrimitiveKind::U16 ||
                          p == PrimitiveKind::U32 || p == PrimitiveKind::U64);
        }
    }

    // x86 idiv: dividend in rdx:rax, divisor in operand
    // mov rax, lhs
    emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                 MachOperand::virt(lhs), w));

    // Sign/zero extend into rdx
    if (is_signed) {
        MachInstr cqo(X86Op::Cqo);
        cqo.width = w;
        cqo.operand_count = 0;
        emit(cqo);
    } else {
        // xor rdx, rdx
        emit(makeAlu(X86Op::Xor,
                     MachOperand::precolored(PhysReg::RDX),
                     MachOperand::precolored(PhysReg::RDX), w));
    }

    // idiv rhs
    MachInstr div(X86Op::IDiv);
    div.width = w;
    div.operand_count = 1;
    div.inline_ops[0] = MachOperand::virt(rhs);
    emit(div);

    // Result: rax = quotient, rdx = remainder
    PhysReg result_reg = is_mod ? PhysReg::RDX : PhysReg::RAX;
    emit(makeMov(MachOperand::virt(dst),
                 MachOperand::precolored(result_reg), w));
}

// ============================================================================
// Float Binary Ops
// ============================================================================

void InstructionSelector::selectFloatBinOp(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg dst = instr.result;
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;

    // movsd dst, lhs
    X86Op mov_op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
    MachInstr mov(mov_op);
    mov.width = w;
    mov.operand_count = 2;
    mov.inline_ops[0] = MachOperand::virt(dst);
    mov.inline_ops[1] = MachOperand::virt(lhs);
    emit(mov);

    // Select SSE op
    X86Op op;
    switch (instr.op) {
        case LIROp::FAdd: op = (w == 32) ? X86Op::Addss : X86Op::Addsd; break;
        case LIROp::FSub: op = (w == 32) ? X86Op::Subss : X86Op::Subsd; break;
        case LIROp::FMul: op = (w == 32) ? X86Op::Mulss : X86Op::Mulsd; break;
        case LIROp::FDiv: op = (w == 32) ? X86Op::Divss : X86Op::Divsd; break;
        default: op = X86Op::Addsd; break;
    }

    MachInstr alu(op);
    alu.width = w;
    alu.operand_count = 2;
    alu.inline_ops[0] = MachOperand::virt(dst);
    alu.inline_ops[1] = MachOperand::virt(rhs);
    emit(alu);
}

// ============================================================================
// Comparisons
// ============================================================================

CondCode InstructionSelector::mapICmpCC(LIROp op) {
    switch (op) {
        case LIROp::ICmpEq: return CondCode::E;
        case LIROp::ICmpNe: return CondCode::NE;
        case LIROp::ICmpLt: return CondCode::L;
        case LIROp::ICmpLe: return CondCode::LE;
        case LIROp::ICmpGt: return CondCode::G;
        case LIROp::ICmpGe: return CondCode::GE;
        default: return CondCode::E;
    }
}

CondCode InstructionSelector::mapFCmpCC(LIROp op) {
    switch (op) {
        case LIROp::FCmpEq: return CondCode::E;
        case LIROp::FCmpNe: return CondCode::NE;
        case LIROp::FCmpLt: return CondCode::B;  // unsigned for ucomisd
        case LIROp::FCmpLe: return CondCode::BE;
        case LIROp::FCmpGt: return CondCode::A;
        case LIROp::FCmpGe: return CondCode::AE;
        default: return CondCode::E;
    }
}

void InstructionSelector::selectICmp(const LIRInstr& instr) {
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;
    VReg dst = instr.result;

    // Determine operand width from the types being compared (not result bool type)
    // We use 64 as default since the comparison applies to the operand types
    uint8_t w = 64;  // compare as 64-bit by default

    // xor dst, dst (before cmp to not clobber flags)
    emit(makeAlu(X86Op::Xor, MachOperand::virt(dst),
                 MachOperand::virt(dst), 64));

    // cmp lhs, rhs
    emit(makeCmp(MachOperand::virt(lhs), MachOperand::virt(rhs), w));

    // setcc dst
    emit(makeSetcc(mapICmpCC(instr.op), MachOperand::virt(dst)));
}

void InstructionSelector::selectFCmp(const LIRInstr& instr) {
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;
    VReg dst = instr.result;

    // xor dst first
    emit(makeAlu(X86Op::Xor, MachOperand::virt(dst),
                 MachOperand::virt(dst), 64));

    // ucomisd lhs, rhs
    MachInstr ucomi(X86Op::Ucomisd);
    ucomi.width = 64;  // TODO: check f32 vs f64
    ucomi.operand_count = 2;
    ucomi.inline_ops[0] = MachOperand::virt(lhs);
    ucomi.inline_ops[1] = MachOperand::virt(rhs);
    emit(ucomi);

    // setcc dst (unsigned comparison for float)
    emit(makeSetcc(mapFCmpCC(instr.op), MachOperand::virt(dst)));
}

// ============================================================================
// Unary Ops
// ============================================================================

void InstructionSelector::selectUnaryNeg(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg src = instr.unary.operand;
    VReg dst = instr.result;

    // mov dst, src; neg dst
    emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), w));

    MachInstr neg(X86Op::Neg);
    neg.width = w;
    neg.operand_count = 1;
    neg.inline_ops[0] = MachOperand::virt(dst);
    emit(neg);
}

void InstructionSelector::selectUnaryFNeg(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg src = instr.unary.operand;
    VReg dst = instr.result;

    // xorpd zero, zero; subsd dst, src  (0.0 - x = -x)
    VReg zero = freshVReg();
    X86Op xor_op = (w == 32) ? X86Op::Xorps : X86Op::Xorpd;
    MachInstr xz(xor_op);
    xz.width = w;
    xz.operand_count = 2;
    xz.inline_ops[0] = MachOperand::virt(zero);
    xz.inline_ops[1] = MachOperand::virt(zero);
    emit(xz);

    // dst = 0.0 - src
    X86Op mov_op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
    MachInstr mov(mov_op);
    mov.width = w;
    mov.operand_count = 2;
    mov.inline_ops[0] = MachOperand::virt(dst);
    mov.inline_ops[1] = MachOperand::virt(zero);
    emit(mov);

    X86Op sub_op = (w == 32) ? X86Op::Subss : X86Op::Subsd;
    MachInstr sub(sub_op);
    sub.width = w;
    sub.operand_count = 2;
    sub.inline_ops[0] = MachOperand::virt(dst);
    sub.inline_ops[1] = MachOperand::virt(src);
    emit(sub);
}

void InstructionSelector::selectUnaryNot(const LIRInstr& instr) {
    VReg src = instr.unary.operand;
    VReg dst = instr.result;

    // xor dst, dst; test src, src; sete dst
    emit(makeAlu(X86Op::Xor, MachOperand::virt(dst),
                 MachOperand::virt(dst), 64));

    MachInstr test(X86Op::Test);
    test.width = 8;
    test.operand_count = 2;
    test.inline_ops[0] = MachOperand::virt(src);
    test.inline_ops[1] = MachOperand::virt(src);
    emit(test);

    emit(makeSetcc(CondCode::E, MachOperand::virt(dst)));
}

// ============================================================================
// Memory Operations
// ============================================================================

void InstructionSelector::selectAddrOf(const LIRInstr& instr) {
    // lea dst, [src] — take address of a stack location
    VReg dst = instr.result;
    VReg src = instr.addr_of.source;
    emit(makeLea(MachOperand::virt(dst), MachOperand::virt(src)));
}

void InstructionSelector::selectLoad(const LIRInstr& instr) {
    // mov dst, [ptr]
    uint8_t w = widthOf(instr.type);
    VReg dst = instr.result;
    VReg ptr = instr.load.ptr;

    if (isFloat(instr.type)) {
        X86Op op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
        MachInstr mi(op);
        mi.width = w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(ptr);
        emit(mi);
    } else {
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(ptr), w));
    }
}

void InstructionSelector::selectStore(const LIRInstr& instr) {
    // mov [ptr], value
    uint8_t w = widthOf(instr.type);
    VReg ptr = instr.store.ptr;
    VReg val = instr.store.value;

    if (isFloat(instr.type)) {
        X86Op op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
        MachInstr mi(op);
        mi.width = w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(ptr);
        mi.inline_ops[1] = MachOperand::virt(val);
        emit(mi);
    } else {
        emit(makeMov(MachOperand::virt(ptr), MachOperand::virt(val), w));
    }
}

void InstructionSelector::selectFieldPtr(const LIRInstr& instr) {
    // lea dst, [base + offset]
    VReg dst = instr.result;
    VReg base = instr.field_ptr.base;
    uint32_t offset = instr.field_ptr.offset;

    if (offset == 0) {
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(base), 64));
    } else {
        // lea dst, [base + offset]
        // Represented as: mov dst, base; add dst, offset
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(base), 64));
        emit(makeAlu(X86Op::Add, MachOperand::virt(dst),
                     MachOperand::immediate(offset), 64));
    }
}

void InstructionSelector::selectStructAlloc(const LIRInstr& instr) {
    // sub rsp, size → lea dst, [rsp]
    // The actual stack allocation will be handled by RegisterAllocator
    // For now, represent as a pseudo stack alloc
    VReg dst = instr.result;
    uint32_t size = instr.struct_alloc.size;

    // Allocate stack space: sub rsp, size
    emit(makeAlu(X86Op::Sub,
                 MachOperand::precolored(PhysReg::RSP),
                 MachOperand::immediate(size), 64));

    // lea dst, [rsp]  — dst points to allocated space
    emit(makeLea(MachOperand::virt(dst),
                 MachOperand::precolored(PhysReg::RSP)));
}

// ============================================================================
// Control Flow
// ============================================================================

void InstructionSelector::selectBranch(const LIRInstr& instr,
                                        const LIRFunction& fn) {
    auto label = blockLabel(fn, instr.branch.target);
    emit(makeJmp(MachOperand::lbl(label)));
}

void InstructionSelector::selectCondBranch(const LIRInstr& instr,
                                            const LIRFunction& fn) {
    VReg cond = instr.cond_branch.cond;
    auto true_label = blockLabel(fn, instr.cond_branch.true_target);
    auto false_label = blockLabel(fn, instr.cond_branch.false_target);

    // test cond, cond; jnz true_label; jmp false_label
    MachInstr test(X86Op::Test);
    test.width = 8;
    test.operand_count = 2;
    test.inline_ops[0] = MachOperand::virt(cond);
    test.inline_ops[1] = MachOperand::virt(cond);
    emit(test);

    emit(makeJcc(CondCode::NE, MachOperand::lbl(true_label)));
    emit(makeJmp(MachOperand::lbl(false_label)));
}

void InstructionSelector::selectRet(const LIRInstr& instr) {
    VReg val = instr.ret.value;

    if (val != INVALID_VREG) {
        // mov rax, val (or movsd xmm0, val for float)
        // Note: the actual type check for float return would need
        // function return type info, but for simplicity we use GPR
        emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                     MachOperand::virt(val), 64));
    }

    // Epilogue will be inserted by RegisterAllocator/Backend
    emit(makeRet());
}

void InstructionSelector::selectCall(const LIRInstr& instr) {
    const auto& call = instr.call;

    // Set up arguments via parallel move
    // For now, generate individual moves (parallel move optimization later)
    uint32_t gpr_idx = 0;
    uint32_t xmm_idx = 0;

    for (uint32_t i = 0; i < call.arg_count; ++i) {
        // TODO: type-aware arg classification (float→xmm, struct→multi-reg)
        // For now: all args go to GPRs
        if (gpr_idx < MAX_GPR_ARGS) {
            emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[gpr_idx]),
                         MachOperand::virt(call.args[i]), 64));
            gpr_idx++;
        }
        // else: stack args (not yet handled)
    }
    (void)xmm_idx;

    // Build callee label: _name
    std::string_view callee_label;
    {
        char buf[256];
        int len = snprintf(buf, sizeof(buf), "_%.*s",
                           static_cast<int>(call.callee.size()),
                           call.callee.data());
        callee_label = ctx_.strings.intern(std::string_view(buf, len));
    }

    if (call.is_tail) {
        // Tail call: jmp instead of call
        emit(makeJmp(MachOperand::lbl(callee_label)));
    } else {
        emit(makeCall(MachOperand::lbl(callee_label)));
    }

    // Move return value
    if (instr.result != INVALID_VREG) {
        emit(makeMov(MachOperand::virt(instr.result),
                     MachOperand::precolored(PhysReg::RAX), 64));
    }
}

void InstructionSelector::selectBlockArg(const LIRInstr& instr) {
    // Block arguments are like phi nodes — they should have been
    // handled by the LIR→MachIR lowering via explicit moves.
    // For now, BlockArg is a no-op placeholder.
    (void)instr;
}

} // namespace kern
