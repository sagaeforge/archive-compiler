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
    if (info.kind == TypeKind::Enum) {
        return static_cast<uint8_t>(info.enum_.backing_size * 8);
    }
    return 64;  // pointers, structs, etc.
}

uint32_t InstructionSelector::sizeOfType(TypeId type) const {
    if (type >= ctx_.types.size()) return 8;
    const auto& info = ctx_.types.get(type);
    if (info.kind == TypeKind::Struct) return info.struct_.size;
    if (info.kind == TypeKind::DynTrait) return 16;  // fat pointer: data_ptr + vtable_ptr
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
    return info.kind == TypeKind::Struct || info.kind == TypeKind::Union ||
           info.kind == TypeKind::DynTrait;
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
    mod->module_name = lir_mod.module_name;
    module_name_ = lir_mod.module_name;
    mod->fn_count = lir_mod.fn_count;
    mod->functions = ctx_.arena.makeArray<MachFunction>(lir_mod.fn_count);

    // Store globals for label lookup in selectGlobalRef
    globals_ = lir_mod.globals;
    global_count_ = lir_mod.global_count;

    extern_labels_.clear();
    link_names_.clear();

    // Pre-scan for @link_name mappings
    // Also collect names of functions with definitions (have blocks)
    std::unordered_set<std::string_view> defined_fns;
    for (uint32_t i = 0; i < lir_mod.fn_count; ++i) {
        if (!lir_mod.functions[i].link_name.empty()) {
            link_names_[lir_mod.functions[i].name] = lir_mod.functions[i].link_name;
        }
        if (lir_mod.functions[i].block_count > 0) {
            defined_fns.insert(lir_mod.functions[i].name);
        }
    }

    // Track extern fn declarations as extern labels for the linker
    // (only if not also defined in this module)
    for (uint32_t i = 0; i < lir_mod.fn_count; ++i) {
        if (lir_mod.functions[i].is_extern && lir_mod.functions[i].block_count == 0
            && !defined_fns.count(lir_mod.functions[i].name)) {
            const char* sp = symPrefix();
            auto name = lir_mod.functions[i].link_name.empty()
                        ? lir_mod.functions[i].name
                        : lir_mod.functions[i].link_name;
            char buf[512];
            int len = snprintf(buf, sizeof(buf), "%s%.*s", sp,
                               static_cast<int>(name.size()), name.data());
            extern_labels_.push_back(ctx_.strings.intern(std::string_view(buf, len)));
        }
    }

    for (uint32_t i = 0; i < lir_mod.fn_count; ++i) {
        auto* mf = selectFunction(lir_mod.functions[i]);
        mod->functions[i] = *mf;
    }

    // Copy cross-module extern labels to module
    if (!extern_labels_.empty()) {
        mod->extern_label_count = static_cast<uint32_t>(extern_labels_.size());
        mod->extern_labels = ctx_.arena.makeArray<std::string_view>(mod->extern_label_count);
        for (uint32_t i = 0; i < mod->extern_label_count; ++i) {
            mod->extern_labels[i] = extern_labels_[i];
        }
    }

    return mod;
}

MachFunction* InstructionSelector::selectFunction(const LIRFunction& fn) {
    auto* mf = ctx_.arena.make<MachFunction>();
    mf->name = fn.name;
    mf->is_intrinsic = fn.is_intrinsic;
    mf->is_naked = fn.is_naked;
    mf->is_interrupt = fn.is_interrupt;
    mf->is_inline = fn.is_inline;
    mf->is_noinline = fn.is_noinline;
    mf->is_noreturn = fn.is_noreturn;
    mf->is_pub = fn.is_pub;
    mf->is_extern = fn.is_extern;
    mf->is_weak = fn.is_weak;
    mf->section_name = fn.section_name;
    mf->link_name = fn.link_name;
    mf->next_vreg = fn.next_vreg;
    next_vreg_ = fn.next_vreg;
    struct_alloc_bytes_ = 0;
    float_vregs_.clear();
    struct_vregs_.clear();
    struct_vreg_sizes_.clear();
    stack_ptr_vregs_.clear();
    struct_alloc_vregs_.clear();
    gpr_arg_slot_ = 0;
    xmm_arg_slot_ = 0;
    next_param_idx_ = 0;
    fn_return_type_ = fn.return_type;
    hidden_ret_ptr_ = INVALID_VREG;

    // >16B struct return: the caller passes a hidden pointer in RDI.
    // We allocate a vreg to hold it and shift gpr_arg_slot_ so the first
    // visible parameter starts from RSI.
    if (isStructType(fn.return_type) && sizeOfType(fn.return_type) > 16) {
        hidden_ret_ptr_ = freshVReg();
    }

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

        // >16B struct return: capture hidden return pointer from RDI at the
        // very start of the entry block.  Previously this was done inside
        // selectBlockArg, but functions with zero parameters have no
        // BlockArg instructions, leaving hidden_ret_ptr_ uninitialised.
        if (b == 0 && hidden_ret_ptr_ != INVALID_VREG && gpr_arg_slot_ == 0) {
            emit(makeMov(MachOperand::virt(hidden_ret_ptr_),
                         MachOperand::precolored(GPR_ARG_REGS[0]), 64));
            gpr_arg_slot_ = 1;  // visible params start from RSI
        }

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
        float_vregs_[instr.result] = widthOf(instr.type);
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
        case LIROp::AddWrap:
        case LIROp::SubWrap:
        case LIROp::MulWrap:
        case LIROp::BAnd:
        case LIROp::BOr:
        case LIROp::BXor:       selectBinOp(instr); break;
        case LIROp::AddSat:
        case LIROp::SubSat:     selectSaturatingOp(instr); break;
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
        case LIROp::BlockArg:    selectBlockArg(instr, fn); break;
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
        case LIROp::LoadGlobal:     selectLoadGlobal(instr); break;
        case LIROp::StoreGlobal:    selectStoreGlobal(instr); break;
        case LIROp::Clz:            selectClz(instr); break;
        case LIROp::Ctz:            selectCtz(instr); break;
        case LIROp::Popcnt:         selectPopcnt(instr); break;
        case LIROp::Bswap:          selectBswap(instr); break;
        case LIROp::PortIn:         selectPortIn(instr); break;
        case LIROp::PortOut:        selectPortOut(instr); break;
    }
}

// ============================================================================
// Constants
// ============================================================================

void InstructionSelector::selectConstInt(const LIRInstr& instr) {
    // mov vreg, imm — always use 64-bit to fully initialize register.
    // Sub-64-bit constants have correct values in the low bits and zeros above.
    uint8_t w = widthOf(instr.type);
    if (w == 0) return;  // Unit
    // Mask the immediate for unsigned sub-64-bit types to avoid sign-extension issues
    int64_t imm = instr.const_int.value;
    if (w < 64) {
        // Zero-extend: mask to the correct bit width
        uint64_t mask = (1ULL << w) - 1;
        imm = static_cast<int64_t>(static_cast<uint64_t>(imm) & mask);
    }
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::immediate(imm), 64));
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
        case LIROp::Add:     op = X86Op::Add; break;
        case LIROp::Sub:     op = X86Op::Sub; break;
        case LIROp::Mul:     op = X86Op::IMul; break;
        case LIROp::AddWrap: op = X86Op::Add; break;
        case LIROp::SubWrap: op = X86Op::Sub; break;
        case LIROp::MulWrap: op = X86Op::IMul; break;
        case LIROp::BAnd: op = X86Op::And; break;
        case LIROp::BOr:  op = X86Op::Or; break;
        case LIROp::BXor: op = X86Op::Xor; break;
        default: op = X86Op::Add; break;
    }

    emit(makeAlu(op, MachOperand::virt(dst), MachOperand::virt(rhs), w));
}

void InstructionSelector::selectSaturatingOp(const LIRInstr& instr) {
    uint8_t w = widthOf(instr.type);
    VReg dst = instr.result;
    VReg lhs = instr.bin.lhs;
    VReg rhs = instr.bin.rhs;

    // Check if unsigned
    bool is_unsigned = false;
    if (instr.type < ctx_.types.size()) {
        const auto& info = ctx_.types.get(instr.type);
        if (info.kind == TypeKind::Primitive) {
            auto p = info.primitive.prim;
            is_unsigned = (p == PrimitiveKind::U8 || p == PrimitiveKind::U16 ||
                           p == PrimitiveKind::U32 || p == PrimitiveKind::U64);
        }
    }

    // mov dst, lhs
    emit(makeMov(MachOperand::virt(dst), MachOperand::virt(lhs), w));

    // add/sub dst, rhs
    X86Op arith_op = (instr.op == LIROp::AddSat) ? X86Op::Add : X86Op::Sub;
    emit(makeAlu(arith_op, MachOperand::virt(dst), MachOperand::virt(rhs), w));

    // Compute saturated value in a temp register, then cmov on overflow
    VReg sat_tmp = freshVReg();

    if (is_unsigned) {
        if (instr.op == LIROp::AddSat) {
            // Unsigned add overflow: carry flag set → clamp to UINT_MAX
            emit(makeMov(MachOperand::virt(sat_tmp), MachOperand::immediate(-1), w));
            MachInstr cmov(X86Op::Cmovcc);
            cmov.cc = CondCode::B;
            cmov.width = w;
            cmov.operand_count = 2;
            cmov.operand(0) = MachOperand::virt(dst);
            cmov.operand(1) = MachOperand::virt(sat_tmp);
            emit(cmov);
        } else {
            // Unsigned sub underflow: borrow → clamp to 0
            emit(makeMov(MachOperand::virt(sat_tmp), MachOperand::immediate(0), w));
            MachInstr cmov(X86Op::Cmovcc);
            cmov.cc = CondCode::B;
            cmov.width = w;
            cmov.operand_count = 2;
            cmov.operand(0) = MachOperand::virt(dst);
            cmov.operand(1) = MachOperand::virt(sat_tmp);
            emit(cmov);
        }
    } else {
        // Signed overflow: use sign of result to determine which limit
        //   overflow with negative result → positive overflow → INT_MAX
        //   overflow with positive result → negative overflow → INT_MIN
        int64_t max_val = 0, min_val = 0;
        switch (w) {
            case 8:  max_val = 0x7F;        min_val = -0x80;         break;
            case 16: max_val = 0x7FFF;      min_val = -0x8000;       break;
            case 32: max_val = 0x7FFFFFFF;  min_val = -0x80000000LL; break;
            default: max_val = 0x7FFFFFFFFFFFFFFFLL;
                     min_val = static_cast<int64_t>(0x8000000000000000ULL); break;
        }

        // Load INT_MAX into sat_tmp
        emit(makeMov(MachOperand::virt(sat_tmp), MachOperand::immediate(max_val), w));

        // If result is non-negative (SF=0), we overflowed negative → want INT_MIN
        VReg min_tmp = freshVReg();
        emit(makeMov(MachOperand::virt(min_tmp), MachOperand::immediate(min_val), w));

        MachInstr cmov_sign(X86Op::Cmovcc);
        cmov_sign.cc = CondCode::GE;  // SF=0 → result non-negative
        cmov_sign.width = w;
        cmov_sign.operand_count = 2;
        cmov_sign.operand(0) = MachOperand::virt(sat_tmp);
        cmov_sign.operand(1) = MachOperand::virt(min_tmp);
        emit(cmov_sign);

        // cmovo dst, sat_tmp (if overflow, replace with saturated value)
        MachInstr cmov_ov(X86Op::Cmovcc);
        cmov_ov.cc = CondCode::O;
        cmov_ov.width = w;
        cmov_ov.operand_count = 2;
        cmov_ov.operand(0) = MachOperand::virt(dst);
        cmov_ov.operand(1) = MachOperand::virt(sat_tmp);
        emit(cmov_ov);
    }
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

    // ucomiss/ucomisd lhs, rhs
    uint8_t fw = 64;
    auto it = float_vregs_.find(lhs);
    if (it != float_vregs_.end()) fw = it->second;
    X86Op ucomi_op = (fw == 32) ? X86Op::Ucomiss : X86Op::Ucomisd;
    MachInstr ucomi(ucomi_op);
    ucomi.width = fw;
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
        // Float load from pointer: movss/movsd xmm, [gpr]
        MachInstr mi(X86Op::FloatLoad);
        mi.width = w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(ptr);
        mi.is_volatile = instr.load.is_volatile;
        emit(mi);
    } else if (isStructType(instr.type) && !struct_alloc_vregs_.count(ptr)) {
        // Struct dereference: ptr points to actual struct data in memory.
        // Allocate stack space and copy the full struct from [ptr] to stack.
        // Result vreg holds a pointer to the stack copy (like StructAlloc).
        // Note: if ptr IS a stack_ptr_vreg (e.g. var slot), it holds an 8-byte
        // pointer — fall through to the scalar load path.
        uint32_t sz = sizeOfType(instr.type);
        sz = (sz + 7u) & ~7u;
        struct_alloc_bytes_ += sz;
        static constexpr int32_t CALLEE_SAVED_AREA = NUM_CALLEE_SAVED * 8;
        int32_t slot = -CALLEE_SAVED_AREA - static_cast<int32_t>(struct_alloc_bytes_);

        // LEA dst, [rbp + slot]  — dst = pointer to stack copy
        emit(makeLea(MachOperand::virt(dst), MachOperand::stack(slot)));
        stack_ptr_vregs_.insert(dst);

        // Copy data from [ptr] to [dst] in 8-byte chunks
        VReg src_ptr = freshVReg();
        VReg dst_ptr = freshVReg();
        emit(makeMov(MachOperand::virt(src_ptr), MachOperand::virt(ptr), 64));
        emit(makeMov(MachOperand::virt(dst_ptr), MachOperand::virt(dst), 64));
        for (uint32_t off = 0; off < sz; off += 8) {
            VReg tmp = freshVReg();
            MachInstr ld(X86Op::MovLoad);
            ld.width = 64;
            ld.operand_count = 2;
            ld.inline_ops[0] = MachOperand::virt(tmp);
            ld.inline_ops[1] = MachOperand::virt(src_ptr);
            emit(ld);
            MachInstr st(X86Op::MovStore);
            st.width = 64;
            st.operand_count = 2;
            st.inline_ops[0] = MachOperand::virt(dst_ptr);
            st.inline_ops[1] = MachOperand::virt(tmp);
            emit(st);
            if (off + 8 < sz) {
                emit(makeAlu(X86Op::Add, MachOperand::virt(src_ptr),
                             MachOperand::immediate(8), 64));
                emit(makeAlu(X86Op::Add, MachOperand::virt(dst_ptr),
                             MachOperand::immediate(8), 64));
            }
        }
    } else {
        // For sub-32-bit loads (8/16-bit), use movzx/movsx to zero/sign-extend
        // to 64-bit. Without this, upper register bits contain garbage from
        // previous values, causing incorrect 64-bit comparisons.
        if (w <= 16 && !instr.load.is_volatile) {
            bool is_signed = instr.type < ctx_.types.size() &&
                             ctx_.types.isSigned(instr.type);
            // First do a narrow MovLoad, then extend
            MachInstr ld(X86Op::MovLoad);
            ld.width = w;
            ld.operand_count = 2;
            ld.inline_ops[0] = MachOperand::virt(dst);
            ld.inline_ops[1] = MachOperand::virt(ptr);
            emit(ld);
            // Extend in-place: movzx/movsx dst64, dst8/16
            MachInstr ext(is_signed ? X86Op::MovSX : X86Op::MovZX);
            ext.width = w;
            ext.operand_count = 2;
            ext.inline_ops[0] = MachOperand::virt(dst);
            ext.inline_ops[1] = MachOperand::virt(dst);
            emit(ext);
        } else {
            MachInstr mi(X86Op::MovLoad);
            mi.width = w;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::virt(dst);
            mi.inline_ops[1] = MachOperand::virt(ptr);
            mi.is_volatile = instr.load.is_volatile;
            emit(mi);
        }

        // If loading from a var slot (stack_ptr_vreg) and the result is a
        // pointer type, mark it as a stack address. This prevents unsafe
        // tail calls with arguments pointing into the local stack frame.
        if (stack_ptr_vregs_.count(ptr) && instr.type < ctx_.types.size()) {
            auto kind = ctx_.types.get(instr.type).kind;
            if (kind == TypeKind::Ptr || kind == TypeKind::PtrMut ||
                kind == TypeKind::Fn) {
                stack_ptr_vregs_.insert(dst);
            }
        }
    }
}

void InstructionSelector::selectStore(const LIRInstr& instr) {
    // mov [ptr], value  — store to memory at ptr
    VReg ptr = instr.store.ptr;
    VReg val = instr.store.value;

    // Check if the value vreg is a float (store type is Unit, not the value type)
    auto fit = float_vregs_.find(val);
    if (fit != float_vregs_.end()) {
        // Float store to pointer: movss/movsd [gpr], xmm
        uint8_t fw = fit->second;
        MachInstr mi(X86Op::FloatStore);
        mi.width = fw;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(ptr);
        mi.inline_ops[1] = MachOperand::virt(val);
        mi.is_volatile = instr.store.is_volatile;
        emit(mi);
    } else {
        uint8_t w = widthOf(instr.type);
        MachInstr mi(X86Op::MovStore);
        mi.width = w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(ptr);
        mi.inline_ops[1] = MachOperand::virt(val);
        mi.is_volatile = instr.store.is_volatile;
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

    // Propagate stack pointer status: field_ptr of a stack address
    // is still a stack address (base + offset within the same frame).
    if (stack_ptr_vregs_.count(base)) {
        stack_ptr_vregs_.insert(dst);
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
    struct_alloc_vregs_.insert(dst);
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
        if (hidden_ret_ptr_ != INVALID_VREG &&
            struct_vregs_.count(val) && struct_vreg_sizes_.count(val)) {
            // >16B struct return: copy struct data to [hidden_ret_ptr_] and
            // return the pointer in RAX.
            uint32_t sz = struct_vreg_sizes_[val];
            // Round up to 8-byte aligned for safe 8-byte copy loop
            assert(sz % 8 == 0 && "struct size must be 8-byte aligned for copy loop");
            VReg src = freshVReg();
            VReg dst = freshVReg();
            emit(makeMov(MachOperand::virt(src), MachOperand::virt(val), 64));
            emit(makeMov(MachOperand::virt(dst), MachOperand::virt(hidden_ret_ptr_), 64));
            for (uint32_t off = 0; off < sz; off += 8) {
                VReg tmp = freshVReg();
                MachInstr ld(X86Op::MovLoad);
                ld.width = 64;
                ld.operand_count = 2;
                ld.inline_ops[0] = MachOperand::virt(tmp);
                ld.inline_ops[1] = MachOperand::virt(src);
                emit(ld);
                MachInstr st(X86Op::MovStore);
                st.width = 64;
                st.operand_count = 2;
                st.inline_ops[0] = MachOperand::virt(dst);
                st.inline_ops[1] = MachOperand::virt(tmp);
                emit(st);
                if (off + 8 < sz) {
                    emit(makeAlu(X86Op::Add, MachOperand::virt(src),
                                 MachOperand::immediate(8), 64));
                    emit(makeAlu(X86Op::Add, MachOperand::virt(dst),
                                 MachOperand::immediate(8), 64));
                }
            }
            emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                         MachOperand::virt(hidden_ret_ptr_), 64));
        } else if (float_vregs_.count(val)) {
            uint8_t fw = float_vregs_[val];
            X86Op mov_op = (fw == 32) ? X86Op::Movss : X86Op::Movsd;
            MachInstr mi(mov_op);
            mi.width = fw;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::precolored(PhysReg::XMM0);
            mi.inline_ops[1] = MachOperand::virt(val);
            emit(mi);
        } else if (struct_vregs_.count(val) && struct_vreg_sizes_.count(val)) {
            // ≤16B struct return: pack fields into RAX (+ RDX for >8B).
            // Use precolored R11 as base pointer to prevent register allocator
            // from assigning it to RAX/RDX and causing clobber issues.
            uint32_t sz = struct_vreg_sizes_[val];
            emit(makeMov(MachOperand::precolored(PhysReg::R11),
                         MachOperand::virt(val), 64));
            {
                MachInstr load(X86Op::MovLoad);
                load.width = 64;
                load.operand_count = 2;
                load.inline_ops[0] = MachOperand::precolored(PhysReg::RAX);
                load.inline_ops[1] = MachOperand::precolored(PhysReg::R11);
                emit(load);
            }
            if (sz > 8) {
                emit(makeAlu(X86Op::Add, MachOperand::precolored(PhysReg::R11),
                             MachOperand::immediate(8), 64));
                MachInstr load2(X86Op::MovLoad);
                load2.width = 64;
                load2.operand_count = 2;
                load2.inline_ops[0] = MachOperand::precolored(PhysReg::RDX);
                load2.inline_ops[1] = MachOperand::precolored(PhysReg::R11);
                emit(load2);
            }
        } else {
            emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                         MachOperand::virt(val), 64));
        }
    }

    emit(makeRet());
}

void InstructionSelector::selectCall(const LIRInstr& instr) {
    const auto& call = instr.call;

    // >16B struct return: caller allocates stack space and passes hidden
    // pointer as the first GPR arg (RDI), shifting visible args by one.
    bool large_struct_ret = false;
    VReg ret_buf = INVALID_VREG;
    if (instr.result != INVALID_VREG && struct_vregs_.count(instr.result)) {
        uint32_t sz = struct_vreg_sizes_[instr.result];
        if (sz > 16) {
            large_struct_ret = true;
            sz = (sz + 7u) & ~7u;
            struct_alloc_bytes_ += sz;
            static constexpr int32_t CS_AREA = NUM_CALLEE_SAVED * 8;
            int32_t offset = -CS_AREA - static_cast<int32_t>(struct_alloc_bytes_);
            ret_buf = freshVReg();
            emit(makeLea(MachOperand::virt(ret_buf), MachOperand::stack(offset)));
        }
    }

    // Set up arguments via parallel move
    uint32_t gpr_idx = 0;
    uint32_t xmm_idx = 0;

    // Pass hidden return pointer as first GPR arg
    if (large_struct_ret) {
        emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[0]),
                     MachOperand::virt(ret_buf), 64));
        gpr_idx = 1;
    }

    for (uint32_t i = 0; i < call.arg_count; ++i) {
        VReg arg = call.args[i];
        if (float_vregs_.count(arg)) {
            // Float arg → XMM register
            if (xmm_idx < MAX_XMM_ARGS) {
                uint8_t fw = float_vregs_[arg];
                X86Op mov_op = (fw == 32) ? X86Op::Movss : X86Op::Movsd;
                MachInstr mi(mov_op);
                mi.width = fw;
                mi.operand_count = 2;
                mi.inline_ops[0] = MachOperand::precolored(XMM_ARG_REGS[xmm_idx]);
                mi.inline_ops[1] = MachOperand::virt(arg);
                emit(mi);
                xmm_idx++;
            }
        } else if (struct_vregs_.count(arg)) {
            // Struct arg: vreg holds a pointer to the struct data on stack.
            uint32_t size = struct_vreg_sizes_[arg];
            if (size > 16) {
                // >16B: pass pointer to struct data directly via GPR
                if (gpr_idx < MAX_GPR_ARGS) {
                    emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[gpr_idx]),
                                 MachOperand::virt(arg), 64));
                    gpr_idx++;
                }
            } else {
                // ≤16B: load fields into consecutive GPR args
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

    // Build callee label with module mangling
    std::string_view callee_label;
    {
        char buf[512];
        int len;
        const char* sp = symPrefix();

        // Check for @link_name override
        auto ln_it = link_names_.find(call.callee);
        if (ln_it != link_names_.end()) {
            len = snprintf(buf, sizeof(buf), "%s%.*s", sp,
                           static_cast<int>(ln_it->second.size()),
                           ln_it->second.data());
        } else {
            // Determine which module to use for mangling:
            // - cross-module call: use callee_module
            // - same-module call: use module_name_
            std::string_view mangle_mod = call.callee_module.empty()
                                            ? module_name_ : call.callee_module;

            // Don't mangle: main, intrinsics, extern "C" calls, or callee names
            // that already contain "__" (already mangled by monomorphization)
            if (call.callee == "main" || call.callee.find("__") != std::string_view::npos
                || mangle_mod.empty()) {
                len = snprintf(buf, sizeof(buf), "%s%.*s", sp,
                               static_cast<int>(call.callee.size()),
                               call.callee.data());
            } else {
                len = snprintf(buf, sizeof(buf), "%s%.*s__%.*s", sp,
                               static_cast<int>(mangle_mod.size()),
                               mangle_mod.data(),
                               static_cast<int>(call.callee.size()),
                               call.callee.data());
            }
        }
        callee_label = ctx_.strings.intern(std::string_view(buf, len));

        // Track cross-module calls as extern labels
        if (!call.callee_module.empty()) {
            extern_labels_.push_back(callee_label);
        }
    }

    // Can't tail-call if any arg is a stack pointer (struct_alloc/addr_of)
    // because the callee would access the caller's (destroyed) stack frame.
    // Also can't tail-call if we need to pass a hidden return pointer.
    bool can_tail = call.is_tail && !large_struct_ret;
    if (can_tail) {
        for (uint32_t i = 0; i < call.arg_count; ++i) {
            if (stack_ptr_vregs_.count(call.args[i]) ||
                struct_vregs_.count(call.args[i])) {
                can_tail = false;
                break;
            }
        }
    }

    // Variadic: SysV ABI requires AL = number of XMM args used
    if (call.is_variadic) {
        emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                     MachOperand::immediate(static_cast<int64_t>(xmm_idx)), 8));
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
        if (large_struct_ret) {
            // >16B struct return: result is already at ret_buf (callee wrote
            // through the hidden pointer). Point result vreg to it.
            emit(makeMov(MachOperand::virt(instr.result),
                         MachOperand::virt(ret_buf), 64));
            stack_ptr_vregs_.insert(instr.result);
        } else if (float_vregs_.count(instr.result)) {
            uint8_t fw = float_vregs_[instr.result];
            X86Op mov_op = (fw == 32) ? X86Op::Movss : X86Op::Movsd;
            MachInstr mi(mov_op);
            mi.width = fw;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::virt(instr.result);
            mi.inline_ops[1] = MachOperand::precolored(PhysReg::XMM0);
            emit(mi);
        } else if (struct_vregs_.count(instr.result)) {
            // ≤16B struct return: callee returned struct in RAX (+RDX for >8B).
            // Allocate local stack space and unpack the register values.
            uint32_t sz = struct_vreg_sizes_[instr.result];
            sz = (sz + 7u) & ~7u;
            struct_alloc_bytes_ += sz;
            static constexpr int32_t CS_AREA = NUM_CALLEE_SAVED * 8;
            int32_t offset = -CS_AREA - static_cast<int32_t>(struct_alloc_bytes_);
            // Point the result vreg to the stack slot
            emit(makeLea(MachOperand::virt(instr.result),
                         MachOperand::stack(offset)));
            stack_ptr_vregs_.insert(instr.result);

            // Store RAX → [struct_base + 0]
            VReg base1 = freshVReg();
            emit(makeMov(MachOperand::virt(base1), MachOperand::virt(instr.result), 64));
            MachInstr store1(X86Op::MovStore);
            store1.width = 64;
            store1.operand_count = 2;
            store1.inline_ops[0] = MachOperand::virt(base1);
            store1.inline_ops[1] = MachOperand::precolored(PhysReg::RAX);
            emit(store1);
            if (sz > 8) {
                // Store RDX → [struct_base + 8]
                VReg base2 = freshVReg();
                emit(makeMov(MachOperand::virt(base2), MachOperand::virt(instr.result), 64));
                emit(makeAlu(X86Op::Add, MachOperand::virt(base2),
                             MachOperand::immediate(8), 64));
                MachInstr store2(X86Op::MovStore);
                store2.width = 64;
                store2.operand_count = 2;
                store2.inline_ops[0] = MachOperand::virt(base2);
                store2.inline_ops[1] = MachOperand::precolored(PhysReg::RDX);
                emit(store2);
            }
        } else {
            emit(makeMov(MachOperand::virt(instr.result),
                         MachOperand::precolored(PhysReg::RAX), 64));
        }
    }
}

void InstructionSelector::selectFnRef(const LIRInstr& instr) {
    // lea vreg, [_fnname]  — get address of function
    char buf[512];
    int len;
    auto fn_name = instr.fn_ref.fn_name;
    std::string_view mangle_mod = instr.fn_ref.fn_module.empty()
                                    ? module_name_ : instr.fn_ref.fn_module;
    const char* sp = symPrefix();
    if (fn_name == "main" || fn_name.find("__") != std::string_view::npos
        || mangle_mod.empty()) {
        len = snprintf(buf, sizeof(buf), "%s%.*s", sp,
                       static_cast<int>(fn_name.size()), fn_name.data());
    } else {
        len = snprintf(buf, sizeof(buf), "%s%.*s__%.*s", sp,
                       static_cast<int>(mangle_mod.size()), mangle_mod.data(),
                       static_cast<int>(fn_name.size()), fn_name.data());
    }
    auto callee_label = ctx_.strings.intern(std::string_view(buf, len));
    emit(makeLea(MachOperand::virt(instr.result),
                 MachOperand::lbl(callee_label), 64));
}

void InstructionSelector::selectCallIndirect(const LIRInstr& instr) {
    const auto& ci = instr.call_indirect;

    // >16B struct return: allocate stack space and pass hidden pointer in RDI
    bool large_struct_ret = false;
    VReg ret_buf = INVALID_VREG;
    if (instr.result != INVALID_VREG && struct_vregs_.count(instr.result)) {
        uint32_t sz = struct_vreg_sizes_[instr.result];
        if (sz > 16) {
            large_struct_ret = true;
            sz = (sz + 7u) & ~7u;
            struct_alloc_bytes_ += sz;
            static constexpr int32_t CS_AREA = NUM_CALLEE_SAVED * 8;
            int32_t offset = -CS_AREA - static_cast<int32_t>(struct_alloc_bytes_);
            ret_buf = freshVReg();
            emit(makeLea(MachOperand::virt(ret_buf), MachOperand::stack(offset)));
        }
    }

    // Set up arguments via parallel move (same as direct call)
    uint32_t gpr_idx = 0;
    uint32_t xmm_idx = 0;

    // Pass hidden return pointer as first GPR arg
    if (large_struct_ret) {
        emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[0]),
                     MachOperand::virt(ret_buf), 64));
        gpr_idx = 1;
    }

    for (uint32_t i = 0; i < ci.arg_count; ++i) {
        VReg arg = ci.args[i];
        if (float_vregs_.count(arg)) {
            if (xmm_idx < MAX_XMM_ARGS) {
                uint8_t fw = float_vregs_[arg];
                X86Op mov_op = (fw == 32) ? X86Op::Movss : X86Op::Movsd;
                MachInstr mi(mov_op);
                mi.width = fw;
                mi.operand_count = 2;
                mi.inline_ops[0] = MachOperand::precolored(XMM_ARG_REGS[xmm_idx]);
                mi.inline_ops[1] = MachOperand::virt(arg);
                emit(mi);
                xmm_idx++;
            }
        } else if (struct_vregs_.count(arg)) {
            uint32_t size = struct_vreg_sizes_[arg];
            if (size > 16) {
                // >16B: pass pointer to struct data directly via GPR
                if (gpr_idx < MAX_GPR_ARGS) {
                    emit(makeMov(MachOperand::precolored(GPR_ARG_REGS[gpr_idx]),
                                 MachOperand::virt(arg), 64));
                    gpr_idx++;
                }
            } else {
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
        if (large_struct_ret) {
            // >16B struct return: result is at ret_buf
            emit(makeMov(MachOperand::virt(instr.result),
                         MachOperand::virt(ret_buf), 64));
            stack_ptr_vregs_.insert(instr.result);
        } else if (float_vregs_.count(instr.result)) {
            uint8_t fw = float_vregs_[instr.result];
            X86Op mov_op = (fw == 32) ? X86Op::Movss : X86Op::Movsd;
            MachInstr mi(mov_op);
            mi.width = fw;
            mi.operand_count = 2;
            mi.inline_ops[0] = MachOperand::virt(instr.result);
            mi.inline_ops[1] = MachOperand::precolored(PhysReg::XMM0);
            emit(mi);
        } else if (struct_vregs_.count(instr.result)) {
            // ≤16B struct return from indirect call: unpack RAX+RDX
            uint32_t sz = struct_vreg_sizes_[instr.result];
            sz = (sz + 7u) & ~7u;
            struct_alloc_bytes_ += sz;
            static constexpr int32_t CS_AREA = NUM_CALLEE_SAVED * 8;
            int32_t offset = -CS_AREA - static_cast<int32_t>(struct_alloc_bytes_);
            emit(makeLea(MachOperand::virt(instr.result),
                         MachOperand::stack(offset)));
            stack_ptr_vregs_.insert(instr.result);
            VReg base1 = freshVReg();
            emit(makeMov(MachOperand::virt(base1), MachOperand::virt(instr.result), 64));
            MachInstr store1(X86Op::MovStore);
            store1.width = 64;
            store1.operand_count = 2;
            store1.inline_ops[0] = MachOperand::virt(base1);
            store1.inline_ops[1] = MachOperand::precolored(PhysReg::RAX);
            emit(store1);
            if (sz > 8) {
                VReg base2 = freshVReg();
                emit(makeMov(MachOperand::virt(base2), MachOperand::virt(instr.result), 64));
                emit(makeAlu(X86Op::Add, MachOperand::virt(base2),
                             MachOperand::immediate(8), 64));
                MachInstr store2(X86Op::MovStore);
                store2.width = 64;
                store2.operand_count = 2;
                store2.inline_ops[0] = MachOperand::virt(base2);
                store2.inline_ops[1] = MachOperand::precolored(PhysReg::RDX);
                emit(store2);
            }
        } else {
            emit(makeMov(MachOperand::virt(instr.result),
                         MachOperand::precolored(PhysReg::RAX), 64));
        }
    }
}

void InstructionSelector::selectBlockArg(const LIRInstr& instr,
                                          const LIRFunction& fn) {
    // In the entry block (block 0), block args are function parameters
    // arriving via ABI registers. Use gpr_arg_slot_ to track cumulative
    // GPR slot usage (multi-register params like 16B structs consume 2 slots).
    if (current_block_ == 0) {
        // NOTE: hidden_ret_ptr_ capture from RDI is now emitted at the
        // start of the entry block in selectFunction(), so it works even
        // for zero-parameter functions.

        // Advance past any skipped (DCE'd) parameters so ABI register
        // assignment stays correct. E.g. if block_arg $0 was eliminated,
        // block_arg $1 must still use GPR slot 1 (RSI), not slot 0 (RDI).
        uint32_t target_idx = instr.block_arg.index;
        while (next_param_idx_ < target_idx && next_param_idx_ < fn.param_count) {
            TypeId skipped_type = fn.param_types[next_param_idx_];
            if (isFloat(skipped_type)) {
                xmm_arg_slot_++;
            } else if (isStructType(skipped_type)) {
                uint32_t sz = sizeOfType(skipped_type);
                if (sz > 16) {
                    gpr_arg_slot_++;  // pointer in one GPR
                } else {
                    uint32_t num_regs = (sz <= 8) ? 1 : 2;
                    gpr_arg_slot_ += num_regs;
                }
            } else {
                gpr_arg_slot_++;
            }
            next_param_idx_++;
        }
        next_param_idx_ = target_idx + 1;

        bool is_float_param = isFloat(instr.type);
        bool is_struct = isStructType(instr.type);

        if (is_struct) {
            uint32_t size = sizeOfType(instr.type);
            if (size > 16) {
                // >16B struct: caller passes a pointer in one GPR.
                // Copy to local stack for callee-owned storage.
                struct_alloc_bytes_ += (size + 7u) & ~7u;
                static constexpr int32_t CS_AREA = NUM_CALLEE_SAVED * 8;
                int32_t base_offset = -CS_AREA - static_cast<int32_t>(struct_alloc_bytes_);

                // src = pointer from GPR arg
                VReg src_ptr = freshVReg();
                if (gpr_arg_slot_ < MAX_GPR_ARGS) {
                    emit(makeMov(MachOperand::virt(src_ptr),
                                 MachOperand::precolored(GPR_ARG_REGS[gpr_arg_slot_]), 64));
                    gpr_arg_slot_++;
                }

                // Copy data from caller's pointer to local stack
                // Stack alloc was rounded to 8-byte alignment above; loop
                // uses the same rounded size to avoid over-read on non-aligned structs.
                uint32_t copy_size = (size + 7u) & ~7u;
                VReg dst_ptr = freshVReg();
                emit(makeLea(MachOperand::virt(dst_ptr), MachOperand::stack(base_offset)));
                for (uint32_t off = 0; off < copy_size; off += 8) {
                    VReg tmp = freshVReg();
                    MachInstr ld(X86Op::MovLoad);
                    ld.width = 64;
                    ld.operand_count = 2;
                    ld.inline_ops[0] = MachOperand::virt(tmp);
                    ld.inline_ops[1] = MachOperand::virt(src_ptr);
                    emit(ld);
                    emit(makeMov(MachOperand::stack(base_offset + static_cast<int32_t>(off)),
                                 MachOperand::virt(tmp), 64));
                    if (off + 8 < copy_size) {
                        emit(makeAlu(X86Op::Add, MachOperand::virt(src_ptr),
                                     MachOperand::immediate(8), 64));
                    }
                }

                emit(makeLea(MachOperand::virt(instr.result), MachOperand::stack(base_offset)));
            } else {
                // ≤16B struct: unpack from 1-2 GPR args to local stack
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
            }
        } else if (is_float_param) {
            if (xmm_arg_slot_ < MAX_XMM_ARGS) {
                uint8_t w = widthOf(instr.type);
                X86Op mov_op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
                MachInstr mi(mov_op);
                mi.width = w;
                mi.operand_count = 2;
                mi.inline_ops[0] = MachOperand::virt(instr.result);
                mi.inline_ops[1] = MachOperand::precolored(XMM_ARG_REGS[xmm_arg_slot_]);
                emit(mi);
                xmm_arg_slot_++;
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
    // But if this is a struct type, mark it as a stack pointer (the value was
    // propagated from a stack pointer in the predecessor block).
    if (isStructType(instr.type)) {
        stack_ptr_vregs_.insert(instr.result);
    }
}

void InstructionSelector::selectCast(const LIRInstr& instr) {
    VReg src = instr.cast.operand;
    VReg dst = instr.result;
    TypeId src_type = instr.cast.src_type;
    TypeId dst_type = instr.type;

    uint8_t src_w = widthOf(src_type);
    uint8_t dst_w = widthOf(dst_type);

    bool src_float = isFloat(src_type);
    bool dst_float = isFloat(dst_type);

    // Float → Integer
    if (src_float && !dst_float) {
        X86Op op = (src_w == 32) ? X86Op::Cvttss2si : X86Op::Cvttsd2si;
        MachInstr mi(op);
        mi.width = dst_w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(src);
        emit(mi);
        // If target is narrower than 64-bit, mask to truncate
        if (dst_w < 64) {
            int64_t mask = (dst_w == 8) ? 0xFF :
                           (dst_w == 16) ? 0xFFFF :
                           (dst_w == 32) ? (int64_t)0xFFFFFFFF : -1;
            emit(makeAlu(X86Op::And, MachOperand::virt(dst),
                         MachOperand::immediate(mask), 64));
        }
        return;
    }

    // Integer → Float
    if (!src_float && dst_float) {
        // Sign-extend source to 64-bit first if narrower
        VReg ext_src = src;
        if (src_w < 64) {
            ext_src = freshVReg();
            bool is_signed = ctx_.types.isSigned(src_type);
            X86Op ext_op = is_signed ? X86Op::MovSX : X86Op::MovZX;
            MachInstr ext(ext_op);
            ext.width = src_w;
            ext.operand_count = 2;
            ext.inline_ops[0] = MachOperand::virt(ext_src);
            ext.inline_ops[1] = MachOperand::virt(src);
            emit(ext);
        }
        X86Op op = (dst_w == 32) ? X86Op::Cvtsi2ss : X86Op::Cvtsi2sd;
        MachInstr mi(op);
        mi.width = 64;  // source GPR width
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(ext_src);
        emit(mi);
        return;
    }

    // Float → Float (f32↔f64)
    if (src_float && dst_float && src_w != dst_w) {
        X86Op op = (src_w == 64) ? X86Op::Cvtsd2ss : X86Op::Cvtss2sd;
        MachInstr mi(op);
        mi.width = src_w;
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(src);
        emit(mi);
        return;
    }

    // Ptr/Fn<->int: all are 64-bit addresses, just mov
    auto src_kind = (src_type < ctx_.types.size()) ? ctx_.types.get(src_type).kind : TypeKind::Primitive;
    auto dst_kind = (dst_type < ctx_.types.size()) ? ctx_.types.get(dst_type).kind : TypeKind::Primitive;
    if (src_kind == TypeKind::Ptr || src_kind == TypeKind::PtrMut ||
        dst_kind == TypeKind::Ptr || dst_kind == TypeKind::PtrMut ||
        src_kind == TypeKind::Fn || dst_kind == TypeKind::Fn) {
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
        // Propagate stack pointer status through Ptr/Fn↔int casts so that
        // the tail-call safety check can suppress TCO when a stack address
        // escapes as a u64 argument.
        if (stack_ptr_vregs_.count(src)) {
            stack_ptr_vregs_.insert(dst);
        }
        return;
    }

    if (dst_w > src_w) {
        // Widening: movzx or movsx
        bool is_signed = ctx_.types.isSigned(src_type);
        X86Op ext_op = is_signed ? X86Op::MovSX : X86Op::MovZX;
        MachInstr mi(ext_op);
        mi.width = src_w;  // source width for movsx/movzx
        mi.operand_count = 2;
        mi.inline_ops[0] = MachOperand::virt(dst);
        mi.inline_ops[1] = MachOperand::virt(src);
        emit(mi);
    } else if (dst_w < src_w) {
        // Narrowing: mov + mask to truncate
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
        int64_t mask = (dst_w == 8) ? 0xFF :
                       (dst_w == 16) ? 0xFFFF :
                       (dst_w == 32) ? (int64_t)0xFFFFFFFF : -1;
        emit(makeAlu(X86Op::And, MachOperand::virt(dst),
                     MachOperand::immediate(mask), 64));
    } else {
        // Same width: just mov
        emit(makeMov(MachOperand::virt(dst), MachOperand::virt(src), 64));
    }
}

// Map single-letter constraint to PhysReg
static PhysReg constraintToReg(std::string_view c) {
    // Strip leading '=' or '+'
    if (!c.empty() && (c[0] == '=' || c[0] == '+')) c = c.substr(1);
    if (c == "a") return PhysReg::RAX;
    if (c == "b") return PhysReg::RBX;
    if (c == "c") return PhysReg::RCX;
    if (c == "d") return PhysReg::RDX;
    if (c == "S") return PhysReg::RSI;
    if (c == "D") return PhysReg::RDI;
    // "r" = any register — we'll pick one based on index
    return PhysReg::RAX; // fallback
}

// Pick a GPR for "r" constraint, avoiding already-used registers
static PhysReg pickGPR(uint32_t idx, const PhysReg* used, uint32_t used_count) {
    static constexpr PhysReg GPR_POOL[] = {
        PhysReg::RAX, PhysReg::RCX, PhysReg::RDX, PhysReg::RSI,
        PhysReg::RDI, PhysReg::R8,  PhysReg::R9,  PhysReg::R10,
    };
    uint32_t pick_idx = 0;
    for (auto r : GPR_POOL) {
        bool in_use = false;
        for (uint32_t i = 0; i < used_count; ++i) {
            if (used[i] == r) { in_use = true; break; }
        }
        if (!in_use) {
            if (pick_idx == idx) return r;
            ++pick_idx;
        }
    }
    return GPR_POOL[idx % 8]; // fallback
}

static bool isSpecificRegConstraint(std::string_view c) {
    if (!c.empty() && (c[0] == '=' || c[0] == '+')) c = c.substr(1);
    return c == "a" || c == "b" || c == "c" || c == "d" || c == "S" || c == "D";
}

void InstructionSelector::selectInlineAsm(const LIRInstr& instr) {
    const auto& ia = instr.inline_asm;

    // If no constraints, emit raw asm (legacy path)
    if (ia.output_count == 0 && ia.input_count == 0) {
        MachInstr mi(X86Op::InlineAsm);
        mi.operand_count = 0;
        mi.asm_data.lines = ia.lines;
        mi.asm_data.line_lengths = ia.line_lengths;
        mi.asm_data.line_count = ia.line_count;
        mi.asm_data.outputs = nullptr;
        mi.asm_data.output_count = 0;
        mi.asm_data.inputs = nullptr;
        mi.asm_data.input_count = 0;
        mi.asm_data.clobbers = nullptr;
        mi.asm_data.clobber_count = 0;
        emit(mi);
        return;
    }

    // Extended asm with constraints
    // 1. Resolve physical registers for each operand
    PhysReg used_regs[16];
    uint32_t used_count = 0;
    uint32_t any_idx = 0; // counter for "r" constraints

    auto* out_bindings = ctx_.arena.makeArray<MachAsmBinding>(ia.output_count);
    for (uint32_t i = 0; i < ia.output_count; ++i) {
        auto c = ia.outputs[i].constraint;
        if (isSpecificRegConstraint(c)) {
            out_bindings[i].phys = constraintToReg(c);
        } else {
            out_bindings[i].phys = pickGPR(any_idx++, used_regs, used_count);
        }
        out_bindings[i].constraint = c;
        used_regs[used_count++] = out_bindings[i].phys;
    }

    auto* in_bindings = ctx_.arena.makeArray<MachAsmBinding>(ia.input_count);
    for (uint32_t i = 0; i < ia.input_count; ++i) {
        auto c = ia.inputs[i].constraint;
        if (isSpecificRegConstraint(c)) {
            in_bindings[i].phys = constraintToReg(c);
        } else {
            in_bindings[i].phys = pickGPR(any_idx++, used_regs, used_count);
        }
        in_bindings[i].constraint = c;
        used_regs[used_count++] = in_bindings[i].phys;
    }

    // 2. Move inputs to constrained registers
    for (uint32_t i = 0; i < ia.input_count; ++i) {
        emit(makeMov(MachOperand::precolored(in_bindings[i].phys),
                     MachOperand::virt(ia.inputs[i].vreg), 64));
    }

    // 3. Emit the asm block with resolved bindings
    MachInstr mi(X86Op::InlineAsm);
    mi.operand_count = 0;
    mi.asm_data.lines = ia.lines;
    mi.asm_data.line_lengths = ia.line_lengths;
    mi.asm_data.line_count = ia.line_count;
    mi.asm_data.outputs = out_bindings;
    mi.asm_data.output_count = ia.output_count;
    mi.asm_data.inputs = in_bindings;
    mi.asm_data.input_count = ia.input_count;
    mi.asm_data.clobbers = ia.clobbers;
    mi.asm_data.clobber_count = ia.clobber_count;
    emit(mi);

    // 4. Move output registers to output vregs
    for (uint32_t i = 0; i < ia.output_count; ++i) {
        emit(makeMov(MachOperand::virt(ia.outputs[i].vreg),
                     MachOperand::precolored(out_bindings[i].phys), 64));
    }
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
    // x86 cmpxchg: compares rax with [ptr], if equal [ptr]=desired, else rax=[ptr]
    // Result (old value) is always in rax after the instruction.

    // Move expected → rax (cmpxchg uses rax implicitly)
    emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                 MachOperand::virt(instr.atomic_cas.expected), 64));

    MachInstr mi(X86Op::LockCmpxchg);
    mi.width = 64;
    mi.operand_count = 3;
    mi.inline_ops[0] = MachOperand::precolored(PhysReg::RAX);
    mi.inline_ops[1] = MachOperand::virt(instr.atomic_cas.ptr);
    mi.inline_ops[2] = MachOperand::virt(instr.atomic_cas.desired);
    emit(mi);

    // Move result from rax → result vreg
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::precolored(PhysReg::RAX), 64));
}

void InstructionSelector::selectAtomicFetchAdd(const LIRInstr& instr) {
    // lock xadd [ptr], reg — atomically: old=[ptr], [ptr]+=reg, reg=old
    // The value register is modified in-place to hold the old value.
    // Move value → result vreg, then use result as the xadd operand.
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::virt(instr.atomic_fetch_add.value), 64));

    MachInstr mi(X86Op::LockXadd);
    mi.width = 64;
    mi.operand_count = 3;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.atomic_fetch_add.ptr);
    mi.inline_ops[2] = MachOperand::virt(instr.result);  // result vreg used as xadd operand
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

void InstructionSelector::selectLoadGlobal(const LIRInstr& instr) {
    // For aggregate-typed globals (arrays, structs), we need the address (lea)
    // not the value (mov), because field/index access works via pointer.
    bool is_aggregate = false;
    bool is_float = isFloat(instr.type);
    if (instr.type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(instr.type);
        is_aggregate = (ti.kind == TypeKind::Array || ti.kind == TypeKind::Struct ||
                        ti.kind == TypeKind::DynTrait);
    }

    if (is_float) {
        // Float globals: lea tmp, [rel label]; movsd/movss xmm, [tmp]
        VReg tmp = freshVReg();
        MachInstr lea(X86Op::LeaGlobal);
        lea.width = 64;
        lea.operand_count = 1;
        lea.inline_ops[0] = MachOperand::virt(tmp);
        lea.global_label = instr.load_global.label;
        emit(lea);

        MachInstr load(X86Op::FloatLoad);
        load.width = widthOf(instr.type);
        load.operand_count = 2;
        load.inline_ops[0] = MachOperand::virt(instr.result);
        load.inline_ops[1] = MachOperand::virt(tmp);
        emit(load);
        return;
    }

    MachInstr mi(is_aggregate ? X86Op::LeaGlobal : X86Op::MovLoadGlobal);
    mi.width = is_aggregate ? 64 : widthOf(instr.type);
    mi.operand_count = 1;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.global_label = instr.load_global.label;
    emit(mi);
}

void InstructionSelector::selectStoreGlobal(const LIRInstr& instr) {
    if (isFloat(instr.type)) {
        // Float globals: lea tmp, [rel label]; movsd/movss [tmp], xmm
        VReg tmp = freshVReg();
        MachInstr lea(X86Op::LeaGlobal);
        lea.width = 64;
        lea.operand_count = 1;
        lea.inline_ops[0] = MachOperand::virt(tmp);
        lea.global_label = instr.store_global.label;
        emit(lea);

        MachInstr store(X86Op::FloatStore);
        store.width = widthOf(instr.type);
        store.operand_count = 2;
        store.inline_ops[0] = MachOperand::virt(tmp);
        store.inline_ops[1] = MachOperand::virt(instr.store_global.value);
        emit(store);
        return;
    }

    MachInstr mi(X86Op::MovStoreGlobal);
    mi.width = widthOf(instr.type);
    mi.operand_count = 1;
    mi.inline_ops[0] = MachOperand::virt(instr.store_global.value);
    mi.global_label = instr.store_global.label;
    emit(mi);
}

// ============================================================================
// Bit manipulation
// ============================================================================

void InstructionSelector::selectClz(const LIRInstr& instr) {
    // clz(x) = 63 - bsr(x)  (bsr finds highest set bit index)
    // bsr dst, src
    MachInstr bsr(X86Op::Bsr);
    bsr.width = 64;
    bsr.operand_count = 2;
    bsr.inline_ops[0] = MachOperand::virt(instr.result);
    bsr.inline_ops[1] = MachOperand::virt(instr.unary.operand);
    emit(bsr);
    // xor dst, 63  (flip to get leading zeros count)
    MachInstr xr(X86Op::Xor);
    xr.width = 64;
    xr.operand_count = 2;
    xr.inline_ops[0] = MachOperand::virt(instr.result);
    xr.inline_ops[1] = MachOperand::immediate(63);
    emit(xr);
}

void InstructionSelector::selectCtz(const LIRInstr& instr) {
    // ctz(x) = bsf(x)  (bsf finds lowest set bit index)
    MachInstr mi(X86Op::Bsf);
    mi.width = 64;
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.unary.operand);
    emit(mi);
}

void InstructionSelector::selectPopcnt(const LIRInstr& instr) {
    MachInstr mi(X86Op::Popcnt);
    mi.width = 64;
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    mi.inline_ops[1] = MachOperand::virt(instr.unary.operand);
    emit(mi);
}

void InstructionSelector::selectBswap(const LIRInstr& instr) {
    // bswap operates in-place, so: mov dst, src; bswap dst
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::virt(instr.unary.operand), 64));
    MachInstr mi(X86Op::Bswap);
    mi.width = 64;
    mi.operand_count = 1;
    mi.inline_ops[0] = MachOperand::virt(instr.result);
    emit(mi);
}

// ============================================================================
// Port I/O
// ============================================================================

void InstructionSelector::selectPortIn(const LIRInstr& instr) {
    // in al/ax/eax, dx
    // Port number must be in DX, result comes in AL/AX/EAX (RAX)
    uint8_t w = widthOf(instr.type);
    if (w == 0) w = 8; // default to byte
    emit(makeMov(MachOperand::precolored(PhysReg::RDX),
                 MachOperand::virt(instr.port_in.port), 16));
    MachInstr mi(X86Op::In);
    mi.width = w;
    mi.operand_count = 1;
    mi.inline_ops[0] = MachOperand::precolored(PhysReg::RAX);
    emit(mi);
    emit(makeMov(MachOperand::virt(instr.result),
                 MachOperand::precolored(PhysReg::RAX), w));
}

void InstructionSelector::selectPortOut(const LIRInstr& instr) {
    // out dx, al/ax/eax
    // Port in DX, value in AL/AX/EAX
    uint8_t w = 8; // default byte for outb
    // Determine width from the value operand — just use 8 for now
    // The caller (outb=8, outw=16, outl=32) sets the type
    auto it = float_vregs_.find(instr.port_out.value);
    if (it == float_vregs_.end()) {
        // Not a float — use the instruction width based on type
        // For port I/O, type is Unit but we infer from the intrinsic name
        // Just use 8 bits as default; actual width set by caller
        w = 8;
    }
    emit(makeMov(MachOperand::precolored(PhysReg::RDX),
                 MachOperand::virt(instr.port_out.port), 16));
    emit(makeMov(MachOperand::precolored(PhysReg::RAX),
                 MachOperand::virt(instr.port_out.value), w));
    MachInstr mi(X86Op::Out);
    mi.width = w;
    mi.operand_count = 2;
    mi.inline_ops[0] = MachOperand::precolored(PhysReg::RDX);  // port (implicit)
    mi.inline_ops[1] = MachOperand::precolored(PhysReg::RAX);  // value
    emit(mi);
}

} // namespace kern
