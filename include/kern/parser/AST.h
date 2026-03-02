#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <string_view>
#include <ostream>

namespace kern {

// Forward declarations
struct Expr;
struct Stmt;
struct FnDecl;

// --- Type Reference ---
struct TypeRef {
    enum class Kind { Named, Ptr, Fn, Never, Array, ConstVal, Dyn, Tuple };
    Kind kind = Kind::Named;
    std::string_view name; // "i64", "bool", "Unit"
    SourceLocation loc;
    TypeRef* pointee = nullptr;  // for Ptr<T>: the inner type
    bool is_ptr_var = false;     // for Ptr<var T>: mutable pointer
    TypeRef* array_element = nullptr;  // for [T; N]: element type
    uint32_t array_size = 0;           // for [T; N]: count (0 when using const generic param)
    std::string_view array_size_name;  // for [T; N] with const generic: "N" (empty if literal)
    // For Fn type: fn(T1, T2) -> Ret
    TypeRef* fn_params = nullptr;      // arena array of param types
    uint32_t fn_param_count = 0;
    TypeRef* fn_return = nullptr;      // return type
    // Generic type arguments: Pair<i64>, Result<T, E>
    TypeRef* type_args = nullptr;
    uint32_t type_arg_count = 0;
    // For ConstVal: integer constant in type argument position (e.g. Buffer<i64, 4>)
    int64_t const_value = 0;
    // For Tuple: (T1, T2, ...)
    TypeRef* tuple_elements = nullptr;
    uint32_t tuple_count = 0;
};

// --- Passing Mode ---
enum class PassingMode : uint8_t {
    Borrow,      // default — immutable borrow
    MutBorrow,   // var Type — mutable borrow
    Own,         // own Type — ownership transfer
};

// --- Parameter ---
struct Param {
    std::string_view name;
    TypeRef type;
    SourceLocation loc;
    PassingMode mode = PassingMode::Borrow;
};

// --- Patterns (for match expressions) ---
struct Pattern {
    enum class Kind { IntLit, BoolLit, Wildcard, Variable, Enum, Union, Range };
    Kind kind;
    SourceLocation loc;
};

struct IntLitPattern : Pattern {
    int64_t value;
};

struct RangePattern : Pattern {
    int64_t lo;
    int64_t hi;
    bool inclusive;  // true for lo..=hi, false for lo..hi
};

struct BoolLitPattern : Pattern {
    bool value;
};

struct WildcardPattern : Pattern {};

struct VariablePattern : Pattern {
    std::string_view name;
};

struct EnumPattern : Pattern {
    std::string_view variant_name;    // e.g. "Red" in match Color
};

struct FieldBinding {
    std::string_view field_name;      // struct field name (e.g. "radius")
    std::string_view binding_name;    // variable to bind to (e.g. "r")
    SourceLocation loc;
};

struct UnionPattern : Pattern {
    std::string_view variant_name;    // e.g. "Some" or "Circle"
    Pattern* inner;                   // binding pattern for single-value payload (nullptr for empty)
    FieldBinding* field_bindings;     // struct destructuring bindings (nullptr if not used)
    uint32_t field_binding_count;     // number of field bindings
};

// --- Type parameter (for generics) ---
struct TypeParam {
    std::string_view name;   // e.g. "T" or "N" (const generic)
    SourceLocation loc;
    bool is_const = false;   // true for `const N: u64`
    TypeRef const_type{};    // type of const param (e.g. u64), only valid if is_const
};

// --- Field declarations (for struct definitions) ---
struct FieldDecl {
    std::string_view name;
    TypeRef type;
    bool is_mutable;
    bool is_pub = true;         // pub field — accessible from other modules (default: true)
    uint32_t bit_width = 0;     // 0 = full-width field, >0 = bitfield width in bits
    SourceLocation loc;
};

// --- Struct declaration ---
struct StructDecl {
    std::string_view name;
    FieldDecl* fields;
    uint32_t field_count;
    SourceLocation loc;
    bool is_packed = false;         // @packed
    bool is_repr_c = false;         // @repr(C) — C ABI-compatible layout
    bool is_pub = false;            // pub struct
    uint32_t explicit_align = 0;   // @align(N), 0 = natural alignment
    TypeParam* type_params = nullptr;
    uint32_t type_param_count = 0;
};

// --- Enum declaration ---
struct EnumVariant {
    std::string_view name;
    SourceLocation loc;
    int64_t value = -1;      // explicit discriminant, -1 = auto-assigned
    bool has_value = false;
};

struct EnumDecl {
    std::string_view name;
    EnumVariant* variants;
    uint32_t variant_count;
    SourceLocation loc;
    bool is_pub = false;            // pub enum
    uint8_t backing_size = 8;       // @repr(u8)=1, @repr(u16)=2, @repr(u32)=4, default=8
};

// --- Union declaration ---
struct UnionVariantDecl {
    std::string_view name;
    TypeRef* payload_type;    // nullptr for empty variants (e.g., None, Empty)
    SourceLocation loc;
};

struct UnionDecl {
    std::string_view name;
    UnionVariantDecl* variants;
    uint32_t variant_count;
    SourceLocation loc;
    bool is_pub = false;            // pub union
    bool is_repr_c = false;         // @repr(C) — untagged C-style union
    TypeParam* type_params = nullptr;
    uint32_t type_param_count = 0;
};

// --- Type alias ---
struct TypeAliasDecl {
    std::string_view name;
    TypeRef target;
    SourceLocation loc;
    bool is_pub = false;            // pub type
};

// --- Newtype ---
struct NewtypeDecl {
    std::string_view name;
    TypeRef inner;
    SourceLocation loc;
    bool is_pub = false;            // pub newtype
};

// --- Static assert ---
struct StaticAssertDecl {
    Expr* condition;
    std::string_view message;
    SourceLocation loc;
};

// --- Trait declaration ---
struct TraitMethodSig {
    std::string_view name;
    Param* params;
    uint32_t param_count;
    TypeRef return_type;
    SourceLocation loc;
    // Effect clause: "with io" → effect_names = {"io"}
    std::string_view* effect_names = nullptr;
    uint32_t effect_count = 0;
};

struct TraitDecl {
    std::string_view name;
    TraitMethodSig* methods;
    uint32_t method_count;
    SourceLocation loc;
    bool is_pub = false;            // pub trait
};

// --- Impl declaration ---
struct ImplDecl {
    std::string_view trait_name;
    TypeRef target_type;       // the type implementing the trait
    FnDecl** methods;
    uint32_t method_count;
    SourceLocation loc;
};

// --- Expressions ---
struct Expr {
    enum class Kind {
        IntLit, FloatLit, BoolLit, StringLit, NullLit, Ident,
        BinOp, UnaryOp, Call, Cast,
        If, Block, Return, Match,
        StructLit, FieldAccess,
        EnumAccess, UnionVariant,
        Loop, ForRange, ForEach, WhileLoop, InlineAsm,
        ArrayLit, IndexAccess, SliceExpr,
        Sizeof, Alignof, Offsetof,
        Lambda, MethodCall,
        Try, TupleLit, Uninit
    };
    Kind kind;
    SourceLocation loc;
};

struct IntLitExpr : Expr {
    int64_t value;
    std::string_view suffix;  // "u8", "i32", etc. (empty = no suffix)
};

struct FloatLitExpr : Expr {
    double value;
    bool is_f32;
};

struct BoolLitExpr : Expr {
    bool value;
};

struct NullLitExpr : Expr {};

struct StringLitExpr : Expr {
    const char* data;    // processed bytes (escape sequences resolved)
    uint32_t length;     // byte length of processed data
};

struct IdentExpr : Expr {
    std::string_view name;
};

enum class BinOpKind {
    Add, Sub, Mul, Div, Mod,
    AddWrap, SubWrap, MulWrap,    // wrapping: +%, -%, *%
    AddSat, SubSat,                // saturating: +|, -|
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    And, Or,
    BitAnd, BitOr, BitXor, Shl, Shr
};

struct BinOpExpr : Expr {
    BinOpKind op;
    Expr* lhs;
    Expr* rhs;
};

struct UnaryOpKind_t {
    enum Value { Neg, Not, BitNot, Deref, AddrOf, AddrOfVar };
};
using UnaryOpKind = UnaryOpKind_t::Value;

struct UnaryOpExpr : Expr {
    UnaryOpKind op;
    Expr* operand;
};

struct CallExpr : Expr {
    std::string_view callee;
    Expr** args;
    uint32_t arg_count;
    TypeRef* type_args = nullptr;    // explicit type arguments: f<i64>(x)
    uint32_t type_arg_count = 0;
};

struct IfExpr : Expr {
    Expr* condition;
    Expr* then_branch;
    Expr* else_branch; // may be nullptr
};

struct BlockExpr : Expr {
    Stmt** stmts;
    uint32_t stmt_count;
    Expr* result; // final expression (may be nullptr for Unit)
};

struct ReturnExpr : Expr {
    Expr* value; // may be nullptr
};

struct MatchArm {
    Pattern* pattern;
    Expr* guard;  // may be nullptr
    Expr* body;
    SourceLocation loc;
};

struct MatchExpr : Expr {
    Expr* scrutinee;
    MatchArm* arms;
    uint32_t arm_count;
};

struct FieldInit {
    std::string_view name;
    Expr* value;
    SourceLocation loc;
};

struct StructLitExpr : Expr {
    std::string_view struct_name;
    FieldInit* fields;
    uint32_t field_count;
};

struct FieldAccessExpr : Expr {
    Expr* object;
    std::string_view field_name;
};

struct EnumAccessExpr : Expr {
    std::string_view enum_name;       // "Color"
    std::string_view variant_name;    // "Red"
};

struct UnionVariantExpr : Expr {
    std::string_view union_name;      // "Shape"
    std::string_view variant_name;    // "Circle"
    Expr* payload;                    // nullptr for empty variants (e.g., Shape::Empty)
};

struct ArrayLitExpr : Expr {
    Expr** elements;
    uint32_t count;
};

struct IndexAccessExpr : Expr {
    Expr* array;
    Expr* index;
};

struct SliceExprNode : Expr {
    Expr* array;
    Expr* start;   // nullable (arr[..end] means start=0)
    Expr* end;     // nullable (arr[start..] means end=len)
};

struct TupleLitExpr : Expr {
    Expr** elements;
    uint32_t count;
};

struct CastExpr : Expr {
    Expr* operand;
    TypeRef target;
};

struct SizeofExpr : Expr {
    TypeRef target;
};

struct AlignofExpr : Expr {
    TypeRef target;
};

struct OffsetofExpr : Expr {
    TypeRef target;
    std::string_view field_name;
};

struct LambdaExpr : Expr {
    Param* params;
    uint32_t param_count;
    TypeRef return_type;          // optional, can be Named with empty name if inferred
    Expr* body;
};

struct MethodCallExpr : Expr {
    Expr* object;
    std::string_view method_name;
    Expr** args;
    uint32_t arg_count;
};

struct TryExpr : Expr {
    Expr* operand;
};

struct LoopBinding {
    std::string_view name;
    Expr* init;
};

struct LoopExpr : Expr {
    LoopBinding* bindings;
    uint32_t binding_count;
    Stmt** stmts;
    uint32_t stmt_count;
    Expr* result;  // nullptr for Unit-returning loops
    std::string_view label;  // 'label, empty = no label
};

// for i in start..end { body }
struct ForRangeExpr : Expr {
    std::string_view var_name;  // loop variable name
    Expr* start;                // range start (inclusive)
    Expr* end;                  // range end (exclusive)
    Stmt** stmts;
    uint32_t stmt_count;
    std::string_view label;     // 'label, empty = no label
};

// for item in collection { body }
struct ForEachExpr : Expr {
    std::string_view var_name;  // loop variable name
    Expr* collection;           // array or slice expression
    Stmt** stmts;
    uint32_t stmt_count;
    std::string_view label;     // 'label, empty = no label
};

// while cond { body }
struct WhileLoopExpr : Expr {
    Expr* condition;
    Stmt** stmts;
    uint32_t stmt_count;
    std::string_view label;  // 'label, empty = no label
};

// ASM operand constraint: "=r"(var) / "r"(expr) / "+a"(var)
struct AsmOperand {
    std::string_view constraint;   // "=r", "r", "m", "a", "=a", etc.
    std::string_view var_name;     // variable name for output / input
};

struct InlineAsmExpr : Expr {
    StringLitExpr** lines;
    uint32_t line_count;
    // Extended asm (GCC-style constraints)
    AsmOperand* outputs;            // "=r"(var) bindings
    uint32_t output_count;
    AsmOperand* inputs;             // "r"(var) bindings
    uint32_t input_count;
    std::string_view* clobbers;     // "cc", "memory", register names
    uint32_t clobber_count;
};

// --- Statements ---
struct Stmt {
    enum class Kind { ValDecl, VarDecl, ExprStmt, Assign, FieldAssign, DerefAssign, Break, Continue, IndexAssign };
    Kind kind;
    SourceLocation loc;
};

struct ValDeclStmt : Stmt {
    std::string_view name;
    TypeRef type;
    Expr* init;
};

struct VarDeclStmt : Stmt {
    std::string_view name;
    TypeRef type;
    Expr* init;
};

struct ExprStmt : Stmt {
    Expr* expr;
};

struct AssignStmt : Stmt {
    std::string_view name;
    Expr* value;
};

struct FieldAssignStmt : Stmt {
    Expr* target;   // FieldAccessExpr chain (e.g. s.pos.x)
    Expr* value;
};

struct DerefAssignStmt : Stmt {
    Expr* target;   // UnaryOpExpr(Deref) or FieldAccessExpr via (*ptr).field
    Expr* value;
};

struct BreakStmt : Stmt {
    Expr* value;  // nullptr for unit break
    std::string_view label;  // 'label target, empty = innermost loop
};

struct ContinueStmt : Stmt {
    Expr** args;
    uint32_t arg_count;
    std::string_view label;  // 'label target, empty = innermost loop
};

struct IndexAssignStmt : Stmt {
    Expr* array;
    Expr* index;
    Expr* value;
};

// --- Declarations ---
struct FnDecl {
    std::string_view name;
    Param* params;
    uint32_t param_count;
    TypeRef return_type;
    Expr* body; // BlockExpr (nullptr for intrinsics and extern)
    SourceLocation loc;
    bool is_intrinsic = false;
    bool is_const = false;           // const fn — compile-time evaluable
    bool is_naked = false;           // @naked annotation
    bool is_interrupt = false;       // @interrupt annotation
    bool is_inline = false;          // @inline — hint to inline
    bool is_noinline = false;        // @noinline — prevent inlining
    bool is_noreturn = false;        // @noreturn — function never returns
    bool is_pub = false;             // pub fn — visible to other modules
    bool is_extern = false;          // extern "C" fn — external linkage
    bool is_variadic = false;        // fn(x: i64, ...) — C-style varargs
    bool has_pattern_params = false; // function-level pattern matching
    Pattern* pattern_param = nullptr; // pattern for first param (when has_pattern_params)
    TypeParam* type_params = nullptr; // generic type parameters (<T, U>)
    uint32_t type_param_count = 0;
    std::string_view section_name;    // @section("name"), empty = default
    std::string_view link_name;       // @link_name("name"), empty = auto
    std::string_view extern_abi;      // "C" for extern "C", empty otherwise
    bool is_weak = false;             // @weak — weak symbol linkage
    bool is_no_mangle = false;       // @no_mangle — suppress module name mangling
    // Effect clause: "with io, atomic" → effect_names = {"io", "atomic"}
    std::string_view* effect_names = nullptr;
    uint32_t effect_count = 0;
    bool has_effect_clause = false;  // true if "with ..." clause present (including "with pure")
};

// --- Global variable declaration ---
struct GlobalDecl {
    std::string_view name;
    TypeRef type;
    Expr* init;              // initializer expression (nullptr for .bss)
    bool is_mutable;         // true = "static var", false = "static val"
    bool is_pub = false;     // pub static
    bool is_extern = false;  // extern static — linker-defined symbol
    std::string_view section_name;  // @section("name"), empty = default
    std::string_view link_name;  // @link_name("name"), empty = use name as-is
    SourceLocation loc;
};

// --- Import declaration ---
struct ImportDecl {
    std::string_view module_path;   // "kern.types"
    std::string_view* names;        // imported names: (PhysAddr, VirtAddr)
    uint32_t name_count;
    bool is_pub = false;            // pub import — re-export
    SourceLocation loc;
};

// --- Module ---
struct Module {
    std::string_view module_name;   // "kern.memory" or empty
    ImportDecl** imports;
    uint32_t import_count;
    FnDecl** functions;
    uint32_t fn_count;
    StructDecl** structs;
    uint32_t struct_count;
    EnumDecl** enums;
    uint32_t enum_count;
    UnionDecl** unions;
    uint32_t union_count;
    TypeAliasDecl** type_aliases;
    uint32_t type_alias_count;
    NewtypeDecl** newtypes;
    uint32_t newtype_count;
    StaticAssertDecl** static_asserts;
    uint32_t static_assert_count;
    TraitDecl** traits;
    uint32_t trait_count;
    ImplDecl** impls;
    uint32_t impl_count;
    GlobalDecl** globals;
    uint32_t global_count;
};

// AST dumper
void dumpAST(const Module* mod, std::ostream& out, int indent = 0);
void dumpExpr(const Expr* expr, std::ostream& out, int indent = 0);

} // namespace kern
