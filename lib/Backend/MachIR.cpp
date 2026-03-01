#include "kern/backend/MachIR.h"

namespace kern {

// ============================================================================
// PhysReg names
// ============================================================================

const char* physRegName(PhysReg reg, uint8_t width) {
    // 64-bit names
    static const char* names64[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
        "rsp", "rbp",
        "none",
    };
    static const char* names32[] = {
        "eax", "ebx", "ecx", "edx", "esi", "edi",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
    };
    static const char* names16[] = {
        "ax", "bx", "cx", "dx", "si", "di",
        "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
    };
    static const char* names8[] = {
        "al", "bl", "cl", "dl", "sil", "dil",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
    };

    auto idx = static_cast<uint8_t>(reg);

    // XMM registers don't have width variants
    if (idx >= static_cast<uint8_t>(PhysReg::XMM0) &&
        idx <= static_cast<uint8_t>(PhysReg::XMM15)) {
        return names64[idx];
    }
    // RSP, RBP, NONE always 64-bit name
    if (idx >= static_cast<uint8_t>(PhysReg::RSP)) {
        return names64[idx];
    }

    // GPR: select by width
    if (idx < 14) {
        switch (width) {
            case 8:  return names8[idx];
            case 16: return names16[idx];
            case 32: return names32[idx];
            default: return names64[idx];
        }
    }

    return names64[idx];
}

bool isGPR(PhysReg reg) {
    auto idx = static_cast<uint8_t>(reg);
    return idx <= static_cast<uint8_t>(PhysReg::R15);
}

bool isXMM(PhysReg reg) {
    auto idx = static_cast<uint8_t>(reg);
    return idx >= static_cast<uint8_t>(PhysReg::XMM0) &&
           idx <= static_cast<uint8_t>(PhysReg::XMM15);
}

// ============================================================================
// x86 Op names
// ============================================================================

const char* x86OpName(X86Op op) {
    switch (op) {
        case X86Op::Mov:    return "mov";
        case X86Op::MovZX:  return "movzx";
        case X86Op::MovSX:  return "movsx";
        case X86Op::Lea:    return "lea";
        case X86Op::Push:   return "push";
        case X86Op::Pop:    return "pop";
        case X86Op::Add:    return "add";
        case X86Op::Sub:    return "sub";
        case X86Op::IMul:   return "imul";
        case X86Op::IDiv:   return "idiv";
        case X86Op::Xor:    return "xor";
        case X86Op::And:    return "and";
        case X86Op::Or:     return "or";
        case X86Op::Neg:    return "neg";
        case X86Op::Not:    return "not";
        case X86Op::Cqo:    return "cqo";
        case X86Op::Cmp:    return "cmp";
        case X86Op::Test:   return "test";
        case X86Op::Setcc:  return "set";
        case X86Op::Jmp:    return "jmp";
        case X86Op::Jcc:    return "j";
        case X86Op::Call:   return "call";
        case X86Op::Ret:    return "ret";
        case X86Op::Movss:  return "movss";
        case X86Op::Movsd:  return "movsd";
        case X86Op::Addss:  return "addss";
        case X86Op::Addsd:  return "addsd";
        case X86Op::Subss:  return "subss";
        case X86Op::Subsd:  return "subsd";
        case X86Op::Mulss:  return "mulss";
        case X86Op::Mulsd:  return "mulsd";
        case X86Op::Divss:  return "divss";
        case X86Op::Divsd:  return "divsd";
        case X86Op::Ucomisd: return "ucomisd";
        case X86Op::Ucomiss: return "ucomiss";
        case X86Op::Xorps:  return "xorps";
        case X86Op::Xorpd:  return "xorpd";
        case X86Op::Pseudo_ParallelMove: return "parallel_move";
        case X86Op::Pseudo_FrameSetup:   return "frame_setup";
        case X86Op::Pseudo_FrameDestroy: return "frame_destroy";
        case X86Op::Nop:    return "nop";
    }
    return "?";
}

const char* condCodeSuffix(CondCode cc) {
    switch (cc) {
        case CondCode::E:  return "e";
        case CondCode::NE: return "ne";
        case CondCode::L:  return "l";
        case CondCode::LE: return "le";
        case CondCode::G:  return "g";
        case CondCode::GE: return "ge";
        case CondCode::B:  return "b";
        case CondCode::BE: return "be";
        case CondCode::A:  return "a";
        case CondCode::AE: return "ae";
    }
    return "?";
}

// ============================================================================
// Instruction builders
// ============================================================================

MachInstr makeMov(MachOperand dst, MachOperand src, uint8_t width) {
    MachInstr mi(X86Op::Mov);
    mi.width = width;
    mi.operand_count = 2;
    mi.inline_ops[0] = dst;
    mi.inline_ops[1] = src;
    return mi;
}

MachInstr makeAlu(X86Op op, MachOperand dst, MachOperand src, uint8_t width) {
    MachInstr mi(op);
    mi.width = width;
    mi.operand_count = 2;
    mi.inline_ops[0] = dst;
    mi.inline_ops[1] = src;
    return mi;
}

MachInstr makeCmp(MachOperand lhs, MachOperand rhs, uint8_t width) {
    MachInstr mi(X86Op::Cmp);
    mi.width = width;
    mi.operand_count = 2;
    mi.inline_ops[0] = lhs;
    mi.inline_ops[1] = rhs;
    return mi;
}

MachInstr makeSetcc(CondCode cc, MachOperand dst) {
    MachInstr mi(X86Op::Setcc);
    mi.cc = cc;
    mi.width = 8;
    mi.operand_count = 1;
    mi.inline_ops[0] = dst;
    return mi;
}

MachInstr makeJmp(MachOperand label) {
    MachInstr mi(X86Op::Jmp);
    mi.operand_count = 1;
    mi.inline_ops[0] = label;
    return mi;
}

MachInstr makeJcc(CondCode cc, MachOperand label) {
    MachInstr mi(X86Op::Jcc);
    mi.cc = cc;
    mi.operand_count = 1;
    mi.inline_ops[0] = label;
    return mi;
}

MachInstr makeCall(MachOperand target) {
    MachInstr mi(X86Op::Call);
    mi.operand_count = 1;
    mi.inline_ops[0] = target;
    return mi;
}

MachInstr makeRet() {
    MachInstr mi(X86Op::Ret);
    mi.operand_count = 0;
    return mi;
}

MachInstr makePush(MachOperand src) {
    MachInstr mi(X86Op::Push);
    mi.operand_count = 1;
    mi.inline_ops[0] = src;
    return mi;
}

MachInstr makePop(MachOperand dst) {
    MachInstr mi(X86Op::Pop);
    mi.operand_count = 1;
    mi.inline_ops[0] = dst;
    return mi;
}

MachInstr makeLea(MachOperand dst, MachOperand src, uint8_t width) {
    MachInstr mi(X86Op::Lea);
    mi.width = width;
    mi.operand_count = 2;
    mi.inline_ops[0] = dst;
    mi.inline_ops[1] = src;
    return mi;
}

} // namespace kern
