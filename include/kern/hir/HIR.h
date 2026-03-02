#pragma once
#include "kern/support/SourceLocation.h"
#include "kern/support/TypeSystem.h"
#include <cstdint>
#include <string_view>

namespace kern {

// Forward declarations
struct HIRExpr;
struct HIRStmt;
struct HIRPattern;

// ============================================================================
// HIR Patterns (for match expressions)
// ============================================================================

struct HIRPattern {
    enum class Kind : uint8_t {
        IntLit, BoolLit, Wildcard, Variable, Enum, Union, Range
    };
    Kind kind;
    TypeId type;
    SourceLocation loc;
};

struct HIRIntLitPattern : HIRPattern {
    int64_t value;
};

struct HIRBoolLitPattern : HIRPattern {
    bool value;
};

struct HIRWildcardPattern : HIRPattern {};

struct HIRVariablePattern : HIRPattern {
    std::string_view name;  // interned
};

struct HIREnumPattern : HIRPattern {
    std::string_view variant_name;  // interned
};

struct HIRFieldBinding {
    std::string_view field_name;    // interned
    std::string_view binding_name;  // interned
    SourceLocation loc;
};

struct HIRRangePattern : HIRPattern {
    int64_t lo;
    int64_t hi;
    bool inclusive;  // true for lo..=hi, false for lo..hi
};

struct HIRUnionPattern : HIRPattern {
    std::string_view variant_name;      // interned
    HIRPattern* inner;                  // nullable: binding for single-value payload
    HIRFieldBinding* field_bindings;    // nullable: struct destructuring
    uint32_t field_binding_count;
};

// ============================================================================
// HIR Match Arms
// ============================================================================

struct HIRMatchArm {
    HIRPattern* pattern;
    HIRExpr* guard;   // nullable (M6+ guard expressions)
    HIRExpr* body;
    SourceLocation loc;
};

// ============================================================================
// HIR Expressions
// ============================================================================

struct HIRExpr {
    enum class Kind : uint8_t {
        IntLit, FloatLit, BoolLit, StringLit,
        Ident,
        BinOp, UnaryOp,
        Call,
        If,
        Match,
        Block,
        Return,
        StructLit,
        FieldAccess,
        EnumAccess,
        UnionVariant,
        AddrOf,
        Deref,
        Cast,
        Loop,
        Break,
        Continue,
        ArrayLit,
        IndexAccess,
        InlineAsm,
        FnRef,          // reference to a function → function pointer
        CallIndirect,   // indirect call through function pointer
    };

    Kind kind;
    TypeId type;             // every HIR node has a type
    SourceLocation loc;      // original AST source location
};

struct HIRIntLitExpr : HIRExpr {
    int64_t value;
};

struct HIRFloatLitExpr : HIRExpr {
    double value;
};

struct HIRBoolLitExpr : HIRExpr {
    bool value;
};

struct HIRStringLitExpr : HIRExpr {
    const char* data;
    uint32_t length;
};

struct HIRIdentExpr : HIRExpr {
    std::string_view name;  // interned
};

enum class HIRBinOp : uint8_t {
    Add, Sub, Mul, Div, Mod,
    AddWrap, SubWrap, MulWrap,    // wrapping: +%, -%, *%
    AddSat, SubSat,                // saturating: +|, -|
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    And, Or,
    BitAnd, BitOr, BitXor, Shl, Shr
};

struct HIRBinOpExpr : HIRExpr {
    HIRBinOp op;
    HIRExpr* lhs;
    HIRExpr* rhs;
};

enum class HIRUnaryOp : uint8_t {
    Neg, Not, BitNot, Deref, AddrOf, AddrOfVar
};

struct HIRUnaryOpExpr : HIRExpr {
    HIRUnaryOp op;
    HIRExpr* operand;
};

struct HIRCallExpr : HIRExpr {
    std::string_view callee;  // interned
    std::string_view callee_module;  // module path of callee (for cross-module calls)
    HIRExpr** args;
    uint32_t arg_count;
    bool is_tail_call;        // filled by TailCallAnalysisPass
    TypeId* type_args = nullptr;   // resolved type arguments (for generic calls)
    uint32_t type_arg_count = 0;
};

struct HIRIfExpr : HIRExpr {
    HIRExpr* condition;
    HIRExpr* then_branch;
    HIRExpr* else_branch;  // nullable (Unit type if absent)
};

struct HIRMatchExpr : HIRExpr {
    HIRExpr* scrutinee;
    HIRMatchArm* arms;
    uint32_t arm_count;
};

struct HIRBlockExpr : HIRExpr {
    HIRStmt** stmts;
    uint32_t stmt_count;
    HIRExpr* result;  // nullable (Unit if absent)
};

struct HIRReturnExpr : HIRExpr {
    HIRExpr* value;  // nullable
};

struct HIRFieldInit {
    std::string_view name;  // interned
    HIRExpr* value;
    SourceLocation loc;
};

struct HIRStructLitExpr : HIRExpr {
    std::string_view struct_name;  // interned
    HIRFieldInit* fields;
    uint32_t field_count;
};

struct HIRFieldAccessExpr : HIRExpr {
    HIRExpr* object;
    std::string_view field_name;  // interned
};

struct HIREnumAccessExpr : HIRExpr {
    std::string_view enum_name;     // interned
    std::string_view variant_name;  // interned
};

struct HIRUnionVariantExpr : HIRExpr {
    std::string_view union_name;    // interned
    std::string_view variant_name;  // interned
    HIRExpr* payload;               // nullable (empty variant)
};

struct HIRCastExpr : HIRExpr {
    HIRExpr* operand;
    TypeId target_type;  // type being cast to (same as this->type)
    bool is_explicit_truncate = false;  // set by truncate<T>/clamp<T> builtins
};

struct HIRLoopBinding {
    std::string_view name;  // interned
    TypeId type;
    HIRExpr* init;
    SourceLocation loc;
};

struct HIRLoopExpr : HIRExpr {
    HIRLoopBinding* bindings;
    uint32_t binding_count;
    HIRExpr* body;
    std::string_view label;  // 'label, empty = no label
};

struct HIRBreakExpr : HIRExpr {
    HIRExpr* value;   // nullable — break value
    std::string_view label;  // 'label target, empty = innermost
};

struct HIRContinueExpr : HIRExpr {
    HIRExpr** args;
    uint32_t arg_count;  // new accumulator values
    std::string_view label;  // 'label target, empty = innermost
};

struct HIRArrayLitExpr : HIRExpr {
    HIRExpr** elements;
    uint32_t element_count;
};

struct HIRIndexAccessExpr : HIRExpr {
    HIRExpr* array;
    HIRExpr* index;
};

struct HIRInlineAsmLine {
    const char* text;
    uint32_t length;
};

struct HIRAsmOperand {
    std::string_view constraint;   // "=r", "r", "a", etc.
    std::string_view var_name;     // variable name
};

struct HIRInlineAsmExpr : HIRExpr {
    HIRInlineAsmLine* lines;
    uint32_t line_count;
    HIRAsmOperand* outputs;
    uint32_t output_count;
    HIRAsmOperand* inputs;
    uint32_t input_count;
    std::string_view* clobbers;
    uint32_t clobber_count;
};

struct HIRFnRefExpr : HIRExpr {
    std::string_view fn_name;   // interned — name of the referenced function
    std::string_view fn_module; // module path of function (for cross-module refs)
};

struct HIRCallIndirectExpr : HIRExpr {
    HIRExpr* callee;            // expression yielding a function pointer
    HIRExpr** args;
    uint32_t arg_count;
    bool is_tail_call;
};

struct HIRAddrOfExpr : HIRExpr {
    HIRExpr* operand;
    bool is_mutable;  // &var vs &
};

struct HIRDerefExpr : HIRExpr {
    HIRExpr* operand;
};

// ============================================================================
// HIR Statements
// ============================================================================

struct HIRStmt {
    enum class Kind : uint8_t {
        ValDecl, VarDecl, ExprStmt, Assign, FieldAssign, DerefAssign, IndexAssign
    };
    Kind kind;
    SourceLocation loc;
};

struct HIRValDeclStmt : HIRStmt {
    std::string_view name;  // interned
    TypeId type;
    HIRExpr* init;
};

struct HIRVarDeclStmt : HIRStmt {
    std::string_view name;  // interned
    TypeId type;
    HIRExpr* init;
};

struct HIRExprStmt : HIRStmt {
    HIRExpr* expr;
};

struct HIRAssignStmt : HIRStmt {
    std::string_view name;  // interned
    HIRExpr* value;
};

struct HIRFieldAssignStmt : HIRStmt {
    HIRExpr* target;   // HIRFieldAccessExpr chain
    HIRExpr* value;
};

struct HIRDerefAssignStmt : HIRStmt {
    HIRExpr* target;   // deref or field via pointer
    HIRExpr* value;
};

struct HIRIndexAssignStmt : HIRStmt {
    HIRExpr* array;
    HIRExpr* index;
    HIRExpr* value;
};

// ============================================================================
// HIR Declarations
// ============================================================================

enum class Purity : uint8_t {
    Pure,
    ImpureMut,
    ImpureIo,
    ImpureMem,
    Unknown
};

inline const char* purityName(Purity p) {
    switch (p) {
        case Purity::Pure:      return "pure";
        case Purity::ImpureMut: return "impure(mut)";
        case Purity::ImpureIo:  return "impure(io)";
        case Purity::ImpureMem: return "impure(mem)";
        case Purity::Unknown:   return "unknown";
    }
    return "?";
}

struct FunctionMeta {
    Purity purity = Purity::Unknown;
    bool is_recursive = false;
    bool is_tailrec = false;
};

struct HIRParam {
    std::string_view name;  // interned
    TypeId type;
    SourceLocation loc;
    uint8_t passing_mode = 0;  // PassingMode::Borrow (0=Borrow, 1=MutBorrow, 2=Own)
};

struct HIRTypeParam {
    std::string_view name;  // interned
    TypeId type_var_id;     // TypeVar TypeId in TypeTable (or const param's type for const generics)
    bool is_const = false;  // true for `const N: u64`
    TypeId const_type = INVALID_TYPE;  // resolved type of the const param (e.g. U64)
};

struct HIRFnDecl {
    std::string_view name;  // interned
    HIRParam* params;
    uint32_t param_count;
    TypeId return_type;
    HIRExpr* body;           // nullable (intrinsic / extern)

    // Generic type parameters
    HIRTypeParam* type_params = nullptr;
    uint32_t type_param_count = 0;

    // Metadata (filled by passes)
    uint8_t purity;          // Purity enum value (stored as uint8_t to avoid circular include)
    bool is_recursive;
    bool is_tail_recursive;
    bool is_intrinsic;
    bool is_const;           // @const fn — compile-time evaluable
    bool is_naked;           // @naked — skip prologue/epilogue
    bool is_interrupt;       // @interrupt — iretq return, save all regs
    bool is_inline = false;  // @inline — hint to inline
    bool is_noinline = false;// @noinline — prevent inlining
    bool is_noreturn = false;// @noreturn — function never returns
    bool is_pub = false;     // pub fn — visible to other modules
    bool is_extern = false;  // extern "C" fn — external C linkage
    bool is_variadic = false; // fn(x: T, ...) — C-style varargs
    bool is_weak = false;     // @weak — weak symbol linkage
    std::string_view section_name;  // @section("name"), empty = default
    std::string_view link_name;     // @link_name("name"), empty = auto

    // Effect system (v2)
    EffectSet declared_effects = EFFECT_NONE;  // from "with io, atomic" annotation
    EffectSet inferred_effects = EFFECT_NONE;  // computed by EffectAnalysisPass
    bool has_effect_annotation = false;        // true if "with ..." clause present

    SourceLocation loc;
};

struct HIRStructDecl {
    std::string_view name;  // interned
    TypeId type_id;         // registered in TypeTable
    SourceLocation loc;
};

struct HIREnumDecl {
    std::string_view name;  // interned
    TypeId type_id;         // registered in TypeTable
    SourceLocation loc;
};

struct HIRUnionDecl {
    std::string_view name;  // interned
    TypeId type_id;         // registered in TypeTable
    SourceLocation loc;
};

struct HIRGlobalDecl {
    std::string_view name;  // interned
    TypeId type_id;
    HIRExpr* init;          // initializer expression (must be constant)
    bool is_mutable;        // static var vs static val
    bool is_extern = false; // extern static — linker-defined symbol
    std::string_view section_name;  // @section("name"), empty = default
    std::string_view link_name;     // @link_name("name"), empty = use name
    SourceLocation loc;
};

// ============================================================================
// HIR Module (top-level container)
// ============================================================================

struct HIRModule {
    std::string_view module_name;  // "kern.memory" or empty for single-file

    HIRFnDecl** functions;
    uint32_t fn_count;

    HIRStructDecl** structs;
    uint32_t struct_count;

    HIREnumDecl** enums;
    uint32_t enum_count;

    HIRUnionDecl** unions;
    uint32_t union_count;

    HIRGlobalDecl** globals;
    uint32_t global_count;

    // dyn Trait vtable globals
    struct VTableEntry {
        std::string_view label;      // vtable global label (interned)
        std::string_view* fn_labels; // ordered fn labels for vtable slots
        uint32_t method_count;
    };
    VTableEntry* vtables;
    uint32_t vtable_count;
};

} // namespace kern
