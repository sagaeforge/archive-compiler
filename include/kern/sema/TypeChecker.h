#pragma once
#include "kern/parser/AST.h"
#include "kern/support/Diagnostic.h"
#include <string>
#include <unordered_map>

namespace kern {

// Simplified type system for M1 — only i64 and bool
enum class Type { I64, Bool, Unit, Error };

const char* typeName(Type t);

struct FnSig {
    std::string_view name;
    std::vector<Type> param_types;
    Type return_type;
};

class TypeChecker {
public:
    explicit TypeChecker(DiagnosticEngine& diag);

    bool check(Module* mod);

private:
    Type checkFn(FnDecl* fn);
    Type checkExpr(Expr* expr);
    Type checkBlock(BlockExpr* block);
    void checkStmt(Stmt* stmt);

    Type resolveType(const TypeRef& ref);

    DiagnosticEngine& diag_;
    std::unordered_map<std::string_view, FnSig> fn_table_;
    std::unordered_map<std::string_view, Type> local_vars_;
    Type current_return_type_ = Type::Error;
};

} // namespace kern
