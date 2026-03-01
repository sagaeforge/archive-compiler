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
    enum class Kind { Named, Ptr, Fn };
    Kind kind = Kind::Named;
    std::string_view name; // "i64", "bool", "Unit"
};

// --- Parameter ---
struct Param {
    std::string_view name;
    TypeRef type;
    SourceLocation loc;
};

// --- Expressions ---
struct Expr {
    enum class Kind {
        IntLit, BoolLit, Ident,
        BinOp, UnaryOp, Call,
        If, Block, Return
    };
    Kind kind;
    SourceLocation loc;
};

struct IntLitExpr : Expr {
    int64_t value;
};

struct BoolLitExpr : Expr {
    bool value;
};

struct IdentExpr : Expr {
    std::string_view name;
};

enum class BinOpKind {
    Add, Sub, Mul, Div,
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    And, Or
};

struct BinOpExpr : Expr {
    BinOpKind op;
    Expr* lhs;
    Expr* rhs;
};

struct UnaryOpKind_t {
    enum Value { Neg, Not, Deref, AddrOf };
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

// --- Statements ---
struct Stmt {
    enum class Kind { ValDecl, VarDecl, ExprStmt };
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

// --- Declarations ---
struct FnDecl {
    std::string_view name;
    Param* params;
    uint32_t param_count;
    TypeRef return_type;
    Expr* body; // BlockExpr
    SourceLocation loc;
};

// --- Module ---
struct Module {
    FnDecl** functions;
    uint32_t fn_count;
};

// AST dumper
void dumpAST(const Module* mod, std::ostream& out, int indent = 0);
void dumpExpr(const Expr* expr, std::ostream& out, int indent = 0);

} // namespace kern
