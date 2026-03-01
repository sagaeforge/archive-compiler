#pragma once
#include "kern/support/SourceLocation.h"
#include <cstdint>
#include <string_view>
#include <ostream>

namespace kern {

// Forward declarations
struct Expr;
struct Stmt;

// --- Type Reference ---
struct TypeRef {
    enum class Kind { Named, Ptr, Fn, Never, Array };
    Kind kind = Kind::Named;
    std::string_view name; // "i64", "bool", "Unit"
    SourceLocation loc;
    TypeRef* pointee = nullptr;  // for Ptr<T>: the inner type
    bool is_ptr_var = false;     // for Ptr<var T>: mutable pointer
    TypeRef* array_element = nullptr;  // for [T; N]: element type
    uint32_t array_size = 0;           // for [T; N]: count
    // For Fn type: fn(T1, T2) -> Ret
    TypeRef* fn_params = nullptr;      // arena array of param types
    uint32_t fn_param_count = 0;
    TypeRef* fn_return = nullptr;      // return type
    // Generic type arguments: Pair<i64>, Result<T, E>
    TypeRef* type_args = nullptr;
    uint32_t type_arg_count = 0;
};

// --- Parameter ---
struct Param {
    std::string_view name;
    TypeRef type;
    SourceLocation loc;
};

// --- Patterns (for match expressions) ---
struct Pattern {
    enum class Kind { IntLit, BoolLit, Wildcard, Variable, Enum, Union };
    Kind kind;
    SourceLocation loc;
};

struct IntLitPattern : Pattern {
    int64_t value;
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
    std::string_view name;   // e.g. "T"
    SourceLocation loc;
};

// --- Field declarations (for struct definitions) ---
struct FieldDecl {
    std::string_view name;
    TypeRef type;
    bool is_mutable;
    SourceLocation loc;
};

// --- Struct declaration ---
struct StructDecl {
    std::string_view name;
    FieldDecl* fields;
    uint32_t field_count;
    SourceLocation loc;
    bool is_packed = false;         // @packed
    uint32_t explicit_align = 0;   // @align(N), 0 = natural alignment
    TypeParam* type_params = nullptr;
    uint32_t type_param_count = 0;
};

// --- Enum declaration ---
struct EnumVariant {
    std::string_view name;
    SourceLocation loc;
};

struct EnumDecl {
    std::string_view name;
    EnumVariant* variants;
    uint32_t variant_count;
    SourceLocation loc;
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
    TypeParam* type_params = nullptr;
    uint32_t type_param_count = 0;
};

// --- Type alias ---
struct TypeAliasDecl {
    std::string_view name;
    TypeRef target;
    SourceLocation loc;
};

// --- Newtype ---
struct NewtypeDecl {
    std::string_view name;
    TypeRef inner;
    SourceLocation loc;
};

// --- Static assert ---
struct StaticAssertDecl {
    Expr* condition;
    std::string_view message;
    SourceLocation loc;
};

// --- Expressions ---
struct Expr {
    enum class Kind {
        IntLit, FloatLit, BoolLit, StringLit, Ident,
        BinOp, UnaryOp, Call, Cast,
        If, Block, Return, Match,
        StructLit, FieldAccess,
        EnumAccess, UnionVariant,
        Loop, InlineAsm,
        ArrayLit, IndexAccess,
        Sizeof, Alignof,
        Lambda
    };
    Kind kind;
    SourceLocation loc;
};

struct IntLitExpr : Expr {
    int64_t value;
};

struct FloatLitExpr : Expr {
    double value;
    bool is_f32;
};

struct BoolLitExpr : Expr {
    bool value;
};

struct StringLitExpr : Expr {
    const char* data;    // processed bytes (escape sequences resolved)
    uint32_t length;     // byte length of processed data
};

struct IdentExpr : Expr {
    std::string_view name;
};

enum class BinOpKind {
    Add, Sub, Mul, Div, Mod,
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

struct LambdaExpr : Expr {
    Param* params;
    uint32_t param_count;
    TypeRef return_type;          // optional, can be Named with empty name if inferred
    Expr* body;
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
};

struct InlineAsmExpr : Expr {
    StringLitExpr** lines;
    uint32_t line_count;
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
};

struct ContinueStmt : Stmt {
    Expr** args;
    uint32_t arg_count;
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
    Expr* body; // BlockExpr (nullptr for intrinsics)
    SourceLocation loc;
    bool is_intrinsic = false;
    bool is_const = false;           // const fn — compile-time evaluable
    bool is_naked = false;           // @naked annotation
    bool is_interrupt = false;       // @interrupt annotation
    bool has_pattern_params = false; // function-level pattern matching
    Pattern* pattern_param = nullptr; // pattern for first param (when has_pattern_params)
    TypeParam* type_params = nullptr; // generic type parameters (<T, U>)
    uint32_t type_param_count = 0;
};

// --- Module ---
struct Module {
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
};

// AST dumper
void dumpAST(const Module* mod, std::ostream& out, int indent = 0);
void dumpExpr(const Expr* expr, std::ostream& out, int indent = 0);

} // namespace kern
