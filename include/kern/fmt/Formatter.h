#pragma once
#include "kern/parser/AST.h"
#include <ostream>

namespace kern {

struct FormatOptions {
    uint32_t indent_width = 4;
    uint32_t max_line_width = 100;
};

class Formatter {
    std::ostream& out_;
    FormatOptions opts_;
    int indent_ = 0;

public:
    Formatter(std::ostream& out, FormatOptions opts = {})
        : out_(out), opts_(opts) {}

    void formatModule(const Module* mod);

private:
    void formatFnDecl(const FnDecl* fn);
    void formatStructDecl(const StructDecl* s);
    void formatEnumDecl(const EnumDecl* e);
    void formatUnionDecl(const UnionDecl* u);
    void formatTypeRef(const TypeRef& t);
    void formatExpr(const Expr* expr);
    void formatStmt(const Stmt* stmt);
    void formatBlock(const BlockExpr* block, bool is_fn_body = false);
    void formatPattern(const Pattern* pat);
    void formatMatchArm(const MatchArm& arm);

    void newline();
    void writeIndent();
    void indent() { indent_ += opts_.indent_width; }
    void dedent() { indent_ -= opts_.indent_width; }

    static const char* binOpStr(BinOpKind op);
};

} // namespace kern
