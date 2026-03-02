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

    // Wrapping arithmetic (mod 2^N, no trap)
    AddWrap, SubWrap, MulWrap,

    // Saturating arithmetic (clamp to min/max)
    AddSat, SubSat,

    // Bitwise
    BAnd, BOr, BXor, Shl, Shr,

    // Float arithmetic
    FAdd, FSub, FMul, FDiv,

    // Integer comparison (result: bool vreg)
    ICmpEq, ICmpNe, ICmpLt, ICmpLe, ICmpGt, ICmpGe,

    // Float comparison (result: bool vreg)
    FCmpEq, FCmpNe, FCmpLt, FCmpLe, FCmpGt, FCmpGe,

    // Unary
    Neg, FNeg, Not, BNot,

    // Type cast (integer widening/narrowing, int<->ptr)
    Cast,

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
    CallIndirect,   // indirect call through function pointer
    FnRef,          // get address of named function

    // Block parameter (phi replacement)
    BlockArg,       // loads block parameter by index

    // Inline assembly
    InlineAsm,      // raw assembly lines (passthrough)

    // Atomic operations
    AtomicLoad,     // atomic load with memory ordering
    AtomicStore,    // atomic store with memory ordering
    AtomicCas,      // compare-and-swap (returns old value)
    AtomicFetchAdd, // atomic fetch-and-add (returns old value)

    // Fences
    Fence,          // hardware memory fence (mfence/sfence/lfence)
    CompilerFence,  // compiler-only barrier (no instruction emitted)

    // Per-CPU data
    PercpuLoad,     // load via GS segment
    PercpuStore,    // store via GS segment

    // Global variables (.data/.bss/.rodata)
    LoadGlobal,     // result = *label (load from global)
    StoreGlobal,    // *label = value (store to global)

    // Bit manipulation
    Clz,            // count leading zeros (bsr on x86)
    Ctz,            // count trailing zeros (bsf on x86)
    Popcnt,         // population count (popcnt on x86)
    Bswap,          // byte swap (bswap on x86)

    // Port I/O (x86 in/out)
    PortIn,         // inb/inw/inl: read from I/O port
    PortOut,        // outb/outw/outl: write to I/O port
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
    std::string_view callee_module;  // module path of callee (empty if same module)
    VReg* args;
    uint32_t arg_count;
    bool is_tail;
    bool is_variadic;
};
struct LIRBranchPayload    { uint32_t target; VReg* args; uint32_t arg_count; };
struct LIRCondBrPayload    { VReg cond; uint32_t true_target; uint32_t false_target; };
struct LIRRetPayload       { VReg value; };
struct LIRAddrOfPayload    { VReg source; };
struct LIRLoadPayload      { VReg ptr; bool is_volatile = false; };
struct LIRStorePayload     { VReg ptr; VReg value; bool is_volatile = false; };
struct LIRFieldPtrPayload  { VReg base; uint32_t offset; };
struct LIRStructAllocPayload { uint32_t size; uint32_t align; };
struct LIRBlockArgPayload  { uint32_t index; };
struct LIRCastPayload      { VReg operand; TypeId src_type; };
struct LIRCallIndirectPayload {
    VReg callee;                // vreg holding the function pointer
    VReg* args;
    uint32_t arg_count;
};
struct LIRFnRefPayload { std::string_view fn_name; std::string_view fn_module; };
struct LIRAsmOperand {
    std::string_view constraint;
    VReg vreg;  // vreg holding the value (for inputs) or receiving value (for outputs)
};
struct LIRInlineAsmPayload {
    const char** lines;
    uint32_t* line_lengths;
    uint32_t line_count;
    LIRAsmOperand* outputs;
    uint32_t output_count;
    LIRAsmOperand* inputs;
    uint32_t input_count;
    std::string_view* clobbers;
    uint32_t clobber_count;
};

// Memory ordering for atomic operations
enum class MemOrder : uint8_t {
    Relaxed = 0,
    Acquire = 1,
    Release = 2,
    AcqRel  = 3,
    SeqCst  = 4,
};

struct LIRAtomicLoadPayload  { VReg ptr; MemOrder order; };
struct LIRAtomicStorePayload { VReg ptr; VReg value; MemOrder order; };
struct LIRAtomicCasPayload   { VReg ptr; VReg expected; VReg desired; MemOrder order; };
struct LIRAtomicFetchAddPayload { VReg ptr; VReg value; MemOrder order; };
struct LIRFencePayload       { MemOrder order; };
struct LIRPercpuPayload      { VReg offset; };
struct LIRPercpuStorePayload { VReg offset; VReg value; };
struct LIRLoadGlobalPayload  { std::string_view label; };
struct LIRStoreGlobalPayload { std::string_view label; VReg value; };
struct LIRPortInPayload  { VReg port; };
struct LIRPortOutPayload { VReg port; VReg value; };

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
        LIRCastPayload      cast;
        LIRCallIndirectPayload call_indirect;
        LIRFnRefPayload     fn_ref;
        LIRInlineAsmPayload inline_asm;
        LIRAtomicLoadPayload atomic_load;
        LIRAtomicStorePayload atomic_store;
        LIRAtomicCasPayload  atomic_cas;
        LIRAtomicFetchAddPayload atomic_fetch_add;
        LIRFencePayload      fence;
        LIRPercpuPayload     percpu_load;
        LIRPercpuStorePayload percpu_store;
        LIRLoadGlobalPayload  load_global;
        LIRStoreGlobalPayload store_global;
        LIRPortInPayload      port_in;
        LIRPortOutPayload     port_out;
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
    bool is_naked;
    bool is_interrupt;
    bool is_inline = false;     // @inline — hint to inline
    bool is_noinline = false;   // @noinline — prevent inlining
    bool is_pub = false;        // pub fn — export as global symbol
    bool is_extern = false;     // extern "C" fn — external C linkage
    bool is_variadic = false;   // fn(x: T, ...) — C-style varargs
    bool is_weak = false;       // @weak — weak symbol linkage
    std::string_view section_name;  // @section("name"), empty = default
    std::string_view link_name;     // @link_name("name"), empty = auto
};

// ============================================================================
// Global Data (.rodata)
// ============================================================================

struct GlobalStringLit { const char* data; uint32_t length; };
struct GlobalFloatConst { double value; bool is_f32; };
struct GlobalVariable {
    int64_t init_value;       // integer init value (0 for .bss)
    uint8_t size;             // 1/2/4/8 bytes (element size for arrays)
    bool is_mutable;          // static var → .data/.bss, static val → .rodata
    bool is_extern;           // extern static — linker-defined symbol (no storage)
    int64_t* array_values;    // array initializer values (nullptr if scalar)
    uint32_t array_count;     // number of elements (0 if scalar)
    uint8_t* init_bytes;      // raw byte initializer (struct/float literals)
    uint32_t init_byte_count; // byte count for init_bytes (0 if unused)
    std::string_view section_name;  // @section("name"), empty = default
    std::string_view link_name;     // @link_name("name"), empty = use label
};

struct GlobalVTable {
    std::string_view* fn_labels;  // ordered function labels for vtable slots
    uint32_t method_count;
};

struct GlobalData {
    enum Kind : uint8_t { StringLit, FloatConst, Variable, VTable };
    Kind kind;
    uint32_t index;             // unique index within module
    std::string_view label;     // NASM label (interned)

    union {
        GlobalStringLit  string_lit;
        GlobalFloatConst float_const;
        GlobalVariable   variable;
        GlobalVTable     vtable;
    };

    GlobalData() : kind(StringLit), index(0), string_lit{} {}
};

// ============================================================================
// LIR Module
// ============================================================================

struct LIRModule {
    std::string_view module_name;  // propagated from HIRModule

    LIRFunction* functions;
    uint32_t fn_count;

    GlobalData* globals;
    uint32_t global_count;
};

} // namespace kern
