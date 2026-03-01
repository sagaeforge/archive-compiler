#pragma once
#include "kern/parser/AST.h"
#include "kern/support/Diagnostic.h"
#include <string>
#include <unordered_map>

namespace kern {

enum class Type {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    Bool, Unit, Error
};

const char* typeName(Type t);
bool isInteger(Type t);
bool isSigned(Type t);
bool isUnsigned(Type t);
int bitWidth(Type t);

struct FnSig {
    std::string_view name;
    std::vector<Type> param_types;
    Type return_type;
};

class TypeChecker {
public:
    explicit TypeChecker(DiagnosticEngine& diag);

    bool check(Module* mod);

    // Query the type of an expression (after check() has run)
    Type typeOfExpr(const Expr* expr) const;

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

    // Memoized expression types (Expr* -> Type)
    std::unordered_map<const Expr*, Type> expr_types_;
};

} // namespace kern
