#pragma once
#include "kern/support/TypeSystem.h"
#include <cstdint>
#include <cstring>
#include <string_view>

namespace kern {

// ============================================================================
// Physical Registers (x86-64)
// ============================================================================

enum class PhysReg : uint8_t {
    // General purpose
    RAX, RBX, RCX, RDX, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
    // SSE
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    // Special
    RSP, RBP,
    // Sentinel
    NONE,
};

const char* physRegName(PhysReg reg, uint8_t width = 64);
bool isGPR(PhysReg reg);
bool isXMM(PhysReg reg);

// ABI register arrays
static constexpr PhysReg GPR_ARG_REGS[] = {
    PhysReg::RDI, PhysReg::RSI, PhysReg::RDX,
    PhysReg::RCX, PhysReg::R8, PhysReg::R9,
};
static constexpr uint32_t MAX_GPR_ARGS = 6;

static constexpr PhysReg XMM_ARG_REGS[] = {
    PhysReg::XMM0, PhysReg::XMM1, PhysReg::XMM2, PhysReg::XMM3,
    PhysReg::XMM4, PhysReg::XMM5, PhysReg::XMM6, PhysReg::XMM7,
};
static constexpr uint32_t MAX_XMM_ARGS = 8;

// Callee-saved GPRs
static constexpr PhysReg CALLEE_SAVED_GPRS[] = {
    PhysReg::RBX, PhysReg::R12, PhysReg::R13,
    PhysReg::R14, PhysReg::R15,
};
static constexpr uint32_t NUM_CALLEE_SAVED = 5;

// Caller-saved GPRs (clobbered by calls)
static constexpr PhysReg CALLER_SAVED_GPRS[] = {
    PhysReg::RAX, PhysReg::RCX, PhysReg::RDX,
    PhysReg::RSI, PhysReg::RDI,
    PhysReg::R8, PhysReg::R9, PhysReg::R10, PhysReg::R11,
};
static constexpr uint32_t NUM_CALLER_SAVED = 9;

// Allocatable GPRs (caller-saved, excluding RAX which is scratch/return)
static constexpr PhysReg ALLOCATABLE_GPRS[] = {
    PhysReg::RAX, PhysReg::RCX, PhysReg::RDX, PhysReg::RSI, PhysReg::RDI,
    PhysReg::R8, PhysReg::R9, PhysReg::R10, PhysReg::R11,
    PhysReg::RBX, PhysReg::R12, PhysReg::R13, PhysReg::R14, PhysReg::R15,
};
static constexpr uint32_t NUM_ALLOCATABLE_GPRS = 14;

static constexpr PhysReg ALLOCATABLE_XMMS[] = {
    PhysReg::XMM0, PhysReg::XMM1, PhysReg::XMM2, PhysReg::XMM3,
    PhysReg::XMM4, PhysReg::XMM5, PhysReg::XMM6, PhysReg::XMM7,
    PhysReg::XMM8, PhysReg::XMM9, PhysReg::XMM10, PhysReg::XMM11,
    PhysReg::XMM12, PhysReg::XMM13, PhysReg::XMM14, PhysReg::XMM15,
};
static constexpr uint32_t NUM_ALLOCATABLE_XMMS = 16;

// ============================================================================
// Machine Operand
// ============================================================================

struct MachOperand {
    enum Kind : uint8_t { Reg, Imm, Stack, Label, None };
    Kind kind = Kind::None;
    bool is_physical = false;   // true = PhysReg, false = VReg

    union {
        uint32_t vreg;
        PhysReg phys;
        int64_t imm;
        int32_t stack_offset;
        std::string_view label;
    };

    MachOperand() : kind(Kind::None), is_physical(false) {
        std::memset(&vreg, 0, sizeof(std::string_view));
    }

    static MachOperand virt(uint32_t v) {
        MachOperand op;
        op.kind = Kind::Reg;
        op.is_physical = false;
        op.vreg = v;
        return op;
    }

    static MachOperand precolored(PhysReg r) {
        MachOperand op;
        op.kind = Kind::Reg;
        op.is_physical = true;
        op.phys = r;
        return op;
    }

    static MachOperand physical(PhysReg r) {
        return precolored(r);
    }

    static MachOperand immediate(int64_t v) {
        MachOperand op;
        op.kind = Kind::Imm;
        op.imm = v;
        return op;
    }

    static MachOperand stack(int32_t offset) {
        MachOperand op;
        op.kind = Kind::Stack;
        op.stack_offset = offset;
        return op;
    }

    static MachOperand lbl(std::string_view l) {
        MachOperand op;
        op.kind = Kind::Label;
        op.label = l;
        return op;
    }

    static MachOperand none() {
        return MachOperand{};
    }

    bool isReg() const { return kind == Kind::Reg; }
    bool isPhysical() const { return kind == Kind::Reg && is_physical; }
    bool isVirtual() const { return kind == Kind::Reg && !is_physical; }
    bool isImm() const { return kind == Kind::Imm; }
    bool isStack() const { return kind == Kind::Stack; }
    bool isLabel() const { return kind == Kind::Label; }
    bool isNone() const { return kind == Kind::None; }
};

// ============================================================================
// x86-64 Opcodes
// ============================================================================

enum class X86Op : uint8_t {
    // Data movement
    Mov, MovZX, MovSX,
    MovLoad,  // mov dst, [src]  — load from memory at register address
    MovStore, // mov [dst], src  — store to memory at register address
    Lea,
    Push, Pop,

    // Integer arithmetic
    Add, Sub, IMul, IDiv, Xor, And, Or,
    Shl, Shr, Sar,
    Neg, Not,

    // Division prep
    Cqo,

    // Comparison + conditional
    Cmp, Test,
    Setcc,          // + condition code
    Jmp, Jcc,       // + condition code
    Call, Ret,

    // SSE
    Movss, Movsd,
    Addss, Addsd, Subss, Subsd,
    Mulss, Mulsd, Divss, Divsd,
    Ucomisd, Ucomiss,
    Xorps, Xorpd,  // zero float register

    // Pseudo-instructions
    Pseudo_ParallelMove,    // multi-reg swap (cycle-break)
    Pseudo_FrameSetup,      // prologue placeholder
    Pseudo_FrameDestroy,    // epilogue placeholder

    Nop,

    // Raw assembly passthrough
    InlineAsm,
};

const char* x86OpName(X86Op op);

// Condition codes (used by Setcc, Jcc)
enum class CondCode : uint8_t {
    E, NE, L, LE, G, GE,       // signed
    B, BE, A, AE,               // unsigned
};

const char* condCodeSuffix(CondCode cc);

// ============================================================================
// Machine Instruction
// ============================================================================

static constexpr uint8_t MACH_INLINE_OPERANDS = 4;

// Inline assembly payload for MachInstr
struct MachInlineAsmData {
    const char** lines;
    uint32_t* line_lengths;
    uint32_t line_count;
};

struct MachInstr {
    X86Op op;
    CondCode cc = CondCode::E;      // for Setcc, Jcc
    uint8_t width = 64;             // 8, 16, 32, 64
    uint8_t operand_count = 0;

    union {
        MachOperand inline_ops[MACH_INLINE_OPERANDS];
        MachOperand* heap_ops;
    };

    // For InlineAsm instructions
    MachInlineAsmData asm_data = {};

    MachInstr() : op(X86Op::Nop) {
        std::memset(inline_ops, 0, sizeof(inline_ops));
    }

    explicit MachInstr(X86Op o) : op(o) {
        std::memset(inline_ops, 0, sizeof(inline_ops));
    }

    MachOperand& operand(uint8_t i) {
        return (operand_count <= MACH_INLINE_OPERANDS)
            ? inline_ops[i] : heap_ops[i];
    }

    const MachOperand& operand(uint8_t i) const {
        return (operand_count <= MACH_INLINE_OPERANDS)
            ? inline_ops[i] : heap_ops[i];
    }

    MachOperand& dst()  { return operand(0); }
    MachOperand& src1() { return operand(1); }
    MachOperand& src2() { return operand(2); }
    const MachOperand& dst() const  { return operand(0); }
    const MachOperand& src1() const { return operand(1); }
    const MachOperand& src2() const { return operand(2); }
};

// Instruction builders (convenience)
MachInstr makeMov(MachOperand dst, MachOperand src, uint8_t width = 64);
MachInstr makeAlu(X86Op op, MachOperand dst, MachOperand src, uint8_t width = 64);
MachInstr makeCmp(MachOperand lhs, MachOperand rhs, uint8_t width = 64);
MachInstr makeSetcc(CondCode cc, MachOperand dst);
MachInstr makeJmp(MachOperand label);
MachInstr makeJcc(CondCode cc, MachOperand label);
MachInstr makeCall(MachOperand target);
MachInstr makeRet();
MachInstr makePush(MachOperand src);
MachInstr makePop(MachOperand dst);
MachInstr makeLea(MachOperand dst, MachOperand src, uint8_t width = 64);

// ============================================================================
// Machine Blocks and Functions
// ============================================================================

struct MachBlock {
    std::string_view label;         // interned
    MachInstr* instrs;
    uint32_t instr_count;
};

struct MachFunction {
    std::string_view name;          // interned
    MachBlock* blocks;
    uint32_t block_count;
    uint32_t stack_size = 0;
    uint32_t struct_alloc_bytes = 0; // extra stack for struct_alloc
    uint32_t next_vreg = 0;         // for vreg tracking
    bool callee_saved_used[NUM_CALLEE_SAVED] = {};
    bool is_intrinsic = false;
};

// ============================================================================
// Machine Module
// ============================================================================

struct MachModule {
    MachFunction* functions;
    uint32_t fn_count;
    // GlobalData comes from LIRModule (shared)
};

} // namespace kern
