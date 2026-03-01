#pragma once
#include "kern/parser/AST.h"
#include "kern/lexer/Lexer.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <vector>

namespace kern {

class Parser {
public:
    Parser(Lexer& lexer, Arena& arena, DiagnosticEngine& diag);

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
    Param parseParam();

    // Types
    TypeRef parseType();

    // Expressions (Pratt parsing)
    Expr* parseExpr(uint8_t minBP = 0);
    Expr* parsePrimary();
    Expr* parseIfExpr();
    Expr* parseBlockExpr();
    Expr* parseCallExpr(std::string_view name, SourceLocation loc);

    // Statements
    Stmt* parseStmt();
    Stmt* parseValDecl();

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
};

} // namespace kern
