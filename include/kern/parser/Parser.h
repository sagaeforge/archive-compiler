#pragma once
#include "kern/parser/AST.h"
#include "kern/lexer/Lexer.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kern {

class Parser {
public:
    Parser(Lexer& lexer, Arena& arena, DiagnosticEngine& diag);

    // Set a cfg key-value pair for conditional compilation
    void setCfg(std::string_view key, std::string_view value);
    void setCfg(std::string_view key);  // boolean flag (key exists)

    Module* parseModule();

private:
    // Token management
    Token peek();
    Token advance();
    Token expect(TokenKind kind, const char* message);
    bool check(TokenKind kind);
    bool match(TokenKind kind);
    void skipNewlines();

    // Declarations
    FnDecl* parseFnDecl();
    StructDecl* parseStructDecl();
    EnumDecl* parseEnumDecl();
    UnionDecl* parseUnionDecl();
    TypeAliasDecl* parseTypeAlias();
    NewtypeDecl* parseNewtype();
    TraitDecl* parseTraitDecl();
    ImplDecl* parseImplDecl();
    GlobalDecl* parseGlobalDecl();
    Param parseParam();

    // Types
    TypeRef parseType();

    // Expressions (Pratt parsing)
    Expr* parseExpr(uint8_t minBP = 0);
    Expr* parseExprInfix(Expr* lhs, uint8_t minBP = 0);
    Expr* parsePrimary();
    Expr* parseIfExpr();
    Expr* parseBlockExpr();
    Expr* parseCallExpr(std::string_view name, SourceLocation loc);
    Expr* parseStructLit(std::string_view name, SourceLocation loc);
    Expr* parseMatchExpr();
    Expr* parseLoopExpr();
    Expr* parseForRange();
    Expr* parseWhileLoop();
    Expr* parseLambdaExpr(SourceLocation loc);
    bool isLambdaStart();
    Pattern* parsePattern();

    // Statements
    Stmt* parseValDecl();

    // Helpers
    static bool isDerefTarget(const Expr* expr);
    bool isCompoundAssign();
    BinOpKind compoundAssignOp();
    Expr* wrapCompoundAssign(Expr* lhs, Expr* rhs, BinOpKind op, SourceLocation loc);

    // Operator helpers
    static uint8_t prefixBP(TokenKind kind);
    struct InfixBP { uint8_t left; uint8_t right; };
    static InfixBP infixBP(TokenKind kind);
    static BinOpKind tokenToBinOp(TokenKind kind);

    Lexer& lexer_;
    Arena& arena_;
    DiagnosticEngine& diag_;

    Token current_;
    Token previous_;
    bool has_current_ = false;

    std::unordered_set<std::string_view> struct_names_;
    std::unordered_set<std::string_view> enum_names_;
    std::unordered_set<std::string_view> union_names_;

    // Cfg flags for conditional compilation (@cfg)
    std::unordered_map<std::string_view, std::string_view> cfg_flags_;

    // Evaluate a @cfg condition. Returns true if item should be included.
    bool evaluateCfg();

    // Skip a top-level item (fn, struct, enum, union, etc.)
    void skipTopLevelItem();
};

} // namespace kern
