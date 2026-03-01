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
        IntLit, BoolLit, Wildcard, Variable, Enum, Union
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
    Add, Sub, Mul, Div,
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    And, Or
};

struct HIRBinOpExpr : HIRExpr {
    HIRBinOp op;
    HIRExpr* lhs;
    HIRExpr* rhs;
};

enum class HIRUnaryOp : uint8_t {
    Neg, Not, Deref, AddrOf, AddrOfVar
};

struct HIRUnaryOpExpr : HIRExpr {
    HIRUnaryOp op;
    HIRExpr* operand;
};

struct HIRCallExpr : HIRExpr {
    std::string_view callee;  // interned
    HIRExpr** args;
    uint32_t arg_count;
    bool is_tail_call;        // filled by TailCallAnalysisPass
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
        ValDecl, VarDecl, ExprStmt, Assign, FieldAssign, DerefAssign
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

// ============================================================================
// HIR Declarations
// ============================================================================

enum class Purity : uint8_t;  // forward decl from Metadata.h

struct HIRParam {
    std::string_view name;  // interned
    TypeId type;
    SourceLocation loc;
};

struct HIRFnDecl {
    std::string_view name;  // interned
    HIRParam* params;
    uint32_t param_count;
    TypeId return_type;
    HIRExpr* body;           // nullable (intrinsic)

    // Metadata (filled by passes)
    uint8_t purity;          // Purity enum value (stored as uint8_t to avoid circular include)
    bool is_recursive;
    bool is_tail_recursive;
    bool is_intrinsic;

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

// ============================================================================
// HIR Module (top-level container)
// ============================================================================

struct HIRModule {
    HIRFnDecl** functions;
    uint32_t fn_count;

    HIRStructDecl** structs;
    uint32_t struct_count;

    HIREnumDecl** enums;
    uint32_t enum_count;

    HIRUnionDecl** unions;
    uint32_t union_count;
};

} // namespace kern
