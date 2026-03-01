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

uint32_t InstructionSelector::sizeOfType(TypeId type) const {
    if (type >= ctx_.types.size()) return 8;
    const auto& info = ctx_.types.get(type);
    if (info.kind == TypeKind::Struct) return info.struct_.size;
    if (info.kind == TypeKind::Union) {
        // Union: tag (8B) + max payload. Estimate conservatively.
        uint32_t max_payload = 0;
        for (uint32_t i = 0; i < info.union_.variant_count; ++i) {
            // Each variant with a payload adds up to 8 bytes
            if (info.union_.variants[i].payload_type != INVALID_TYPE)
                max_payload = 8;
        }
        return 8 + max_payload; // tag + max payload
    }
    if (info.kind == TypeKind::Primitive) {
        switch (info.primitive.prim) {
            case PrimitiveKind::Bool:
            case PrimitiveKind::I8:
            case PrimitiveKind::U8:  return 1;
            case PrimitiveKind::I16:
            case PrimitiveKind::U16: return 2;
            case PrimitiveKind::I32:
            case PrimitiveKind::U32:
            case PrimitiveKind::F32: return 4;
            default: return 8;
        }
    }
    return 8;
}

bool InstructionSelector::isStructType(TypeId type) const {
    if (type >= ctx_.types.size()) return false;
    const auto& info = ctx_.types.get(type);
    return info.kind == TypeKind::Struct || info.kind == TypeKind::Union;
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

    // Store globals for label lookup in selectGlobalRef
    globals_ = lir_mod.globals;
    global_count_ = lir_mod.global_count;

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
    mf->is_naked = fn.is_naked;
    mf->is_interrupt = fn.is_interrupt;
    mf->section_name = fn.section_name;
    mf->next_vreg = fn.next_vreg;
    next_vreg_ = fn.next_vreg;
    struct_alloc_bytes_ = 0;
    float_vregs_.clear();
    struct_vregs_.clear();
    struct_vreg_sizes_.clear();
    stack_ptr_vregs_.clear();
    gpr_arg_slot_ = 0;

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
    mf->struct_alloc_bytes = struct_alloc_bytes_;
    mf->next_vreg = next_vreg_;
    return mf;
}

// ============================================================================
// Instruction dispatch
// ============================================================================

void InstructionSelector::selectInstr(const LIRInstr& instr,
                                       const LIRFunction& fn) {
    // Track float vregs for call arg classification
    if (instr.result != INVALID_VREG && isFloat(instr.type)) {
        float_vregs_.insert(instr.result);
    }
    // Track struct vregs for multi-register ABI passing
    if (instr.result != INVALID_VREG && isStructType(instr.type)) {
        struct_vregs_.insert(instr.result);
        struct_vreg_sizes_[instr.result] = sizeOfType(instr.type);
    }

    switch (instr.op) {
        case LIROp::ConstInt:    selectConstInt(instr); break;
        case LIROp::ConstFloat:  selectConstFloat(instr); break;
        case LIROp::ConstBool:   selectConstBool(instr); break;
        case LIROp::ConstString: selectConstString(instr); break;
        case LIROp::GlobalRef:   selectGlobalRef(instr); break;

        case LIROp::Add:
        case LIROp::Sub:
        case LIROp::Mul:
        case LIROp::BAnd:
        case LIROp::BOr:
        case LIROp::BXor:       selectBinOp(instr); break;
        case LIROp::Shl:
        case LIROp::Shr:        selectShift(instr); break;
        case LIROp::Div:        selectDiv(instr, false); break;
        case LIROp::Mod:        selectDiv(instr, true); break;

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
        case LIROp::BNot:        selectUnaryBNot(instr); break;

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
        case LIROp::Cast:        selectCast(instr); break;
        case LIROp::InlineAsm:   selectInlineAsm(instr); break;
        case LIROp::CallIndirect: selectCallIndirect(instr); break;
        case LIROp::FnRef:       selectFnRef(instr); break;
        case LIROp::AtomicLoad:     selectAtomicLoad(instr); break;
        case LIROp::AtomicStore:    selectAtomicStore(instr); break;
        case LIROp::AtomicCas:      selectAtomicCas(instr); break;
        case LIROp::AtomicFetchAdd: selectAtomicFetchAdd(instr); break;
        case LIROp::Fence:          selectFence(instr); break;
        case LIROp::CompilerFence:  break;  // no instruction emitted
        case LIROp::PercpuLoad:     selectPercpuLoad(instr); break;
        case LIROp::PercpuStore:    selectPercpuStore(instr); break;
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
    // mov vreg, 0/1 (use 64-bit width for consistency with comparisons)
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::immediate(instr.const_bool.value ? 1 : 0), 64));
}

void InstructionSelector::selectConstFloat(const LIRInstr& instr) {
    // Handled same as GlobalRef — float constants are in .rodata
    // But ConstFloat with no global ref: materialize via mov to GPR then movq
    // In practice, LIRBuilder puts floats into globals. This is fallback.
    (void)instr;
    // No-op for now; LIRBuilder should always use GlobalRef for floats
}

void InstructionSelector::selectConstString(const LIRInstr& instr) {
    // String = fat pointer: [Ptr<u8> data (8B), u64 len (8B)] = 16 bytes on stack
    VReg dst = instr.result;
    uint32_t idx = instr.const_string.global_index;

    // Allocate 16 bytes on stack for the fat pointer
    struct_alloc_bytes_ += 16;
    static constexpr int32_t CALLEE_SAVED_AREA = NUM_CALLEE_SAVED * 8;
    int32_t base_offset = -CALLEE_SAVED_AREA - static_cast<int32_t>(struct_alloc_bytes_);

    // Get label for the string data
    std::string_view label;
    uint32_t length = 0;
    if (idx < global_count_) {
        label = globals_[idx].label;
        if (globals_[idx].kind == GlobalData::StringLit) {
            length = globals_[idx].string_lit.length;
        }
    }

    // lea tmp, [rel _str_N]  — data pointer
    VReg data_ptr = freshVReg();
    emit(makeLea(MachOperand::virt(data_ptr), MachOperand::lbl(label)));

    // Store data pointer at [base + 0]
    emit(makeMov(MachOperand::stack(base_offset), MachOperand::virt(data_ptr), 64));

    // Store length at [base + 8]
    VReg len_vreg = freshVReg();
    emit(makeMov(MachOperand::virt(len_vreg), MachOperand::immediate(length), 64));
    emit(makeMov(MachOperand::stack(base_offset + 8), MachOperand::virt(len_vreg), 64));

    // dst = pointer to the base of the fat pointer
    emit(makeLea(MachOperand::virt(dst), MachOperand::stack(base_offset)));
}

void InstructionSelector::selectGlobalRef(const LIRInstr& instr) {
    // Load float from .rodata via [rel label]
    uint8_t w = widthOf(instr.type);
    uint32_t idx = instr.global_ref.global_index;
    std::string_view label;
    if (idx < global_count_) {
        label = globals_[idx].label;
    }
    MachInstr mi(w == 32 ? X86Op::Movss : X86Op::Movsd);
    mi.width = w;
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::lbl(label);
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
        case LIROp::Add:  op = X86Op::Add; break;
        case LIROp::Sub:  op = X86Op::Sub; break;
        case LIROp::Mul:  op = X86Op::IMul; break;
        case LIROp::BAnd: op = X86Op::And; break;
        case LIROp::BOr:  op = X86Op::Or; break;
        case LIROp::BXor: op = X86Op::Xor; break;
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
// Shift Ops (need count in CL register)
// ============================================================================

void InstructionSelector::selectShift(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg dst = instr.result;
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;

    // x86 pattern: mov dst, lhs; mov cl, rhs; shl/shr dst, cl
    emit(makeMov(MachOperand::virt(dst), MachOperand::virt(lhs), w));

    // Move shift count to CL (RCX low byte)
    emit(makeMov(MachOperand::precolored(PhysReg::RCX),
                 MachOperand::virt(rhs), w));

    // Determine signed vs unsigned for right shift
    X86Op op;
    if (instr.op == LIROp::Shl) {
        op = X86Op::Shl;
    } else {
        // Shr: use SAR for signed types, SHR for unsigned
        bool is_signed = true;
        if (instr.type < ctx_.types.size()) {
            const auto& info = ctx_.types.get(instr.type);
            if (info.kind == TypeKind::Primitive) {
                auto p = info.primitive.prim;
                is_signed = !(p == PrimitiveKind::U8 || p == PrimitiveKind::U16 ||
                              p == PrimitiveKind::U32 || p == PrimitiveKind::U64);
            }
        }
        op = is_signed ? X86Op::Sar : X86Op::Shr;
    }

    // Shift uses CL as implicit second operand
    emit(makeAlu(op, MachOperand::virt(dst),
                 MachOperand::precolored(PhysReg::RCX), w));
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

void InstructionSelector::selectUnaryBNot(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg src = instr.unary.operand;
    VReg dst = instr.result;

    // mov dst, src; not dst
    emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), w));

    MachInstr not_instr(X86Op::Not);
    not_instr.width = w;
    not_instr.operand_count = 1;
    not_instr.inline_ops[0] = MachOperand::virt(dst);
    emit(not_instr);
}

// ============================================================================
// Memory Operations
// ============================================================================

void InstructionSelector::selectAddrOf(const LIRInstr& instr) {
    // addr_of needs the source on the stack.
    // If the source is a struct_alloc result, its vreg already holds a stack ptr — use it.
    // Otherwise, allocate a stack slot, store the value, and lea from that slot.
    VReg dst = instr.result;
    VReg src = instr.addr_of.source;

    // Allocate an 8-byte stack slot for the source value
    struct_alloc_bytes_ += 8;
    static constexpr int32_t CALLEE_SAVED_AREA = NUM_CALLEE_SAVED * 8;
    int32_t slot = -CALLEE_SAVED_AREA - static_cast<int32_t>(struct_alloc_bytes_);

    // Store value to stack slot
    emit(makeMov(MachOperand::stack(slot), MachOperand::virt(src), 64));
    // LEA dst, [rbp + slot]
    emit(makeLea(MachOperand::virt(dst), MachOperand::stack(slot)));
}

void InstructionSelector::selectLoad(const LIRInstr& instr) {
    // mov dst, [ptr]  — load from memory at ptr
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
        MachInstr mi(X86Op::MovLoad);
        mi.width = w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(ptr);
        emit(mi);
    }
}

void InstructionSelector::selectStore(const LIRInstr& instr) {
    // mov [ptr], value  — store to memory at ptr
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
        MachInstr mi(X86Op::MovStore);
        mi.width = w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(ptr);
        mi.inline_ops[1] = MachOperand::virt(val);
        emit(mi);
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
    // Allocate space in the function's stack frame (tracked via struct_alloc_bytes_).
    // The RegisterAllocator adds struct_alloc_bytes_ to fn.stack_size so the
    // prologue/epilogue sub/add rsp accounts for this space.
    VReg dst = instr.result;
    uint32_t size = instr.struct_alloc.size;

    // Align size to 8 bytes
    size = (size + 7u) & ~7u;
    struct_alloc_bytes_ += size;

    // lea dst, [rbp - callee_saved_area - offset]
    // Reserve 40 bytes for callee-saved pushes (5 regs × 8 bytes)
    static constexpr int32_t CALLEE_SAVED_AREA = NUM_CALLEE_SAVED * 8;  // 40
    int32_t offset = -CALLEE_SAVED_AREA - static_cast<int32_t>(struct_alloc_bytes_);
    emit(makeLea(MachOperand::virt(dst),
                 MachOperand::stack(offset)));
    stack_ptr_vregs_.insert(dst);
}

// ============================================================================
// Control Flow
// ============================================================================

void InstructionSelector::selectBranch(const LIRInstr& instr,
                                        const LIRFunction& fn) {
    // If branch has arguments, emit moves to the target block's BlockArg vregs
    if (instr.branch.arg_count > 0) {
        uint32_t target_bb = instr.branch.target;
        const auto& target_block = fn.blocks[target_bb];
        // Find BlockArg instructions in target block to get their result vregs
        std::vector<VReg> param_vregs;
        for (uint32_t i = 0; i < target_block.instr_count; ++i) {
            if (target_block.instrs[i].op == LIROp::BlockArg) {
                param_vregs.push_back(target_block.instrs[i].result);
            } else {
                break; // BlockArgs are always at the start of a block
            }
        }
        // Emit moves: branch arg[i] → target block_arg vreg[i]
        uint32_t count = std::min(instr.branch.arg_count,
                                  static_cast<uint32_t>(param_vregs.size()));
        for (uint32_t i = 0; i < count; ++i) {
            emit(makeMov(MachOperand::virt(param_vregs[i]),
                         MachOperand::virt(instr.branch.args[i]), 64));
        }
    }
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
        if (float_vregs_.count(val)) {
            // movsd xmm0, val
            MachInstr mi(X86Op::Movsd);
            mi.width = 64;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::precolored(PhysReg::XMM0);
            mi.inline_ops[1] = MachOperand::virt(val);
            emit(mi);
        } else {
            emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                         MachOperand::virt(val), 64));
        }
    }

    emit(makeRet());
}

void InstructionSelector::selectCall(const LIRInstr& instr) {
    const auto& call = instr.call;

    // Set up arguments via parallel move
    uint32_t gpr_idx = 0;
    uint32_t xmm_idx = 0;

    for (uint32_t i = 0; i < call.arg_count; ++i) {
        VReg arg = call.args[i];
        if (float_vregs_.count(arg)) {
            // Float arg → XMM register
            if (xmm_idx < MAX_XMM_ARGS) {
                X86Op mov_op = X86Op::Movsd;
                MachInstr mi(mov_op);
                mi.width = 64;
                mi.operand_count = 2;
                mi.inline_ops[0] = MachOperand::precolored(XMM_ARG_REGS[xmm_idx]);
                mi.inline_ops[1] = MachOperand::virt(arg);
                emit(mi);
                xmm_idx++;
            }
        } else if (struct_vregs_.count(arg)) {
            // Struct arg: vreg holds a pointer to the struct data on stack.
            // Load fields into consecutive GPR args.
            uint32_t size = struct_vreg_sizes_[arg];
            uint32_t num_regs = (size <= 8) ? 1 : 2;
            for (uint32_t r = 0; r < num_regs && gpr_idx < MAX_GPR_ARGS; ++r) {
                // Load 8 bytes from [struct_base + r*8]
                VReg field_ptr = freshVReg();
                if (r == 0) {
                    emit(makeMov(MachOperand::virt(field_ptr), MachOperand::virt(arg), 64));
                } else {
                    emit(makeMov(MachOperand::virt(field_ptr), MachOperand::virt(arg), 64));
                    emit(makeAlu(X86Op::Add, MachOperand::virt(field_ptr),
                                 MachOperand::immediate(r * 8), 64));
                }
                // MovLoad: mov gpr, [field_ptr]
                MachInstr load(X86Op::MovLoad);
                load.width = 64;
                load.operand_count = 2;
                load.inline_ops[0] = MachOperand::precolored(GPR_ARG_REGS[gpr_idx]);
                load.inline_ops[1] = MachOperand::virt(field_ptr);
                emit(load);
                gpr_idx++;
            }
        } else {
            // Integer/pointer arg → GPR
            if (gpr_idx < MAX_GPR_ARGS) {
                emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[gpr_idx]),
                             MachOperand::virt(arg), 64));
                gpr_idx++;
            }
        }
    }

    // Build callee label: _name
    std::string_view callee_label;
    {
        char buf[256];
        int len = snprintf(buf, sizeof(buf), "_%.*s",
                           static_cast<int>(call.callee.size()),
                           call.callee.data());
        callee_label = ctx_.strings.intern(std::string_view(buf, len));
    }

    // Can't tail-call if any arg is a stack pointer (struct_alloc/addr_of)
    // because the callee would access the caller's (destroyed) stack frame.
    bool can_tail = call.is_tail;
    if (can_tail) {
        for (uint32_t i = 0; i < call.arg_count; ++i) {
            if (stack_ptr_vregs_.count(call.args[i]) ||
                struct_vregs_.count(call.args[i])) {
                can_tail = false;
                break;
            }
        }
    }

    if (can_tail) {
        MachInstr frame_destroy(X86Op::Pseudo_FrameDestroy);
        emit(frame_destroy);
        emit(makeJmp(MachOperand::lbl(callee_label)));
    } else {
        emit(makeCall(MachOperand::lbl(callee_label)));
    }

    // Move return value
    if (instr.result != INVALID_VREG) {
        if (float_vregs_.count(instr.result)) {
            MachInstr mi(X86Op::Movsd);
            mi.width = 64;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::virt(instr.result);
            mi.inline_ops[1] = MachOperand::precolored(PhysReg::XMM0);
            emit(mi);
        } else {
            emit(makeMov(MachOperand::virt(instr.result),
                         MachOperand::precolored(PhysReg::RAX), 64));
        }
    }
}

void InstructionSelector::selectFnRef(const LIRInstr& instr) {
    // lea vreg, [_fnname]  — get address of function
    char buf[256];
    int len = snprintf(buf, sizeof(buf), "_%.*s",
                       static_cast<int>(instr.fn_ref.fn_name.size()),
                       instr.fn_ref.fn_name.data());
    auto callee_label = ctx_.strings.intern(std::string_view(buf, len));
    emit(makeLea(MachOperand::virt(instr.result),
                 MachOperand::lbl(callee_label), 64));
}

void InstructionSelector::selectCallIndirect(const LIRInstr& instr) {
    const auto& ci = instr.call_indirect;

    // Set up arguments via parallel move (same as direct call)
    uint32_t gpr_idx = 0;
    uint32_t xmm_idx = 0;

    for (uint32_t i = 0; i < ci.arg_count; ++i) {
        VReg arg = ci.args[i];
        if (float_vregs_.count(arg)) {
            if (xmm_idx < MAX_XMM_ARGS) {
                MachInstr mi(X86Op::Movsd);
                mi.width = 64;
                mi.operand_count = 2;
                mi.inline_ops[0] = MachOperand::precolored(XMM_ARG_REGS[xmm_idx]);
                mi.inline_ops[1] = MachOperand::virt(arg);
                emit(mi);
                xmm_idx++;
            }
        } else if (struct_vregs_.count(arg)) {
            uint32_t size = struct_vreg_sizes_[arg];
            uint32_t num_regs = (size <= 8) ? 1 : 2;
            for (uint32_t r = 0; r < num_regs && gpr_idx < MAX_GPR_ARGS; ++r) {
                VReg field_ptr = freshVReg();
                if (r == 0) {
                    emit(makeMov(MachOperand::virt(field_ptr), MachOperand::virt(arg), 64));
                } else {
                    emit(makeMov(MachOperand::virt(field_ptr), MachOperand::virt(arg), 64));
                    emit(makeAlu(X86Op::Add, MachOperand::virt(field_ptr),
                                 MachOperand::immediate(r * 8), 64));
                }
                MachInstr load(X86Op::MovLoad);
                load.width = 64;
                load.operand_count = 2;
                load.inline_ops[0] = MachOperand::precolored(GPR_ARG_REGS[gpr_idx]);
                load.inline_ops[1] = MachOperand::virt(field_ptr);
                emit(load);
                gpr_idx++;
            }
        } else {
            if (gpr_idx < MAX_GPR_ARGS) {
                emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[gpr_idx]),
                             MachOperand::virt(arg), 64));
                gpr_idx++;
            }
        }
    }

    // Indirect call: call [callee_vreg]
    emit(makeCall(MachOperand::virt(ci.callee)));

    // Move return value
    if (instr.result != INVALID_VREG) {
        if (float_vregs_.count(instr.result)) {
            MachInstr mi(X86Op::Movsd);
            mi.width = 64;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::virt(instr.result);
            mi.inline_ops[1] = MachOperand::precolored(PhysReg::XMM0);
            emit(mi);
        } else {
            emit(makeMov(MachOperand::virt(instr.result),
                         MachOperand::precolored(PhysReg::RAX), 64));
        }
    }
}

void InstructionSelector::selectBlockArg(const LIRInstr& instr) {
    // In the entry block (block 0), block args are function parameters
    // arriving via ABI registers. Use gpr_arg_slot_ to track cumulative
    // GPR slot usage (multi-register params like 16B structs consume 2 slots).
    if (current_block_ == 0) {
        bool is_float_param = isFloat(instr.type);
        bool is_struct = isStructType(instr.type);

        if (is_struct) {
            uint32_t size = sizeOfType(instr.type);
            uint32_t num_regs = (size <= 8) ? 1 : 2;

            struct_alloc_bytes_ += (size + 7u) & ~7u;
            static constexpr int32_t CS_AREA = NUM_CALLEE_SAVED * 8;
            int32_t base_offset = -CS_AREA - static_cast<int32_t>(struct_alloc_bytes_);

            for (uint32_t r = 0; r < num_regs && gpr_arg_slot_ < MAX_GPR_ARGS; ++r) {
                emit(makeMov(MachOperand::stack(base_offset + r * 8),
                             MachOperand::precolored(GPR_ARG_REGS[gpr_arg_slot_]), 64));
                gpr_arg_slot_++;
            }

            emit(makeLea(MachOperand::virt(instr.result),
                         MachOperand::stack(base_offset)));
        } else if (is_float_param) {
            // Float params use separate XMM counter (not tracked in gpr_arg_slot_)
            // For now, use block_arg.index for XMM since we don't have mixed tracking
            uint32_t xmm_idx = instr.block_arg.index;  // TODO: separate XMM counter
            if (xmm_idx < MAX_XMM_ARGS) {
                uint8_t w = widthOf(instr.type);
                X86Op mov_op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
                MachInstr mi(mov_op);
                mi.width = w;
                mi.operand_count = 2;
                mi.inline_ops[0] = MachOperand::virt(instr.result);
                mi.inline_ops[1] = MachOperand::precolored(XMM_ARG_REGS[xmm_idx]);
                emit(mi);
            }
        } else {
            if (gpr_arg_slot_ < MAX_GPR_ARGS) {
                uint8_t w = widthOf(instr.type);
                emit(makeMov(MachOperand::virt(instr.result),
                             MachOperand::precolored(GPR_ARG_REGS[gpr_arg_slot_]), w));
                gpr_arg_slot_++;
            }
        }
        return;
    }
    // Non-entry block args are handled by predecessor branch moves (no-op here).
}

void InstructionSelector::selectCast(const LIRInstr& instr) {
    VReg src = instr.cast.operand;
    VReg dst = instr.result;
    TypeId src_type = instr.cast.src_type;
    TypeId dst_type = instr.type;

    uint8_t src_w = widthOf(src_type);
    uint8_t dst_w = widthOf(dst_type);

    // Ptr<->int: both are 64-bit, just mov
    auto src_kind = (src_type < ctx_.types.size()) ? ctx_.types.get(src_type).kind : TypeKind::Primitive;
    auto dst_kind = (dst_type < ctx_.types.size()) ? ctx_.types.get(dst_type).kind : TypeKind::Primitive;
    if (src_kind == TypeKind::Ptr || src_kind == TypeKind::PtrMut ||
        dst_kind == TypeKind::Ptr || dst_kind == TypeKind::PtrMut) {
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
        return;
    }

    if (dst_w > src_w) {
        // Widening: zero-extend or sign-extend
        bool is_signed = ctx_.types.isSigned(src_type);
        if (!is_signed) {
            // Zero-extend: mov + mask
            emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
            int64_t mask = (src_w == 8) ? 0xFF :
                           (src_w == 16) ? 0xFFFF :
                           (src_w == 32) ? 0xFFFFFFFF : -1;
            if (mask != -1) {
                emit(makeAlu(X86Op::And, MachOperand::virt(dst),
                             MachOperand::immediate(mask), 64));
            }
        } else {
            // Sign-extend: shl + sar to propagate sign bit
            emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
            uint8_t shift = 64 - src_w;
            emit(makeMov(MachOperand::precolored(PhysReg::RCX),
                         MachOperand::immediate(shift), 64));
            emit(makeAlu(X86Op::Shl, MachOperand::virt(dst),
                         MachOperand::precolored(PhysReg::RCX), 64));
            emit(makeMov(MachOperand::precolored(PhysReg::RCX),
                         MachOperand::immediate(shift), 64));
            emit(makeAlu(X86Op::Sar, MachOperand::virt(dst),
                         MachOperand::precolored(PhysReg::RCX), 64));
        }
    } else if (dst_w < src_w) {
        // Narrowing: mov + mask to truncate
        // mov dst, src; and dst, mask
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
        int64_t mask = (dst_w == 8) ? 0xFF :
                       (dst_w == 16) ? 0xFFFF :
                       (dst_w == 32) ? 0xFFFFFFFF : -1;
        emit(makeAlu(X86Op::And, MachOperand::virt(dst),
                     MachOperand::immediate(mask), 64));
    } else {
        // Same width: just mov
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
    }
}

void InstructionSelector::selectInlineAsm(const LIRInstr& instr) {
    // Emit raw assembly lines as InlineAsm MachInstr
    MachInstr mi(X86Op::InlineAsm);
    mi.operand_count = 0;
    mi.asm_data.lines = instr.inline_asm.lines;
    mi.asm_data.line_lengths = instr.inline_asm.line_lengths;
    mi.asm_data.line_count = instr.inline_asm.line_count;
    emit(mi);
}

// ============================================================================
// Atomic operations
// ============================================================================

void InstructionSelector::selectAtomicLoad(const LIRInstr& instr) {
    // Atomic load: mov dst, [ptr] (x86 aligned loads are naturally atomic)
    MachInstr mi(X86Op::MovLoad);
    mi.width = widthOf(instr.type);
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.atomic_load.ptr);
    emit(mi);

    // For SeqCst: add mfence after
    if (instr.atomic_load.order == MemOrder::SeqCst) {
        MachInstr fence(X86Op::Mfence);
        fence.operand_count = 0;
        emit(fence);
    }
}

void InstructionSelector::selectAtomicStore(const LIRInstr& instr) {
    // For SeqCst: use xchg (implicitly locked). Otherwise: mov [ptr], value
    if (instr.atomic_store.order == MemOrder::SeqCst) {
        MachInstr mi(X86Op::Xchg);
        mi.width = 64;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(instr.atomic_store.ptr);
        mi.inline_ops[1] = MachOperand::virt(instr.atomic_store.value);
        emit(mi);
    } else {
        MachInstr mi(X86Op::MovStore);
        mi.width = 64;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(instr.atomic_store.ptr);
        mi.inline_ops[1] = MachOperand::virt(instr.atomic_store.value);
        emit(mi);
    }
}

void InstructionSelector::selectAtomicCas(const LIRInstr& instr) {
    // lock cmpxchg [ptr], desired
    // Move expected → result vreg (will become rax)
    MachInstr mov_exp(X86Op::Mov);
    mov_exp.width = 64;
    mov_exp.operand_count = 2;
    mov_exp.inline_ops[0] = MachOperand::virt(instr.result);
    mov_exp.inline_ops[1] = MachOperand::virt(instr.atomic_cas.expected);
    emit(mov_exp);

    MachInstr mi(X86Op::LockCmpxchg);
    mi.width = 64;
    mi.operand_count = 3;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.atomic_cas.ptr);
    mi.inline_ops[2] = MachOperand::virt(instr.atomic_cas.desired);
    emit(mi);
}

void InstructionSelector::selectAtomicFetchAdd(const LIRInstr& instr) {
    // lock xadd [ptr], value → returns old value
    MachInstr mi(X86Op::LockXadd);
    mi.width = 64;
    mi.operand_count = 3;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.atomic_fetch_add.ptr);
    mi.inline_ops[2] = MachOperand::virt(instr.atomic_fetch_add.value);
    emit(mi);
}

void InstructionSelector::selectFence(const LIRInstr& instr) {
    X86Op fence_op;
    switch (instr.fence.order) {
        case MemOrder::SeqCst:
        case MemOrder::AcqRel:
            fence_op = X86Op::Mfence;
            break;
        case MemOrder::Acquire:
            fence_op = X86Op::Lfence;
            break;
        case MemOrder::Release:
            fence_op = X86Op::Sfence;
            break;
        case MemOrder::Relaxed:
            return;  // no fence needed
    }
    MachInstr mi(fence_op);
    mi.operand_count = 0;
    emit(mi);
}

void InstructionSelector::selectPercpuLoad(const LIRInstr& instr) {
    MachInstr mi(X86Op::GsLoad);
    mi.width = widthOf(instr.type);
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.percpu_load.offset);
    emit(mi);
}

void InstructionSelector::selectPercpuStore(const LIRInstr& instr) {
    MachInstr mi(X86Op::GsStore);
    mi.width = 64;
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.percpu_store.offset);
    mi.inline_ops[1] = MachOperand::virt(instr.percpu_store.value);
    emit(mi);
}

} // namespace kern
