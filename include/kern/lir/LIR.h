#pragma once
#include "kern/support/SourceLocation.h"
#include "kern/support/TypeSystem.h"
#include <cstdint>
#include <cstring>
#include <string_view>

namespace kern {

using VReg = uint32_t;
constexpr VReg INVALID_VREG = UINT32_MAX;

// ============================================================================
// LIR Opcodes
// ============================================================================

enum class LIROp : uint8_t {
    // Constants
    ConstInt,       // result = immediate integer
    ConstFloat,     // result = immediate float
    ConstBool,      // result = immediate bool
    ConstString,    // result = GlobalData index (string literal)
    GlobalRef,      // result = GlobalData index (float constant)

    // Integer arithmetic
    Add, Sub, Mul, Div, Mod,

    // Float arithmetic
    FAdd, FSub, FMul, FDiv,

    // Integer comparison (result: bool vreg)
    ICmpEq, ICmpNe, ICmpLt, ICmpLe, ICmpGt, ICmpGe,

    // Float comparison (result: bool vreg)
    FCmpEq, FCmpNe, FCmpLt, FCmpLe, FCmpGt, FCmpGe,

    // Unary
    Neg, FNeg, Not,

    // Memory
    AddrOf,         // &x → ptr vreg
    Load,           // *ptr → value vreg
    Store,          // *ptr = value (no result)
    FieldPtr,       // base + offset → ptr vreg (GEP-like)
    StructAlloc,    // stack alloc → base ptr vreg

    // Control flow
    Branch,         // unconditional jump to target block
    CondBranch,     // conditional: cond ? true_target : false_target
    Ret,            // return value (or void)

    // Calls
    Call,           // direct call by name

    // Block parameter (phi replacement)
    BlockArg,       // loads block parameter by index
};

const char* lirOpName(LIROp op);

// ============================================================================
// LIR Instructions (tagged union, Arena-allocated)
// ============================================================================

// Operand payload types (named to avoid -Wnested-anon-types)
struct LIRConstInt     { int64_t value; };
struct LIRConstFloat   { double value; };
struct LIRConstBool    { bool value; };
struct LIRGlobalIdx    { uint32_t global_index; };
struct LIRBinPayload   { VReg lhs; VReg rhs; };
struct LIRUnaryPayload { VReg operand; };
struct LIRCallPayload  {
    std::string_view callee;    // interned
    VReg* args;
    uint32_t arg_count;
    bool is_tail;
};
struct LIRBranchPayload    { uint32_t target; };
struct LIRCondBrPayload    { VReg cond; uint32_t true_target; uint32_t false_target; };
struct LIRRetPayload       { VReg value; };
struct LIRAddrOfPayload    { VReg source; };
struct LIRLoadPayload      { VReg ptr; };
struct LIRStorePayload     { VReg ptr; VReg value; };
struct LIRFieldPtrPayload  { VReg base; uint32_t offset; };
struct LIRStructAllocPayload { uint32_t size; uint32_t align; };
struct LIRBlockArgPayload  { uint32_t index; };

struct LIRInstr {
    LIROp op;
    VReg result;            // result virtual register (INVALID_VREG if void)
    TypeId type;            // result type
    SourceLocation loc;

    LIRInstr() : op(LIROp::ConstInt), result(INVALID_VREG), type(0), loc() {
        std::memset(&const_int, 0, sizeof(LIRCallPayload));
    }

    union {
        LIRConstInt         const_int;
        LIRConstFloat       const_float;
        LIRConstBool        const_bool;
        LIRGlobalIdx        const_string;
        LIRGlobalIdx        global_ref;
        LIRBinPayload       bin;
        LIRUnaryPayload     unary;
        LIRCallPayload      call;
        LIRBranchPayload    branch;
        LIRCondBrPayload    cond_branch;
        LIRRetPayload       ret;
        LIRAddrOfPayload    addr_of;
        LIRLoadPayload      load;
        LIRStorePayload     store;
        LIRFieldPtrPayload  field_ptr;
        LIRStructAllocPayload struct_alloc;
        LIRBlockArgPayload  block_arg;
    };
};

// ============================================================================
// LIR Blocks
// ============================================================================

struct LIRBlock {
    std::string_view label;     // interned
    TypeId* param_types;        // block parameter types (phi replacement)
    uint32_t param_count;
    LIRInstr* instrs;           // Arena-allocated array
    uint32_t instr_count;
};

// ============================================================================
// LIR Functions
// ============================================================================

enum class Purity : uint8_t;  // forward decl

struct LIRFunction {
    std::string_view name;      // interned
    TypeId* param_types;        // Arena-allocated
    uint32_t param_count;
    TypeId return_type;
    LIRBlock* blocks;           // Arena-allocated array
    uint32_t block_count;
    VReg next_vreg;             // vreg allocation counter

    // Metadata (copied from HIR)
    uint8_t purity;             // Purity enum value
    bool is_recursive;
    bool is_tail_recursive;
    bool is_intrinsic;
};

// ============================================================================
// Global Data (.rodata)
// ============================================================================

struct GlobalStringLit { const char* data; uint32_t length; };
struct GlobalFloatConst { double value; bool is_f32; };

struct GlobalData {
    enum Kind : uint8_t { StringLit, FloatConst };
    Kind kind;
    uint32_t index;             // unique index within module
    std::string_view label;     // NASM label (interned)

    union {
        GlobalStringLit  string_lit;
        GlobalFloatConst float_const;
    };
};

// ============================================================================
// LIR Module
// ============================================================================

struct LIRModule {
    LIRFunction* functions;
    uint32_t fn_count;

    GlobalData* globals;
    uint32_t global_count;
};

} // namespace kern
