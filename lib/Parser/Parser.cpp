#include "kern/parser/Parser.h"
#include <cstring>
#include <sstream>
#include <iostream>

namespace kern {

// Strip integer type suffix (u8, u16, u32, u64, i8, i16, i32, i64) from text
static std::string_view stripIntSuffix(std::string_view text) {
    // Check for 3-char suffixes: u16, u32, u64, i16, i32, i64
    if (text.size() >= 4) {
        auto tail3 = text.substr(text.size() - 3);
        if (tail3 == "u16" || tail3 == "u32" || tail3 == "u64" ||
            tail3 == "i16" || tail3 == "i32" || tail3 == "i64") {
            return text.substr(0, text.size() - 3);
        }
    }
    // Check for 2-char suffixes: u8, i8
    if (text.size() >= 3) {
        auto tail2 = text.substr(text.size() - 2);
        if (tail2 == "u8" || tail2 == "i8") {
            return text.substr(0, text.size() - 2);
        }
    }
    return text;
}

// Extract integer type suffix as a string (empty if none)
static std::string_view intSuffix(std::string_view text) {
    auto stripped = stripIntSuffix(text);
    if (stripped.size() < text.size()) {
        return text.substr(stripped.size());
    }
    return {};
}

// Strip underscore separators from integer literal text
static std::string stripUnderscores(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        if (c != '_') result += c;
    }
    return result;
}

// Parse integer literal text (hex, binary, octal, decimal) to int64
static int64_t parseIntText(std::string_view text) {
    text = stripIntSuffix(text);
    std::string clean = stripUnderscores(text);
    if (clean.size() > 2 && clean[0] == '0') {
        char prefix = clean[1];
        if (prefix == 'x' || prefix == 'X')
            return static_cast<int64_t>(std::stoull(clean, nullptr, 16));
        if (prefix == 'b' || prefix == 'B')
            return static_cast<int64_t>(std::stoull(clean.substr(2), nullptr, 2));
        if (prefix == 'o' || prefix == 'O')
            return static_cast<int64_t>(std::stoull(clean.substr(2), nullptr, 8));
    }
    return static_cast<int64_t>(std::stoull(clean));
}

// --- AST Dump ---
static void indent(std::ostream& out, int level) {
    for (int i = 0; i < level; ++i) out << "  ";
}

void dumpExpr(const Expr* expr, std::ostream& out, int ind) {
    if (!expr) { indent(out, ind); out << "(null)\n"; return; }
    indent(out, ind);
    switch (expr->kind) {
        case Expr::Kind::IntLit:
            out << "IntLit(" << static_cast<const IntLitExpr*>(expr)->value << ")\n";
            break;
        case Expr::Kind::FloatLit: {
            auto* fl = static_cast<const FloatLitExpr*>(expr);
            out << "FloatLit(" << fl->value << (fl->is_f32 ? "f" : "") << ")\n";
            break;
        }
        case Expr::Kind::BoolLit:
            out << "BoolLit(" << (static_cast<const BoolLitExpr*>(expr)->value ? "true" : "false") << ")\n";
            break;
        case Expr::Kind::NullLit:
            out << "NullLit\n";
            break;
        case Expr::Kind::StringLit: {
            auto* sl = static_cast<const StringLitExpr*>(expr);
            out << "StringLit(\"";
            for (uint32_t i = 0; i < sl->length; ++i) {
                char c = sl->data[i];
                if (c == '\n') out << "\\n";
                else if (c == '\t') out << "\\t";
                else if (c == '\\') out << "\\\\";
                else if (c == '"') out << "\\\"";
                else out << c;
            }
            out << "\")\n";
            break;
        }
        case Expr::Kind::Ident:
            out << "Ident(" << static_cast<const IdentExpr*>(expr)->name << ")\n";
            break;
        case Expr::Kind::BinOp: {
            auto* b = static_cast<const BinOpExpr*>(expr);
            const char* ops[] = {"+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=", "and", "or", "&", "|", "^", "<<", ">>"};
            out << "BinOp(" << ops[static_cast<int>(b->op)] << ")\n";
            dumpExpr(b->lhs, out, ind + 1);
            dumpExpr(b->rhs, out, ind + 1);
            break;
        }
        case Expr::Kind::UnaryOp: {
            auto* u = static_cast<const UnaryOpExpr*>(expr);
            const char* ops[] = {"-", "not", "~", "*", "&", "&var"};
            out << "UnaryOp(" << ops[static_cast<int>(u->op)] << ")\n";
            dumpExpr(u->operand, out, ind + 1);
            break;
        }
        case Expr::Kind::Call: {
            auto* c = static_cast<const CallExpr*>(expr);
            out << "Call(" << c->callee << ")\n";
            for (uint32_t i = 0; i < c->arg_count; ++i) {
                dumpExpr(c->args[i], out, ind + 1);
            }
            break;
        }
        case Expr::Kind::If: {
            auto* ifE = static_cast<const IfExpr*>(expr);
            out << "If\n";
            indent(out, ind + 1); out << "cond:\n";
            dumpExpr(ifE->condition, out, ind + 2);
            indent(out, ind + 1); out << "then:\n";
            dumpExpr(ifE->then_branch, out, ind + 2);
            if (ifE->else_branch) {
                indent(out, ind + 1); out << "else:\n";
                dumpExpr(ifE->else_branch, out, ind + 2);
            }
            break;
        }
        case Expr::Kind::Block: {
            auto* bl = static_cast<const BlockExpr*>(expr);
            out << "Block\n";
            for (uint32_t i = 0; i < bl->stmt_count; ++i) {
                auto* st = bl->stmts[i];
                indent(out, ind + 1);
                switch (st->kind) {
                    case Stmt::Kind::ValDecl: {
                        auto* vd = static_cast<const ValDeclStmt*>(st);
                        out << "ValDecl(" << vd->name << ": " << vd->type.name << ")\n";
                        dumpExpr(vd->init, out, ind + 2);
                        break;
                    }
                    case Stmt::Kind::VarDecl: {
                        auto* vd = static_cast<const VarDeclStmt*>(st);
                        out << "VarDecl(" << vd->name << ": " << vd->type.name << ")\n";
                        dumpExpr(vd->init, out, ind + 2);
                        break;
                    }
                    case Stmt::Kind::ExprStmt: {
                        out << "ExprStmt\n";
                        dumpExpr(static_cast<const ExprStmt*>(st)->expr, out, ind + 2);
                        break;
                    }
                    case Stmt::Kind::Assign: {
                        auto* as = static_cast<const AssignStmt*>(st);
                        out << "Assign(" << as->name << ")\n";
                        dumpExpr(as->value, out, ind + 2);
                        break;
                    }
                    case Stmt::Kind::FieldAssign: {
                        auto* fas = static_cast<const FieldAssignStmt*>(st);
                        out << "FieldAssign\n";
                        indent(out, ind + 2); out << "target:\n";
                        dumpExpr(fas->target, out, ind + 3);
                        indent(out, ind + 2); out << "value:\n";
                        dumpExpr(fas->value, out, ind + 3);
                        break;
                    }
                    case Stmt::Kind::DerefAssign: {
                        auto* da = static_cast<const DerefAssignStmt*>(st);
                        out << "DerefAssign\n";
                        indent(out, ind + 2); out << "target:\n";
                        dumpExpr(da->target, out, ind + 3);
                        indent(out, ind + 2); out << "value:\n";
                        dumpExpr(da->value, out, ind + 3);
                        break;
                    }
                    case Stmt::Kind::Break: {
                        auto* bs = static_cast<const BreakStmt*>(st);
                        out << "Break\n";
                        if (bs->value) dumpExpr(bs->value, out, ind + 2);
                        break;
                    }
                    case Stmt::Kind::Continue: {
                        auto* cs = static_cast<const ContinueStmt*>(st);
                        out << "Continue(" << cs->arg_count << " args)\n";
                        for (uint32_t ci = 0; ci < cs->arg_count; ++ci) {
                            dumpExpr(cs->args[ci], out, ind + 2);
                        }
                        break;
                    }
                    case Stmt::Kind::IndexAssign: {
                        auto* ias = static_cast<const IndexAssignStmt*>(st);
                        out << "IndexAssign\n";
                        indent(out, ind + 2); out << "array:\n";
                        dumpExpr(ias->array, out, ind + 3);
                        indent(out, ind + 2); out << "index:\n";
                        dumpExpr(ias->index, out, ind + 3);
                        indent(out, ind + 2); out << "value:\n";
                        dumpExpr(ias->value, out, ind + 3);
                        break;
                    }
                    case Stmt::Kind::TupleDestruct: {
                        auto* td = static_cast<const TupleDestructStmt*>(st);
                        out << (td->is_var ? "VarTupleDestruct(" : "ValTupleDestruct(");
                        for (uint32_t ti = 0; ti < td->name_count; ++ti) {
                            if (ti > 0) out << ", ";
                            out << td->names[ti];
                        }
                        out << ")\n";
                        dumpExpr(td->init, out, ind + 2);
                        break;
                    }
                    case Stmt::Kind::Defer: {
                        auto* ds = static_cast<const DeferStmt*>(st);
                        out << "Defer\n";
                        dumpExpr(ds->body, out, ind + 2);
                        break;
                    }
                }
            }
            if (bl->result) {
                indent(out, ind + 1); out << "result:\n";
                dumpExpr(bl->result, out, ind + 2);
            }
            break;
        }
        case Expr::Kind::Return: {
            out << "Return\n";
            auto* r = static_cast<const ReturnExpr*>(expr);
            if (r->value) dumpExpr(r->value, out, ind + 1);
            break;
        }
        case Expr::Kind::Match: {
            auto* m = static_cast<const MatchExpr*>(expr);
            out << "Match\n";
            indent(out, ind + 1); out << "scrutinee:\n";
            dumpExpr(m->scrutinee, out, ind + 2);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                const auto& arm = m->arms[i];
                indent(out, ind + 1);
                out << "arm: ";
                switch (arm.pattern->kind) {
                    case Pattern::Kind::IntLit:
                        out << static_cast<const IntLitPattern*>(arm.pattern)->value;
                        break;
                    case Pattern::Kind::BoolLit:
                        out << (static_cast<const BoolLitPattern*>(arm.pattern)->value ? "true" : "false");
                        break;
                    case Pattern::Kind::Wildcard:
                        out << "_";
                        break;
                    case Pattern::Kind::Variable:
                        out << static_cast<const VariablePattern*>(arm.pattern)->name;
                        break;
                    case Pattern::Kind::Enum:
                        out << static_cast<const EnumPattern*>(arm.pattern)->variant_name;
                        break;
                    case Pattern::Kind::Range: {
                        auto* rp = static_cast<const RangePattern*>(arm.pattern);
                        out << rp->lo << (rp->inclusive ? "..=" : "..") << rp->hi;
                        break;
                    }
                    case Pattern::Kind::Union: {
                        auto* up = static_cast<const UnionPattern*>(arm.pattern);
                        out << up->variant_name;
                        if (up->inner) {
                            out << "(";
                            if (up->inner->kind == Pattern::Kind::Variable)
                                out << static_cast<const VariablePattern*>(up->inner)->name;
                            else
                                out << "_";
                            out << ")";
                        }
                    }
                        break;
                }
                if (arm.guard) out << " if ...";
                out << "\n";
                dumpExpr(arm.body, out, ind + 2);
            }
            break;
        }
        case Expr::Kind::StructLit: {
            auto* sl = static_cast<const StructLitExpr*>(expr);
            out << "StructLit(" << sl->struct_name << ")\n";
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                indent(out, ind + 1);
                out << sl->fields[i].name << ":\n";
                dumpExpr(sl->fields[i].value, out, ind + 2);
            }
            break;
        }
        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<const FieldAccessExpr*>(expr);
            out << "FieldAccess(." << fa->field_name << ")\n";
            dumpExpr(fa->object, out, ind + 1);
            break;
        }
        case Expr::Kind::EnumAccess: {
            auto* ea = static_cast<const EnumAccessExpr*>(expr);
            out << "EnumAccess(" << ea->enum_name << "." << ea->variant_name << ")\n";
            break;
        }
        case Expr::Kind::UnionVariant: {
            auto* uv = static_cast<const UnionVariantExpr*>(expr);
            out << "UnionVariant(" << uv->union_name << "::" << uv->variant_name << ")\n";
            if (uv->payload) dumpExpr(uv->payload, out, ind + 1);
            break;
        }
        case Expr::Kind::Cast: {
            auto* c = static_cast<const CastExpr*>(expr);
            out << "CastExpr(" << c->target.name << ")\n";
            dumpExpr(c->operand, out, ind + 1);
            break;
        }
        case Expr::Kind::Loop: {
            auto* lp = static_cast<const LoopExpr*>(expr);
            out << "LoopExpr\n";
            for (uint32_t i = 0; i < lp->binding_count; ++i) {
                indent(out, ind + 1);
                out << "binding " << lp->bindings[i].name << ":\n";
                dumpExpr(lp->bindings[i].init, out, ind + 2);
            }
            for (uint32_t i = 0; i < lp->stmt_count; ++i) {
                indent(out, ind + 1);
                out << "stmt:\n";
            }
            if (lp->result) {
                indent(out, ind + 1); out << "result:\n";
                dumpExpr(lp->result, out, ind + 2);
            }
            break;
        }
        case Expr::Kind::ForRange: {
            auto* fr = static_cast<const ForRangeExpr*>(expr);
            out << "ForRange(" << fr->var_name << ")\n";
            indent(out, ind + 1); out << "start:\n";
            dumpExpr(fr->start, out, ind + 2);
            indent(out, ind + 1); out << "end:\n";
            dumpExpr(fr->end, out, ind + 2);
            break;
        }
        case Expr::Kind::ForEach: {
            auto* fe = static_cast<const ForEachExpr*>(expr);
            out << "ForEach(" << fe->var_name << ")\n";
            indent(out, ind + 1); out << "collection:\n";
            dumpExpr(fe->collection, out, ind + 2);
            break;
        }
        case Expr::Kind::WhileLoop: {
            out << "WhileLoop\n";
            break;
        }
        case Expr::Kind::InlineAsm: {
            auto* ia = static_cast<const InlineAsmExpr*>(expr);
            out << "InlineAsm(" << ia->line_count << " lines)\n";
            break;
        }
        case Expr::Kind::ArrayLit: {
            auto* al = static_cast<const ArrayLitExpr*>(expr);
            out << "ArrayLit(" << al->count << " elements)\n";
            for (uint32_t i = 0; i < al->count; ++i) {
                dumpExpr(al->elements[i], out, ind + 1);
            }
            break;
        }
        case Expr::Kind::IndexAccess: {
            auto* ia = static_cast<const IndexAccessExpr*>(expr);
            out << "IndexAccess\n";
            indent(out, ind + 1); out << "array:\n";
            dumpExpr(ia->array, out, ind + 2);
            indent(out, ind + 1); out << "index:\n";
            dumpExpr(ia->index, out, ind + 2);
            break;
        }
        case Expr::Kind::SliceExpr: {
            auto* se = static_cast<const SliceExprNode*>(expr);
            out << "SliceExpr\n";
            indent(out, ind + 1); out << "array:\n";
            dumpExpr(se->array, out, ind + 2);
            if (se->start) {
                indent(out, ind + 1); out << "start:\n";
                dumpExpr(se->start, out, ind + 2);
            }
            if (se->end) {
                indent(out, ind + 1); out << "end:\n";
                dumpExpr(se->end, out, ind + 2);
            }
            break;
        }
        case Expr::Kind::Sizeof: {
            auto* sz = static_cast<const SizeofExpr*>(expr);
            out << "Sizeof(" << sz->target.name << ")\n";
            break;
        }
        case Expr::Kind::Alignof: {
            auto* al = static_cast<const AlignofExpr*>(expr);
            out << "Alignof(" << al->target.name << ")\n";
            break;
        }
        case Expr::Kind::Offsetof: {
            auto* oe = static_cast<const OffsetofExpr*>(expr);
            out << "Offsetof(" << oe->target.name << ", " << oe->field_name << ")\n";
            break;
        }
        case Expr::Kind::Lambda: {
            auto* lam = static_cast<const LambdaExpr*>(expr);
            out << "Lambda(";
            for (uint32_t i = 0; i < lam->param_count; ++i) {
                if (i > 0) out << ", ";
                out << lam->params[i].name;
                if (!lam->params[i].type.name.empty())
                    out << ": " << lam->params[i].type.name;
            }
            out << ")\n";
            dumpExpr(lam->body, out, ind + 1);
            break;
        }
        case Expr::Kind::MethodCall: {
            auto* mc = static_cast<const MethodCallExpr*>(expr);
            out << "MethodCall(." << mc->method_name << ")\n";
            dumpExpr(mc->object, out, ind + 1);
            for (uint32_t i = 0; i < mc->arg_count; ++i) {
                dumpExpr(mc->args[i], out, ind + 1);
            }
            break;
        }
        case Expr::Kind::Try: {
            auto* te = static_cast<const TryExpr*>(expr);
            out << "Try(?)\n";
            dumpExpr(te->operand, out, ind + 1);
            break;
        }
        case Expr::Kind::TupleLit: {
            auto* tl = static_cast<const TupleLitExpr*>(expr);
            out << "TupleLit(" << tl->count << ")\n";
            for (uint32_t i = 0; i < tl->count; ++i)
                dumpExpr(tl->elements[i], out, ind + 1);
            break;
        }
        case Expr::Kind::Uninit:
            out << "Uninit\n";
            break;
    }
}

void dumpAST(const Module* mod, std::ostream& out, int /*ind*/) {
    for (uint32_t i = 0; i < mod->struct_count; ++i) {
        auto* sd = mod->structs[i];
        out << "StructDecl(" << sd->name << ")\n";
        for (uint32_t j = 0; j < sd->field_count; ++j) {
            out << "  " << (sd->fields[j].is_mutable ? "var " : "")
                << sd->fields[j].name << ": " << sd->fields[j].type.name << "\n";
        }
    }
    for (uint32_t i = 0; i < mod->enum_count; ++i) {
        auto* ed = mod->enums[i];
        out << "EnumDecl(" << ed->name << ")\n";
        for (uint32_t j = 0; j < ed->variant_count; ++j) {
            out << "  " << ed->variants[j].name;
            if (ed->variants[j].has_value) {
                out << " = " << ed->variants[j].value;
            }
            out << "\n";
        }
    }
    for (uint32_t i = 0; i < mod->union_count; ++i) {
        auto* ud = mod->unions[i];
        out << "UnionDecl(" << ud->name << ")\n";
        for (uint32_t j = 0; j < ud->variant_count; ++j) {
            out << "  " << ud->variants[j].name;
            if (ud->variants[j].payload_type) {
                out << "(" << ud->variants[j].payload_type->name << ")";
            }
            out << "\n";
        }
    }
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        auto* fn = mod->functions[i];
        out << "FnDecl(" << fn->name << "(";
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            if (j > 0) out << ", ";
            out << fn->params[j].name << ": " << fn->params[j].type.name;
        }
        out << ")";
        if (!fn->return_type.name.empty())
            out << " -> " << fn->return_type.name;
        if (fn->is_intrinsic) out << " [intrinsic]";
        out << ")\n";
        if (fn->body) {
            dumpExpr(fn->body, out, 1);
        }
    }
}

// --- Parser ---

Parser::Parser(Lexer& lexer, Arena& arena, DiagnosticEngine& diag)
    : lexer_(lexer), arena_(arena), diag_(diag) {}

void Parser::setCfg(std::string_view key, std::string_view value) {
    cfg_flags_[key] = value;
}

void Parser::setCfg(std::string_view key) {
    cfg_flags_[key] = "";
}

bool Parser::evaluateCfg() {
    // Called after consuming "cfg" ident and expects '(' condition ')'
    expect(TokenKind::LParen, "expected '(' after @cfg");

    bool negated = false;
    if (check(TokenKind::KwNot) || (check(TokenKind::Ident) && peek().text == "not")) {
        advance();
        negated = true;
        expect(TokenKind::LParen, "expected '(' after 'not'");
    }

    Token key = expect(TokenKind::Ident, "expected cfg key");
    bool result = false;

    if (check(TokenKind::Eq)) {
        // @cfg(key = "value")
        advance(); // consume '='
        Token val = expect(TokenKind::StringLit, "expected string value in @cfg");
        auto val_text = val.text;
        if (val_text.size() >= 2 && val_text.front() == '"' && val_text.back() == '"') {
            val_text = val_text.substr(1, val_text.size() - 2);
        }
        auto it = cfg_flags_.find(key.text);
        result = (it != cfg_flags_.end() && it->second == val_text);
    } else {
        // @cfg(key) — boolean presence check
        result = cfg_flags_.count(key.text) > 0;
    }

    if (negated) {
        result = !result;
        expect(TokenKind::RParen, "expected ')' after not(...)");
    }

    expect(TokenKind::RParen, "expected ')' after @cfg(...)");
    return result;
}

void Parser::skipTopLevelItem() {
    // Skip tokens until the next top-level boundary
    // Handle balanced braces
    if (check(TokenKind::KwFn)) {
        // fn ... { ... } or fn ... = intrinsic
        while (!check(TokenKind::Eof)) {
            if (check(TokenKind::LBrace)) {
                advance();
                int depth = 1;
                while (depth > 0 && !check(TokenKind::Eof)) {
                    if (check(TokenKind::LBrace)) depth++;
                    else if (check(TokenKind::RBrace)) depth--;
                    advance();
                }
                return;
            }
            if (check(TokenKind::Eq)) {
                advance(); // consume '='
                if (check(TokenKind::Ident) && peek().text == "intrinsic") {
                    advance();
                }
                skipNewlines();
                return;
            }
            if (check(TokenKind::Newline)) {
                // Check if next non-newline token starts a new item
                skipNewlines();
                if (check(TokenKind::KwFn) || check(TokenKind::KwStruct) ||
                    check(TokenKind::KwEnum) || check(TokenKind::KwUnion) ||
                    check(TokenKind::KwTrait) || check(TokenKind::KwImpl) ||
                    check(TokenKind::KwPub) || check(TokenKind::At) ||
                    check(TokenKind::KwStatic) || check(TokenKind::KwExtern) ||
                    check(TokenKind::Eof)) {
                    return;
                }
                continue;
            }
            advance();
        }
    } else if (check(TokenKind::KwStruct) || check(TokenKind::KwEnum) ||
               check(TokenKind::KwUnion) || check(TokenKind::KwTrait) ||
               check(TokenKind::KwImpl)) {
        // Skip past the opening brace and its matching close
        while (!check(TokenKind::Eof)) {
            if (check(TokenKind::LBrace)) {
                advance();
                int depth = 1;
                while (depth > 0 && !check(TokenKind::Eof)) {
                    if (check(TokenKind::LBrace)) depth++;
                    else if (check(TokenKind::RBrace)) depth--;
                    advance();
                }
                skipNewlines();
                return;
            }
            advance();
        }
    } else if (check(TokenKind::KwStatic)) {
        // static val/var ... = expr
        while (!check(TokenKind::Eof) && !check(TokenKind::Newline)) {
            advance();
        }
        skipNewlines();
        return;
    } else {
        // Unknown — skip to next newline
        while (!check(TokenKind::Eof) && !check(TokenKind::Newline)) {
            advance();
        }
        skipNewlines();
    }
}

Token Parser::peek() {
    if (!has_current_) {
        current_ = lexer_.nextToken();
        has_current_ = true;
    }
    return current_;
}

Token Parser::advance() {
    if (!has_current_) {
        current_ = lexer_.nextToken();
    }
    previous_ = current_;
    has_current_ = false;
    return previous_;
}

Token Parser::expect(TokenKind kind, const char* message) {
    Token tok = peek();
    if (tok.kind != kind) {
        diag_.error(tok.loc, message);
        advance(); // skip past the unexpected token to avoid cascade errors
        return tok;
    }
    return advance();
}

bool Parser::check(TokenKind kind) {
    return peek().kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

void Parser::skipNewlines() {
    while (check(TokenKind::Newline)) {
        advance();
    }
}

// --- Module ---
Module* Parser::parseModule() {
    std::vector<FnDecl*> fns;
    std::vector<StructDecl*> structs;
    std::vector<EnumDecl*> enums;
    std::vector<UnionDecl*> unions;
    std::vector<TypeAliasDecl*> type_aliases;
    std::vector<NewtypeDecl*> newtypes;
    std::vector<StaticAssertDecl*> static_asserts;
    std::vector<TraitDecl*> traits;
    std::vector<ImplDecl*> impls;
    std::vector<ImportDecl*> imports;
    std::vector<GlobalDecl*> globals;
    std::string_view module_name;

    // Register builtin struct-like types so struct literal syntax works
    struct_names_.insert("Slice");
    skipNewlines();

    // Parse optional module declaration: module kern.memory
    if (check(TokenKind::KwModule)) {
        advance();
        std::string name_buf;
        Token first = expect(TokenKind::Ident, "expected module name");
        name_buf = std::string(first.text);
        while (check(TokenKind::Dot)) {
            advance();
            Token part = expect(TokenKind::Ident, "expected module name part after '.'");
            name_buf += ".";
            name_buf += std::string(part.text);
        }
        char* buf = arena_.makeArray<char>(name_buf.size());
        std::memcpy(buf, name_buf.data(), name_buf.size());
        module_name = std::string_view(buf, name_buf.size());
        skipNewlines();
    }

    // Parse import declarations: [pub] import kern.types (PhysAddr, VirtAddr)
    while (check(TokenKind::KwImport)) {
        auto loc = peek().loc;
        advance();
        std::string path_buf;
        Token first = expect(TokenKind::Ident, "expected module path");
        path_buf = std::string(first.text);
        while (check(TokenKind::Dot)) {
            advance();
            Token part = expect(TokenKind::Ident, "expected module path part after '.'");
            path_buf += ".";
            path_buf += std::string(part.text);
        }
        char* pbuf = arena_.makeArray<char>(path_buf.size());
        std::memcpy(pbuf, path_buf.data(), path_buf.size());

        auto* imp = arena_.make<ImportDecl>();
        imp->module_path = std::string_view(pbuf, path_buf.size());
        imp->loc = loc;
        imp->is_pub = false;
        imp->names = nullptr;
        imp->name_count = 0;

        // Optional: (Name1, Name2, ...)
        if (check(TokenKind::LParen)) {
            advance();
            std::vector<std::string_view> names;
            while (!check(TokenKind::RParen) && !check(TokenKind::Eof)) {
                Token n = expect(TokenKind::Ident, "expected import name");
                names.push_back(n.text);
                if (check(TokenKind::Comma)) advance();
            }
            expect(TokenKind::RParen, "expected ')' after import names");
            if (!names.empty()) {
                imp->names = arena_.makeArray<std::string_view>(names.size());
                for (uint32_t j = 0; j < names.size(); ++j) {
                    imp->names[j] = names[j];
                }
                imp->name_count = static_cast<uint32_t>(names.size());
            }
        }
        imports.push_back(imp);
        skipNewlines();
    }

    while (!check(TokenKind::Eof)) {
        // Parse visibility modifier: pub
        bool is_pub = false;
        if (check(TokenKind::KwPub)) {
            is_pub = true;
            advance();
            skipNewlines();
        }

        // Parse extern modifier: extern "C"
        bool is_extern = false;
        std::string_view extern_abi;
        if (check(TokenKind::KwExtern)) {
            is_extern = true;
            advance();
            if (check(TokenKind::StringLit)) {
                auto abi_tok = advance();
                extern_abi = abi_tok.text;
                // Strip quotes: "C" → C
                if (extern_abi.size() >= 2 && extern_abi.front() == '"' && extern_abi.back() == '"') {
                    extern_abi = extern_abi.substr(1, extern_abi.size() - 2);
                }
            }
            skipNewlines();
        }

        // pub import — re-export
        if (is_pub && check(TokenKind::KwImport)) {
            auto loc = peek().loc;
            advance();
            std::string path_buf;
            Token first = expect(TokenKind::Ident, "expected module path");
            path_buf = std::string(first.text);
            while (check(TokenKind::Dot)) {
                advance();
                Token part = expect(TokenKind::Ident, "expected module path part after '.'");
                path_buf += ".";
                path_buf += std::string(part.text);
            }
            char* pbuf = arena_.makeArray<char>(path_buf.size());
            std::memcpy(pbuf, path_buf.data(), path_buf.size());
            auto* imp = arena_.make<ImportDecl>();
            imp->module_path = std::string_view(pbuf, path_buf.size());
            imp->loc = loc;
            imp->is_pub = true;
            imp->names = nullptr;
            imp->name_count = 0;
            if (check(TokenKind::LParen)) {
                advance();
                std::vector<std::string_view> names;
                while (!check(TokenKind::RParen) && !check(TokenKind::Eof)) {
                    Token n = expect(TokenKind::Ident, "expected import name");
                    names.push_back(n.text);
                    if (check(TokenKind::Comma)) advance();
                }
                expect(TokenKind::RParen, "expected ')' after import names");
                if (!names.empty()) {
                    imp->names = arena_.makeArray<std::string_view>(names.size());
                    for (uint32_t j = 0; j < names.size(); ++j) imp->names[j] = names[j];
                    imp->name_count = static_cast<uint32_t>(names.size());
                }
            }
            imports.push_back(imp);
            skipNewlines();
            continue;
        }

        // Parse annotations (@packed, @align(N), @section("name"), @cfg(...))
        bool is_packed = false;
        bool is_naked = false;
        bool is_interrupt = false;
        bool is_inline = false;
        bool is_noinline = false;
        bool is_noreturn = false;
        bool is_const_fn = false;
        bool is_repr_c = false;
        uint8_t repr_backing_size = 0;  // @repr(u8)=1, @repr(u16)=2, etc.
        bool is_weak = false;
        bool is_no_mangle = false;
        bool cfg_excluded = false;
        uint32_t explicit_align = 0;
        std::string_view section_name;
        std::string_view link_name;
        while (check(TokenKind::At)) {
            advance(); // consume '@'
            // Accept Ident or keywords that double as annotation names
            Token anno;
            if (check(TokenKind::KwConst)) {
                anno = advance();
            } else if (check(TokenKind::KwNoreturn)) {
                anno = advance();
            } else {
                anno = expect(TokenKind::Ident, "expected annotation name after '@'");
            }
            if (anno.text == "packed") {
                is_packed = true;
            } else if (anno.text == "naked") {
                is_naked = true;
            } else if (anno.text == "interrupt") {
                is_interrupt = true;
            } else if (anno.text == "inline") {
                is_inline = true;
            } else if (anno.text == "noinline") {
                is_noinline = true;
            } else if (anno.text == "noreturn") {
                is_noreturn = true;
            } else if (anno.text == "const") {
                is_const_fn = true;
            } else if (anno.text == "cfg") {
                if (!evaluateCfg()) {
                    cfg_excluded = true;
                }
            } else if (anno.text == "align") {
                expect(TokenKind::LParen, "expected '(' after @align");
                Token val = expect(TokenKind::IntLit, "expected integer in @align(N)");
                explicit_align = static_cast<uint32_t>(std::stoull(std::string(val.text)));
                expect(TokenKind::RParen, "expected ')' after @align(N)");
            } else if (anno.text == "section") {
                expect(TokenKind::LParen, "expected '(' after @section");
                Token val = expect(TokenKind::StringLit, "expected string in @section(\"name\")");
                // Strip surrounding quotes from StringLit token text
                section_name = val.text;
                if (section_name.size() >= 2 && section_name.front() == '"' && section_name.back() == '"') {
                    section_name = section_name.substr(1, section_name.size() - 2);
                }
                expect(TokenKind::RParen, "expected ')' after @section(\"name\")");
            } else if (anno.text == "repr") {
                expect(TokenKind::LParen, "expected '(' after @repr");
                Token val = expect(TokenKind::Ident, "expected repr kind in @repr(C)");
                if (val.text == "C") {
                    is_repr_c = true;
                } else if (val.text == "u8" || val.text == "i8") {
                    repr_backing_size = 1;
                } else if (val.text == "u16" || val.text == "i16") {
                    repr_backing_size = 2;
                } else if (val.text == "u32" || val.text == "i32") {
                    repr_backing_size = 4;
                } else if (val.text == "u64" || val.text == "i64") {
                    repr_backing_size = 8;
                } else {
                    diag_.error(val.loc, std::string("unknown repr kind '") +
                                std::string(val.text) + "'; expected 'C', 'u8', 'u16', 'u32', or 'u64'");
                }
                expect(TokenKind::RParen, "expected ')' after @repr(C)");
            } else if (anno.text == "weak") {
                is_weak = true;
            } else if (anno.text == "no_mangle") {
                is_no_mangle = true;
            } else if (anno.text == "link_name") {
                expect(TokenKind::LParen, "expected '(' after @link_name");
                Token val = expect(TokenKind::StringLit, "expected string in @link_name(\"name\")");
                link_name = val.text;
                if (link_name.size() >= 2 && link_name.front() == '"' && link_name.back() == '"') {
                    link_name = link_name.substr(1, link_name.size() - 2);
                }
                expect(TokenKind::RParen, "expected ')' after @link_name(\"name\")");
            } else {
                diag_.error(anno.loc, std::string("unknown annotation '@") +
                            std::string(anno.text) + "'");
            }
            skipNewlines();
        }

        // If @cfg excluded this item, skip it entirely
        if (cfg_excluded) {
            skipTopLevelItem();
            skipNewlines();
            continue;
        }

        // Parse extern modifier after annotations (e.g., @link_name("x") extern static val)
        if (!is_extern && check(TokenKind::KwExtern)) {
            is_extern = true;
            advance();
            if (check(TokenKind::StringLit)) {
                auto abi_tok = advance();
                extern_abi = abi_tok.text;
                if (extern_abi.size() >= 2 && extern_abi.front() == '"' && extern_abi.back() == '"') {
                    extern_abi = extern_abi.substr(1, extern_abi.size() - 2);
                }
            }
            skipNewlines();
        }

        if (check(TokenKind::KwStruct)) {
            StructDecl* sd = parseStructDecl();
            if (sd) {
                sd->is_packed = is_packed;
                sd->is_repr_c = is_repr_c;
                sd->is_pub = is_pub;
                sd->explicit_align = explicit_align;
                structs.push_back(sd);
                struct_names_.insert(sd->name);
            }
        } else if (check(TokenKind::KwEnum)) {
            EnumDecl* ed = parseEnumDecl();
            if (ed) {
                ed->is_pub = is_pub;
                if (repr_backing_size > 0) {
                    ed->backing_size = repr_backing_size;
                }
                enums.push_back(ed);
                enum_names_.insert(ed->name);
            }
        } else if (check(TokenKind::KwUnion)) {
            UnionDecl* ud = parseUnionDecl();
            if (ud) {
                ud->is_pub = is_pub;
                ud->is_repr_c = is_repr_c;
                unions.push_back(ud);
                union_names_.insert(ud->name);
            }
        } else if (check(TokenKind::KwFn)) {
            FnDecl* fn = parseFnDecl();
            if (fn) {
                fn->is_naked = is_naked;
                fn->is_interrupt = is_interrupt;
                fn->is_inline = is_inline;
                fn->is_noinline = is_noinline;
                fn->is_noreturn = is_noreturn;
                fn->is_const = is_const_fn;
                fn->is_pub = is_pub;
                fn->is_extern = is_extern;
                fn->is_weak = is_weak;
                fn->is_no_mangle = is_no_mangle;
                fn->extern_abi = extern_abi;
                fn->section_name = section_name;
                fn->link_name = link_name;
                if (is_no_mangle && fn->link_name.empty()) {
                    fn->link_name = fn->name;
                }
                if (is_extern && fn->body) {
                    // extern functions should not have a body
                    fn->body = nullptr;
                }
                if (!fn->body && !is_extern && !fn->is_intrinsic) {
                    diag_.error(fn->loc, "function '" + std::string(fn->name) +
                                "' has no body (use 'extern fn' for forward declarations)");
                }
                fns.push_back(fn);
            }
        } else if (check(TokenKind::KwType)) {
            auto* ta = parseTypeAlias();
            if (ta) {
                ta->is_pub = is_pub;
                type_aliases.push_back(ta);
            }
        } else if (check(TokenKind::KwNewtype)) {
            auto* nt = parseNewtype();
            if (nt) {
                nt->is_pub = is_pub;
                newtypes.push_back(nt);
                struct_names_.insert(nt->name);
            }
        } else if (check(TokenKind::KwTrait)) {
            auto* td = parseTraitDecl();
            if (td) {
                td->is_pub = is_pub;
                traits.push_back(td);
            }
        } else if (check(TokenKind::KwImpl)) {
            auto* id = parseImplDecl();
            if (id) impls.push_back(id);
        } else if (check(TokenKind::KwStatic)) {
            auto* gd = parseGlobalDecl();
            if (gd) {
                gd->is_pub = is_pub;
                gd->is_extern = is_extern;
                gd->section_name = section_name;
                gd->link_name = link_name;
                if (is_no_mangle && gd->link_name.empty()) {
                    gd->link_name = gd->name;
                }
                globals.push_back(gd);
            }
        } else if (peek().kind == TokenKind::Ident && peek().text == "static_assert") {
            advance(); // consume 'static_assert'
            expect(TokenKind::LParen, "expected '(' after static_assert");
            Expr* cond = parseExpr();
            expect(TokenKind::Comma, "expected ',' after static_assert condition");
            Token msg_tok = expect(TokenKind::StringLit, "expected string literal message");
            expect(TokenKind::RParen, "expected ')' after static_assert");
            auto* sa = arena_.make<StaticAssertDecl>();
            sa->condition = cond;
            sa->message = msg_tok.text;
            sa->loc = cond->loc;
            static_asserts.push_back(sa);
        } else {
            diag_.error(peek().loc, "expected function, struct, enum, union, type, newtype, trait, or impl declaration");
            advance();
        }
        skipNewlines();
    }

    // Function-level pattern matching: group same-name FnDecls
    // fn fib(0) -> i64 { 0 }
    // fn fib(1) -> i64 { 1 }
    // fn fib(n: i64) -> i64 { ... }
    // → single fn fib(_arg0: i64) -> i64 { match _arg0 { 0 => 0, 1 => 1, n => ... } }
    std::vector<FnDecl*> merged;
    size_t i = 0;
    while (i < fns.size()) {
        // Check if this starts a group of pattern-matched overloads
        size_t group_end = i + 1;
        bool is_pattern_group = fns[i]->has_pattern_params;
        while (group_end < fns.size() && fns[group_end]->name == fns[i]->name) {
            group_end++;
        }

        if (is_pattern_group && group_end - i > 1) {
            // Desugar: merge into single fn with match expr
            auto* first = fns[i];
            std::string_view param_name = "_arg0";

            // Determine parameter type from the variable-pattern overload
            TypeRef param_type = {};
            for (size_t j = i; j < group_end; ++j) {
                if (!fns[j]->has_pattern_params && fns[j]->param_count > 0) {
                    param_type = fns[j]->params[0].type;
                    // Also grab the variable name for the last (catch-all) arm
                    break;
                }
            }

            // Build match arms
            std::vector<MatchArm> arms;
            for (size_t j = i; j < group_end; ++j) {
                MatchArm arm;
                arm.loc = fns[j]->loc;
                arm.guard = nullptr;

                if (fns[j]->has_pattern_params) {
                    // This overload has a literal pattern param
                    arm.pattern = fns[j]->pattern_param;
                } else {
                    // Variable param → VariablePattern
                    if (fns[j]->param_count > 0) {
                        auto* vp = arena_.make<VariablePattern>();
                        vp->kind = Pattern::Kind::Variable;
                        vp->loc = fns[j]->params[0].loc;
                        vp->name = fns[j]->params[0].name;
                        arm.pattern = vp;
                    } else {
                        auto* wp = arena_.make<WildcardPattern>();
                        wp->kind = Pattern::Kind::Wildcard;
                        wp->loc = fns[j]->loc;
                        arm.pattern = wp;
                    }
                }
                arm.body = fns[j]->body;
                arms.push_back(arm);
            }

            // Build MatchExpr
            auto* scrutinee_ident = arena_.make<IdentExpr>();
            scrutinee_ident->kind = Expr::Kind::Ident;
            scrutinee_ident->loc = first->loc;
            scrutinee_ident->name = param_name;

            auto* match_expr = arena_.make<MatchExpr>();
            match_expr->kind = Expr::Kind::Match;
            match_expr->loc = first->loc;
            match_expr->scrutinee = scrutinee_ident;
            match_expr->arm_count = static_cast<uint32_t>(arms.size());
            match_expr->arms = arena_.makeArray<MatchArm>(arms.size());
            for (size_t j = 0; j < arms.size(); ++j) {
                match_expr->arms[j] = arms[j];
            }

            // Wrap in block
            auto* block = arena_.make<BlockExpr>();
            block->kind = Expr::Kind::Block;
            block->loc = first->loc;
            block->stmt_count = 0;
            block->stmts = nullptr;
            block->result = match_expr;

            // Build merged FnDecl
            auto* merged_fn = arena_.make<FnDecl>();
            merged_fn->name = first->name;
            merged_fn->param_count = 1;
            merged_fn->params = arena_.makeArray<Param>(1);
            merged_fn->params[0] = {param_name, param_type, first->loc};
            merged_fn->return_type = first->return_type;
            merged_fn->body = block;
            merged_fn->loc = first->loc;
            merged_fn->is_intrinsic = false;
            merged.push_back(merged_fn);
            i = group_end;
        } else {
            merged.push_back(fns[i]);
            i++;
        }
    }

    auto* mod = arena_.make<Module>();
    mod->fn_count = static_cast<uint32_t>(merged.size());
    mod->functions = arena_.makeArray<FnDecl*>(merged.size());
    for (size_t j = 0; j < merged.size(); ++j) {
        mod->functions[j] = merged[j];
    }
    mod->struct_count = static_cast<uint32_t>(structs.size());
    mod->structs = arena_.makeArray<StructDecl*>(structs.size());
    for (size_t j = 0; j < structs.size(); ++j) {
        mod->structs[j] = structs[j];
    }
    mod->enum_count = static_cast<uint32_t>(enums.size());
    mod->enums = arena_.makeArray<EnumDecl*>(enums.size());
    for (size_t j = 0; j < enums.size(); ++j) {
        mod->enums[j] = enums[j];
    }
    mod->union_count = static_cast<uint32_t>(unions.size());
    mod->unions = arena_.makeArray<UnionDecl*>(unions.size());
    for (size_t j = 0; j < unions.size(); ++j) {
        mod->unions[j] = unions[j];
    }
    mod->type_alias_count = static_cast<uint32_t>(type_aliases.size());
    mod->type_aliases = arena_.makeArray<TypeAliasDecl*>(type_aliases.size());
    for (size_t j = 0; j < type_aliases.size(); ++j) {
        mod->type_aliases[j] = type_aliases[j];
    }
    mod->newtype_count = static_cast<uint32_t>(newtypes.size());
    mod->newtypes = arena_.makeArray<NewtypeDecl*>(newtypes.size());
    for (size_t j = 0; j < newtypes.size(); ++j) {
        mod->newtypes[j] = newtypes[j];
    }
    mod->static_assert_count = static_cast<uint32_t>(static_asserts.size());
    mod->static_asserts = arena_.makeArray<StaticAssertDecl*>(static_asserts.size());
    for (size_t j = 0; j < static_asserts.size(); ++j) {
        mod->static_asserts[j] = static_asserts[j];
    }
    mod->trait_count = static_cast<uint32_t>(traits.size());
    mod->traits = arena_.makeArray<TraitDecl*>(traits.size());
    for (size_t j = 0; j < traits.size(); ++j) {
        mod->traits[j] = traits[j];
    }
    mod->impl_count = static_cast<uint32_t>(impls.size());
    mod->impls = arena_.makeArray<ImplDecl*>(impls.size());
    for (size_t j = 0; j < impls.size(); ++j) {
        mod->impls[j] = impls[j];
    }
    mod->module_name = module_name;
    mod->import_count = static_cast<uint32_t>(imports.size());
    mod->imports = arena_.makeArray<ImportDecl*>(imports.size());
    for (size_t j = 0; j < imports.size(); ++j) {
        mod->imports[j] = imports[j];
    }
    mod->global_count = static_cast<uint32_t>(globals.size());
    mod->globals = arena_.makeArray<GlobalDecl*>(globals.size());
    for (size_t j = 0; j < globals.size(); ++j) {
        mod->globals[j] = globals[j];
    }
    return mod;
}

// --- Type Alias: type Name = TargetType ---
TypeAliasDecl* Parser::parseTypeAlias() {
    SourceLocation loc = peek().loc;
    advance(); // consume 'type'
    Token nameTok = expect(TokenKind::Ident, "expected type alias name");
    expect(TokenKind::Eq, "expected '=' in type alias");
    TypeRef target = parseType();
    auto* ta = arena_.make<TypeAliasDecl>();
    ta->name = nameTok.text;
    ta->target = target;
    ta->loc = loc;
    return ta;
}

// --- Newtype: newtype Name(InnerType) ---
NewtypeDecl* Parser::parseNewtype() {
    SourceLocation loc = peek().loc;
    advance(); // consume 'newtype'
    Token nameTok = expect(TokenKind::Ident, "expected newtype name");
    expect(TokenKind::LParen, "expected '(' after newtype name");
    TypeRef inner = parseType();
    expect(TokenKind::RParen, "expected ')' after newtype inner type");
    auto* nt = arena_.make<NewtypeDecl>();
    nt->name = nameTok.text;
    nt->inner = inner;
    nt->loc = loc;
    return nt;
}

// --- Trait Declaration ---
// trait Printable {
//     fn to_string(self: Self) -> String
// }
TraitDecl* Parser::parseTraitDecl() {
    SourceLocation loc = peek().loc;
    advance(); // consume 'trait'
    Token nameTok = expect(TokenKind::Ident, "expected trait name");
    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' after trait name");
    skipNewlines();

    std::vector<TraitMethodSig> methods;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        expect(TokenKind::KwFn, "expected 'fn' in trait body");
        Token methodName = expect(TokenKind::Ident, "expected method name");
        expect(TokenKind::LParen, "expected '(' after method name");

        std::vector<Param> params;
        if (!check(TokenKind::RParen)) {
            params.push_back(parseParam());
            while (match(TokenKind::Comma)) {
                params.push_back(parseParam());
            }
        }
        expect(TokenKind::RParen, "expected ')' after parameters");
        expect(TokenKind::Arrow, "expected '->' for return type");
        TypeRef ret = parseType();

        // Parse optional effect clause for trait methods
        std::vector<std::string_view> eff_names;
        if (check(TokenKind::KwWith)) {
            advance();
            if (check(TokenKind::Ident) && peek().text == "pure") {
                advance();
            } else {
                Token eff = expect(TokenKind::Ident, "expected effect name after 'with'");
                eff_names.push_back(eff.text);
                while (match(TokenKind::Comma)) {
                    Token eff2 = expect(TokenKind::Ident, "expected effect name after ','");
                    eff_names.push_back(eff2.text);
                }
            }
        }

        TraitMethodSig sig;
        sig.name = methodName.text;
        sig.param_count = static_cast<uint32_t>(params.size());
        sig.params = arena_.makeArray<Param>(params.size());
        for (size_t j = 0; j < params.size(); ++j) {
            sig.params[j] = params[j];
        }
        sig.return_type = ret;
        sig.loc = methodName.loc;
        sig.effect_count = static_cast<uint32_t>(eff_names.size());
        if (!eff_names.empty()) {
            sig.effect_names = arena_.makeArray<std::string_view>(eff_names.size());
            for (size_t j = 0; j < eff_names.size(); ++j) {
                sig.effect_names[j] = eff_names[j];
            }
        }
        methods.push_back(sig);
        skipNewlines();
    }
    expect(TokenKind::RBrace, "expected '}' to close trait");

    auto* td = arena_.make<TraitDecl>();
    td->name = nameTok.text;
    td->method_count = static_cast<uint32_t>(methods.size());
    td->methods = arena_.makeArray<TraitMethodSig>(methods.size());
    for (size_t j = 0; j < methods.size(); ++j) {
        td->methods[j] = methods[j];
    }
    td->loc = loc;
    return td;
}

// --- Impl Declaration ---
// impl Printable for Point {
//     fn to_string(self: Point) -> String { ... }
// }
ImplDecl* Parser::parseImplDecl() {
    SourceLocation loc = peek().loc;
    advance(); // consume 'impl'
    Token traitName = expect(TokenKind::Ident, "expected trait name after 'impl'");
    // Expect 'for' keyword
    expect(TokenKind::KwFor, "expected 'for' after trait name in impl declaration");
    TypeRef target = parseType();
    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' in impl block");
    skipNewlines();

    std::vector<FnDecl*> methods;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        if (check(TokenKind::KwFn)) {
            FnDecl* fn = parseFnDecl();
            if (fn) methods.push_back(fn);
        } else {
            diag_.error(peek().loc, "expected 'fn' in impl block");
            advance();
        }
        skipNewlines();
    }
    expect(TokenKind::RBrace, "expected '}' to close impl block");

    auto* id = arena_.make<ImplDecl>();
    id->trait_name = traitName.text;
    id->target_type = target;
    id->method_count = static_cast<uint32_t>(methods.size());
    id->methods = arena_.makeArray<FnDecl*>(methods.size());
    for (size_t j = 0; j < methods.size(); ++j) {
        id->methods[j] = methods[j];
    }
    id->loc = loc;
    return id;
}

// --- Global Variable Declaration: static val/var name: Type = expr ---
GlobalDecl* Parser::parseGlobalDecl() {
    SourceLocation loc = peek().loc;
    advance(); // consume 'static'

    bool is_mutable = false;
    if (check(TokenKind::KwVar)) {
        is_mutable = true;
        advance();
    } else if (check(TokenKind::KwVal)) {
        advance();
    } else {
        diag_.error(peek().loc, "expected 'val' or 'var' after 'static'");
        return nullptr;
    }

    Token name = expect(TokenKind::Ident, "expected name in static declaration");
    expect(TokenKind::Colon, "expected ':' after static variable name");
    TypeRef type = parseType();

    Expr* init = nullptr;
    if (match(TokenKind::Eq)) {
        init = parseExpr();
    }

    auto* gd = arena_.make<GlobalDecl>();
    gd->name = name.text;
    gd->type = type;
    gd->init = init;
    gd->is_mutable = is_mutable;
    gd->loc = loc;
    return gd;
}

// --- Struct Declaration ---
StructDecl* Parser::parseStructDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwStruct, "expected 'struct'");
    Token nameTok = expect(TokenKind::Ident, "expected struct name");

    // Parse optional type parameters: struct Vec<T> { ... } or struct Buffer<T, const N: u64> { ... }
    std::vector<TypeParam> type_params;
    if (check(TokenKind::Lt)) {
        advance();
        while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
            TypeParam p;
            if (check(TokenKind::KwConst)) {
                advance(); // consume 'const'
                Token tp = expect(TokenKind::Ident, "expected const parameter name");
                p.name = tp.text;
                p.loc = tp.loc;
                p.is_const = true;
                expect(TokenKind::Colon, "expected ':' after const parameter name");
                p.const_type = parseType();
            } else {
                Token tp = expect(TokenKind::Ident, "expected type parameter name");
                p.name = tp.text;
                p.loc = tp.loc;
            }
            type_params.push_back(p);
            if (!check(TokenKind::Gt)) {
                expect(TokenKind::Comma, "expected ',' or '>' in type parameter list");
            }
        }
        expect(TokenKind::Gt, "expected '>' after type parameters");
    }

    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' after struct name");
    skipNewlines();

    std::vector<FieldDecl> fields;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        FieldDecl fd;
        fd.is_mutable = false;
        fd.is_pub = true;  // default: public (backward compat)
        // Parse optional visibility: pub / priv
        if (check(TokenKind::KwPub)) {
            advance();
            fd.is_pub = true;
        } else if (check(TokenKind::KwPriv)) {
            advance();
            fd.is_pub = false;
        }
        if (check(TokenKind::KwVar)) {
            advance();
            fd.is_mutable = true;
        } else if (check(TokenKind::KwVal)) {
            advance();
        }
        Token field_name = expect(TokenKind::Ident, "expected field name");
        fd.name = field_name.text;
        fd.loc = field_name.loc;
        expect(TokenKind::Colon, "expected ':' after field name");
        fd.type = parseType();
        // Optional bitfield width: `field: u32 : 5`
        if (check(TokenKind::Colon)) {
            advance();
            Token width_tok = expect(TokenKind::IntLit, "expected bitfield width");
            fd.bit_width = static_cast<uint32_t>(std::stoul(std::string(width_tok.text)));
        }
        fields.push_back(fd);
        // Skip comma, semicolons, newlines between fields
        while (match(TokenKind::Comma) || match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after struct fields");

    auto* sd = arena_.make<StructDecl>();
    sd->name = nameTok.text;
    sd->loc = loc;
    sd->field_count = static_cast<uint32_t>(fields.size());
    sd->fields = arena_.makeArray<FieldDecl>(fields.size());
    for (size_t i = 0; i < fields.size(); ++i) {
        sd->fields[i] = fields[i];
    }
    sd->type_param_count = static_cast<uint32_t>(type_params.size());
    if (!type_params.empty()) {
        sd->type_params = arena_.makeArray<TypeParam>(type_params.size());
        for (size_t i = 0; i < type_params.size(); ++i) {
            sd->type_params[i] = type_params[i];
        }
    }
    return sd;
}

// --- Enum Declaration ---
EnumDecl* Parser::parseEnumDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwEnum, "expected 'enum'");
    Token nameTok = expect(TokenKind::Ident, "expected enum name");
    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' after enum name");
    skipNewlines();

    std::vector<EnumVariant> variants;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        Token vname = expect(TokenKind::Ident, "expected variant name");
        EnumVariant ev;
        ev.name = vname.text;
        ev.loc = vname.loc;
        // Optional explicit discriminant: = IntLit
        if (check(TokenKind::Eq)) {
            advance();
            Token val = expect(TokenKind::IntLit, "expected integer discriminant after '='");
            ev.value = parseIntText(val.text);
            ev.has_value = true;
        }
        variants.push_back(ev);
        while (match(TokenKind::Comma) || match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after enum variants");

    auto* ed = arena_.make<EnumDecl>();
    ed->name = nameTok.text;
    ed->loc = loc;
    ed->variant_count = static_cast<uint32_t>(variants.size());
    ed->variants = arena_.makeArray<EnumVariant>(variants.size());
    for (size_t i = 0; i < variants.size(); ++i) {
        ed->variants[i] = variants[i];
    }
    return ed;
}

// --- Union Declaration ---
UnionDecl* Parser::parseUnionDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwUnion, "expected 'union'");
    Token nameTok = expect(TokenKind::Ident, "expected union name");

    // Parse optional type parameters: <T, U, ...> or <T, const N: u64>
    std::vector<TypeParam> type_params;
    if (check(TokenKind::Lt)) {
        advance(); // consume '<'
        while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
            TypeParam p;
            if (check(TokenKind::KwConst)) {
                advance();
                Token tp = expect(TokenKind::Ident, "expected const parameter name");
                p.name = tp.text; p.loc = tp.loc;
                p.is_const = true;
                expect(TokenKind::Colon, "expected ':' after const parameter name");
                p.const_type = parseType();
            } else {
                Token tp = expect(TokenKind::Ident, "expected type parameter name");
                p.name = tp.text; p.loc = tp.loc;
            }
            type_params.push_back(p);
            if (!check(TokenKind::Gt)) {
                expect(TokenKind::Comma, "expected ',' or '>' in type parameter list");
            }
        }
        expect(TokenKind::Gt, "expected '>' after type parameters");
    }

    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' after union name");
    skipNewlines();

    std::vector<UnionVariantDecl> variants;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        Token vname = expect(TokenKind::Ident, "expected variant name");
        UnionVariantDecl vd;
        vd.name = vname.text;
        vd.loc = vname.loc;
        vd.payload_type = nullptr;
        if (match(TokenKind::LParen)) {
            auto* tr = arena_.make<TypeRef>();
            *tr = parseType();
            vd.payload_type = tr;
            expect(TokenKind::RParen, "expected ')' after variant type");
        }
        variants.push_back(vd);
        while (match(TokenKind::Comma) || match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after union variants");

    auto* ud = arena_.make<UnionDecl>();
    ud->name = nameTok.text;
    ud->loc = loc;
    ud->variant_count = static_cast<uint32_t>(variants.size());
    ud->variants = arena_.makeArray<UnionVariantDecl>(variants.size());
    for (size_t i = 0; i < variants.size(); ++i) {
        ud->variants[i] = variants[i];
    }
    ud->type_param_count = static_cast<uint32_t>(type_params.size());
    if (!type_params.empty()) {
        ud->type_params = arena_.makeArray<TypeParam>(type_params.size());
        for (size_t i = 0; i < type_params.size(); ++i)
            ud->type_params[i] = type_params[i];
    }
    return ud;
}

// --- Function Declaration ---
FnDecl* Parser::parseFnDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwFn, "expected 'fn'");

    Token nameTok = expect(TokenKind::Ident, "expected function name");

    // Parse optional type parameters: <T, U, ...> or <T, const N: u64>
    std::vector<TypeParam> type_params;
    if (check(TokenKind::Lt)) {
        advance(); // consume '<'
        while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
            TypeParam p;
            if (check(TokenKind::KwConst)) {
                advance();
                Token tp = expect(TokenKind::Ident, "expected const parameter name");
                p.name = tp.text;
                p.loc = tp.loc;
                p.is_const = true;
                expect(TokenKind::Colon, "expected ':' after const parameter name");
                p.const_type = parseType();
            } else {
                Token tp = expect(TokenKind::Ident, "expected type parameter name");
                p.name = tp.text;
                p.loc = tp.loc;
            }
            type_params.push_back(p);
            if (!check(TokenKind::Gt)) {
                expect(TokenKind::Comma, "expected ',' or '>' in type parameter list");
            }
        }
        expect(TokenKind::Gt, "expected '>' after type parameters");
    }

    expect(TokenKind::LParen, "expected '(' after function name");

    std::vector<Param> params;
    bool has_pattern_params = false;
    bool is_variadic = false;
    Pattern* pattern_param = nullptr;

    if (!check(TokenKind::RParen)) {
        // Check for literal pattern parameter (function-level pattern matching)
        Token first = peek();
        if (first.kind == TokenKind::IntLit) {
            // fn fib(0) -> i64 { ... }
            advance();
            has_pattern_params = true;
            auto* pat = arena_.make<IntLitPattern>();
            pat->kind = Pattern::Kind::IntLit;
            pat->loc = first.loc;
            pat->value = parseIntText(first.text);
            pattern_param = pat;
        } else if (first.kind == TokenKind::Minus) {
            // fn f(-1) -> i64 { ... }
            advance();
            Token num = expect(TokenKind::IntLit, "expected integer after '-' in pattern parameter");
            has_pattern_params = true;
            auto* pat = arena_.make<IntLitPattern>();
            pat->kind = Pattern::Kind::IntLit;
            pat->loc = first.loc;
            pat->value = -parseIntText(num.text);
            pattern_param = pat;
        } else if (first.kind == TokenKind::KwTrue || first.kind == TokenKind::KwFalse) {
            advance();
            has_pattern_params = true;
            auto* pat = arena_.make<BoolLitPattern>();
            pat->kind = Pattern::Kind::BoolLit;
            pat->loc = first.loc;
            pat->value = (first.kind == TokenKind::KwTrue);
            pattern_param = pat;
        } else {
            params.push_back(parseParam());
            while (match(TokenKind::Comma)) {
                // Check for variadic "..." (DotDot + Dot, or three Dots)
                if (check(TokenKind::DotDot)) {
                    advance(); // consume ..
                    expect(TokenKind::Dot, "expected '...' for variadic parameters");
                    is_variadic = true;
                    break;
                }
                if (check(TokenKind::Dot)) {
                    advance(); // first .
                    expect(TokenKind::Dot, "expected '..' in '...'");
                    expect(TokenKind::Dot, "expected '...' for variadic parameters");
                    is_variadic = true;
                    break;
                }
                params.push_back(parseParam());
            }
        }
    }
    expect(TokenKind::RParen, "expected ')' after parameters");

    TypeRef ret;
    if (match(TokenKind::Arrow)) {
        ret = parseType();
    }
    // else: ret stays default (empty name = infer from body)

    // Parse optional effect clause: "with io, atomic"
    std::vector<std::string_view> effect_names;
    bool has_effect_clause = false;
    if (check(TokenKind::KwWith)) {
        advance(); // consume 'with'
        has_effect_clause = true;
        // Parse "pure" as a special case (explicitly annotated pure)
        if (check(TokenKind::Ident) && peek().text == "pure") {
            advance(); // consume 'pure' — effect_names stays empty
        } else {
            // Parse comma-separated effect names
            Token eff = expect(TokenKind::Ident, "expected effect name after 'with'");
            effect_names.push_back(eff.text);
            while (match(TokenKind::Comma)) {
                Token eff2 = expect(TokenKind::Ident, "expected effect name after ','");
                effect_names.push_back(eff2.text);
            }
        }
    }

    skipNewlines();

    // Check for `= intrinsic`, body block, or forward declaration (no body)
    bool is_intrinsic = false;
    Expr* body = nullptr;
    if (check(TokenKind::Eq)) {
        advance();
        Token kw = expect(TokenKind::Ident, "expected 'intrinsic' after '='");
        if (kw.text == "intrinsic") {
            is_intrinsic = true;
        } else {
            diag_.error(kw.loc, std::string("expected 'intrinsic', got '") +
                        std::string(kw.text) + "'");
        }
    } else if (check(TokenKind::LBrace)) {
        body = parseBlockExpr();
    }
    // else: forward declaration (extern fn) — body remains nullptr

    auto* fn = arena_.make<FnDecl>();
    fn->name = nameTok.text;
    fn->param_count = static_cast<uint32_t>(params.size());
    fn->params = arena_.makeArray<Param>(params.size());
    for (size_t i = 0; i < params.size(); ++i) {
        fn->params[i] = params[i];
    }
    fn->return_type = ret;
    fn->body = body;
    fn->loc = loc;
    fn->is_intrinsic = is_intrinsic;
    fn->is_variadic = is_variadic;
    fn->has_pattern_params = has_pattern_params;
    fn->pattern_param = pattern_param;
    fn->type_param_count = static_cast<uint32_t>(type_params.size());
    if (!type_params.empty()) {
        fn->type_params = arena_.makeArray<TypeParam>(type_params.size());
        for (size_t i = 0; i < type_params.size(); ++i) {
            fn->type_params[i] = type_params[i];
        }
    }
    fn->effect_count = static_cast<uint32_t>(effect_names.size());
    fn->has_effect_clause = has_effect_clause;
    if (!effect_names.empty()) {
        fn->effect_names = arena_.makeArray<std::string_view>(effect_names.size());
        for (size_t i = 0; i < effect_names.size(); ++i) {
            fn->effect_names[i] = effect_names[i];
        }
    }
    return fn;
}

Param Parser::parseParam() {
    Token name = expect(TokenKind::Ident, "expected parameter name");
    expect(TokenKind::Colon, "expected ':' after parameter name");
    // Check for passing mode: "var Type" or "own Type"
    PassingMode mode = PassingMode::Borrow;
    if (check(TokenKind::KwVar)) {
        advance();
        mode = PassingMode::MutBorrow;
    } else if (check(TokenKind::KwOwn)) {
        advance();
        mode = PassingMode::Own;
    }
    TypeRef type = parseType();
    return {name.text, type, name.loc, mode};
}

TypeRef Parser::parseType() {
    // Const value in type position (for const generics): e.g. Buffer<i64, 4>
    if (check(TokenKind::IntLit)) {
        Token tok = advance();
        TypeRef ref;
        ref.kind = TypeRef::Kind::ConstVal;
        ref.name = tok.text;
        ref.loc = tok.loc;
        ref.const_value = parseIntText(tok.text);
        return ref;
    }

    // Never type: !
    if (check(TokenKind::Exclaim)) {
        Token bang = advance();
        TypeRef never_ref;
        never_ref.kind = TypeRef::Kind::Never;
        never_ref.name = "!";
        never_ref.loc = bang.loc;
        return never_ref;
    }

    // Array type: [T; N] where N is integer literal or const generic param name
    if (check(TokenKind::LBracket)) {
        Token bracket = advance(); // consume '['
        auto* elem = arena_.make<TypeRef>();
        *elem = parseType();
        expect(TokenKind::Semicolon, "expected ';' in array type [T; N]");
        TypeRef ref;
        ref.kind = TypeRef::Kind::Array;
        ref.name = "Array";
        ref.loc = bracket.loc;
        ref.array_element = elem;
        if (check(TokenKind::IntLit)) {
            Token size_tok = advance();
            ref.array_size = static_cast<uint32_t>(parseIntText(size_tok.text));
        } else if (check(TokenKind::Ident)) {
            Token name_tok = advance();
            ref.array_size_name = name_tok.text;
        } else {
            diag_.error(peek().loc, "expected integer or const generic parameter for array size");
        }
        expect(TokenKind::RBracket, "expected ']' after array type");
        return ref;
    }
    // Function type: fn(T1, T2) -> Ret
    if (check(TokenKind::KwFn)) {
        Token fn_tok = advance(); // consume 'fn'
        expect(TokenKind::LParen, "expected '(' in function type");
        std::vector<TypeRef> params;
        while (!check(TokenKind::RParen) && !check(TokenKind::Eof)) {
            params.push_back(parseType());
            if (check(TokenKind::Comma)) advance();
        }
        expect(TokenKind::RParen, "expected ')' in function type");
        expect(TokenKind::Arrow, "expected '->' in function type");
        auto* ret = arena_.make<TypeRef>();
        *ret = parseType();
        TypeRef ref;
        ref.kind = TypeRef::Kind::Fn;
        ref.name = "fn";
        ref.loc = fn_tok.loc;
        ref.fn_param_count = static_cast<uint32_t>(params.size());
        if (!params.empty()) {
            ref.fn_params = arena_.makeArray<TypeRef>(params.size());
            for (size_t i = 0; i < params.size(); ++i)
                ref.fn_params[i] = params[i];
        }
        ref.fn_return = ret;
        return ref;
    }
    // Tuple type: (T1, T2, ...)
    if (check(TokenKind::LParen)) {
        Token paren_tok = advance(); // consume '('
        std::vector<TypeRef> elems;
        while (!check(TokenKind::RParen) && !check(TokenKind::Eof)) {
            elems.push_back(parseType());
            if (!check(TokenKind::RParen)) {
                expect(TokenKind::Comma, "expected ',' or ')' in tuple type");
            }
        }
        expect(TokenKind::RParen, "expected ')' after tuple type");
        TypeRef ref;
        ref.kind = TypeRef::Kind::Tuple;
        ref.name = "Tuple";
        ref.loc = paren_tok.loc;
        ref.tuple_count = static_cast<uint32_t>(elems.size());
        if (!elems.empty()) {
            ref.tuple_elements = arena_.makeArray<TypeRef>(elems.size());
            for (size_t i = 0; i < elems.size(); ++i)
                ref.tuple_elements[i] = elems[i];
        }
        return ref;
    }
    // dyn TraitName — dynamic dispatch trait object (fat pointer)
    if (check(TokenKind::KwDyn)) {
        Token dyn_tok = advance(); // consume 'dyn'
        Token trait_tok = expect(TokenKind::Ident, "expected trait name after 'dyn'");
        TypeRef ref;
        ref.kind = TypeRef::Kind::Dyn;
        ref.name = trait_tok.text;  // trait name
        ref.loc = dyn_tok.loc;
        return ref;
    }
    Token tok = expect(TokenKind::Ident, "expected type name");
    if (tok.text == "Ptr" && check(TokenKind::Lt)) {
        advance(); // consume '<'
        bool is_var = false;
        if (check(TokenKind::KwVar)) {
            advance(); // consume 'var'
            is_var = true;
        }
        auto* pointee = arena_.make<TypeRef>();
        *pointee = parseType();
        expect(TokenKind::Gt, "expected '>' after Ptr type parameter");
        TypeRef ref;
        ref.kind = TypeRef::Kind::Ptr;
        ref.name = is_var ? "Ptr<var>" : "Ptr";
        ref.loc = tok.loc;
        ref.pointee = pointee;
        ref.is_ptr_var = is_var;
        return ref;
    }
    // Slice<T> — fat pointer: {data: Ptr<T>, len: u64}
    if (tok.text == "Slice" && check(TokenKind::Lt)) {
        advance(); // consume '<'
        auto* elem = arena_.make<TypeRef>();
        *elem = parseType();
        expect(TokenKind::Gt, "expected '>' after Slice type parameter");
        TypeRef ref;
        ref.kind = TypeRef::Kind::Named;
        ref.name = "Slice";
        ref.loc = tok.loc;
        ref.pointee = elem;  // reuse pointee field for element type
        return ref;
    }
    // Generic type: Name<T1, T2, ...>
    if (check(TokenKind::Lt)) {
        advance(); // consume '<'
        std::vector<TypeRef> args;
        while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
            args.push_back(parseType());
            if (!check(TokenKind::Gt)) {
                expect(TokenKind::Comma, "expected ',' or '>' in type argument list");
            }
        }
        expect(TokenKind::Gt, "expected '>' after type arguments");
        TypeRef ref;
        ref.kind = TypeRef::Kind::Named;
        ref.name = tok.text;
        ref.loc = tok.loc;
        ref.type_arg_count = static_cast<uint32_t>(args.size());
        if (!args.empty()) {
            ref.type_args = arena_.makeArray<TypeRef>(args.size());
            for (size_t i = 0; i < args.size(); ++i)
                ref.type_args[i] = args[i];
        }
        return ref;
    }
    TypeRef named_ref;
    named_ref.kind = TypeRef::Kind::Named;
    named_ref.name = tok.text;
    named_ref.loc = tok.loc;
    return named_ref;
}

// --- Expressions ---

Parser::InfixBP Parser::infixBP(TokenKind kind) {
    switch (kind) {
        case TokenKind::Pipe:      return {10, 11};
        case TokenKind::KwOr:      return {20, 21};
        case TokenKind::KwAnd:     return {30, 31};
        case TokenKind::EqEq:
        case TokenKind::NotEq:     return {40, 41};
        case TokenKind::Lt:
        case TokenKind::LtEq:
        case TokenKind::Gt:
        case TokenKind::GtEq:     return {50, 51};
        case TokenKind::BitOr:     return {60, 61};
        case TokenKind::BitXor:    return {70, 71};
        case TokenKind::Ampersand: return {80, 81};  // bitwise AND (infix)
        case TokenKind::Plus:
        case TokenKind::PlusWrap:
        case TokenKind::PlusSat:
        case TokenKind::Minus:
        case TokenKind::MinusWrap:
        case TokenKind::MinusSat:  return {90, 91};
        case TokenKind::Shl:
        case TokenKind::Shr:       return {100, 101};
        case TokenKind::Star:
        case TokenKind::StarWrap:
        case TokenKind::Slash:
        case TokenKind::Percent:   return {110, 111};
        case TokenKind::KwAs:      return {130, 131};
        case TokenKind::Dot:       return {200, 201};
        default:                   return {0, 0};
    }
}

uint8_t Parser::prefixBP(TokenKind kind) {
    switch (kind) {
        case TokenKind::Minus:
        case TokenKind::KwNot:
        case TokenKind::Tilde:     // ~x (bitwise NOT)
        case TokenKind::Star:      // *ptr (deref)
        case TokenKind::Ampersand: // &x (addr-of)
            return 125;
        default:
            return 0;
    }
}

BinOpKind Parser::tokenToBinOp(TokenKind kind) {
    switch (kind) {
        case TokenKind::Plus:      return BinOpKind::Add;
        case TokenKind::PlusWrap:  return BinOpKind::AddWrap;
        case TokenKind::PlusSat:   return BinOpKind::AddSat;
        case TokenKind::Minus:     return BinOpKind::Sub;
        case TokenKind::MinusWrap: return BinOpKind::SubWrap;
        case TokenKind::MinusSat:  return BinOpKind::SubSat;
        case TokenKind::Star:      return BinOpKind::Mul;
        case TokenKind::StarWrap:  return BinOpKind::MulWrap;
        case TokenKind::Slash:     return BinOpKind::Div;
        case TokenKind::Percent:   return BinOpKind::Mod;
        case TokenKind::EqEq:      return BinOpKind::Eq;
        case TokenKind::NotEq:     return BinOpKind::NotEq;
        case TokenKind::Lt:        return BinOpKind::Lt;
        case TokenKind::LtEq:      return BinOpKind::LtEq;
        case TokenKind::Gt:        return BinOpKind::Gt;
        case TokenKind::GtEq:      return BinOpKind::GtEq;
        case TokenKind::KwAnd:     return BinOpKind::And;
        case TokenKind::KwOr:      return BinOpKind::Or;
        case TokenKind::Ampersand: return BinOpKind::BitAnd;
        case TokenKind::BitOr:     return BinOpKind::BitOr;
        case TokenKind::BitXor:    return BinOpKind::BitXor;
        case TokenKind::Shl:       return BinOpKind::Shl;
        case TokenKind::Shr:       return BinOpKind::Shr;
        default:                   return BinOpKind::Add; // unreachable
    }
}

Expr* Parser::parseExprInfix(Expr* lhs, uint8_t minBP) {
    while (true) {
        // Check if the next meaningful token is on a different line than
        // the current position. If so, and the token is an ambiguous
        // prefix/infix operator (Star or Ampersand), treat as end of
        // expression — the next line starts a new statement.
        uint32_t lhs_line = lhs->loc.line;
        skipNewlines();
        TokenKind op = peek().kind;
        bool on_new_line = peek().loc.line > lhs_line;
        if (on_new_line && (op == TokenKind::Star || op == TokenKind::Ampersand ||
                             op == TokenKind::Tilde)) {
            break;
        }
        // Try operator: expr? — postfix, same precedence as '.'
        if (op == TokenKind::Question && !on_new_line) {
            if (200 < minBP) break;
            advance(); // consume '?'
            auto* te = arena_.make<TryExpr>();
            te->kind = Expr::Kind::Try;
            te->loc = lhs->loc;
            te->operand = lhs;
            lhs = te;
            continue;
        }
        // Index access or slice: arr[idx] or arr[start..end]
        if (op == TokenKind::LBracket && !on_new_line) {
            if (200 < minBP) break; // same precedence as Dot
            advance(); // consume '['
            skipNewlines();
            // Check for arr[..end] (start omitted)
            if (check(TokenKind::DotDot) || check(TokenKind::DotDotEq)) {
                advance(); // consume '..' or '..='
                skipNewlines();
                Expr* end_expr = parseExpr();
                skipNewlines();
                expect(TokenKind::RBracket, "expected ']' after slice");
                auto* se = arena_.make<SliceExprNode>();
                se->kind = Expr::Kind::SliceExpr;
                se->loc = lhs->loc;
                se->array = lhs;
                se->start = nullptr;
                se->end = end_expr;
                lhs = se;
                continue;
            }
            Expr* idx = parseExpr();
            skipNewlines();
            // Check for arr[start..end] or arr[start..]
            if (check(TokenKind::DotDot) || check(TokenKind::DotDotEq)) {
                advance(); // consume '..' or '..='
                skipNewlines();
                Expr* end_expr = nullptr;
                if (!check(TokenKind::RBracket)) {
                    end_expr = parseExpr();
                    skipNewlines();
                }
                expect(TokenKind::RBracket, "expected ']' after slice");
                auto* se = arena_.make<SliceExprNode>();
                se->kind = Expr::Kind::SliceExpr;
                se->loc = lhs->loc;
                se->array = lhs;
                se->start = idx;
                se->end = end_expr;
                lhs = se;
                continue;
            }
            expect(TokenKind::RBracket, "expected ']' after index");
            auto* ia = arena_.make<IndexAccessExpr>();
            ia->kind = Expr::Kind::IndexAccess;
            ia->loc = lhs->loc;
            ia->array = lhs;
            ia->index = idx;
            lhs = ia;
            continue;
        }

        auto [lBP, rBP] = infixBP(op);
        if (lBP == 0 || lBP < minBP) break;

        Token opTok = advance();
        skipNewlines();

        // Dot operator: field access or method call
        if (op == TokenKind::Dot) {
            // Allow integer literal for tuple field access: t.0, t.1
            Token field = check(TokenKind::IntLit)
                ? advance()
                : expect(TokenKind::Ident, "expected field name after '.'");
            // Check for method call: expr.method(args)
            if (check(TokenKind::LParen)) {
                advance(); // consume '('
                std::vector<Expr*> args;
                if (!check(TokenKind::RParen)) {
                    args.push_back(parseExpr());
                    while (match(TokenKind::Comma)) {
                        args.push_back(parseExpr());
                    }
                }
                expect(TokenKind::RParen, "expected ')' after method arguments");
                auto* mc = arena_.make<MethodCallExpr>();
                mc->kind = Expr::Kind::MethodCall;
                mc->loc = opTok.loc;
                mc->object = lhs;
                mc->method_name = field.text;
                mc->arg_count = static_cast<uint32_t>(args.size());
                mc->args = arena_.makeArray<Expr*>(args.size());
                for (size_t i = 0; i < args.size(); ++i) {
                    mc->args[i] = args[i];
                }
                lhs = mc;
            } else {
                auto* fa = arena_.make<FieldAccessExpr>();
                fa->kind = Expr::Kind::FieldAccess;
                fa->loc = opTok.loc;
                fa->object = lhs;
                // For tuple field access (t.0, t.1), convert index to _N field name
                if (field.kind == TokenKind::IntLit) {
                    std::string name = "_" + std::string(field.text);
                    char* buf = arena_.makeArray<char>(name.size());
                    std::memcpy(buf, name.data(), name.size());
                    fa->field_name = std::string_view(buf, name.size());
                } else {
                    fa->field_name = field.text;
                }
                lhs = fa;
            }
            continue;
        }

        // Pipe operator: desugar to CallExpr
        if (op == TokenKind::Pipe) {
            Expr* rhs = parsePrimary();
            if (rhs->kind == Expr::Kind::Ident) {
                // a |> f → CallExpr(f, [a])
                auto* ident = static_cast<IdentExpr*>(rhs);
                // Check if followed by '(' — it was parsed as ident, but user wrote f(b,c)
                // parsePrimary already consumed ident and would have parsed call if '(' followed
                auto* call = arena_.make<CallExpr>();
                call->kind = Expr::Kind::Call;
                call->loc = ident->loc;
                call->callee = ident->name;
                call->arg_count = 1;
                call->args = arena_.makeArray<Expr*>(1);
                call->args[0] = lhs;
                lhs = call;
            } else if (rhs->kind == Expr::Kind::Call) {
                // a |> f(b, c) → CallExpr(f, [a, b, c])
                auto* orig_call = static_cast<CallExpr*>(rhs);
                uint32_t new_count = orig_call->arg_count + 1;
                auto** new_args = arena_.makeArray<Expr*>(new_count);
                new_args[0] = lhs;
                for (uint32_t i = 0; i < orig_call->arg_count; ++i) {
                    new_args[i + 1] = orig_call->args[i];
                }
                orig_call->args = new_args;
                orig_call->arg_count = new_count;
                lhs = orig_call;
            } else {
                diag_.error(opTok.loc, "expected function name or call after '|>'");
                lhs = rhs;
            }
            continue;
        }

        // Cast operator: expr as Type
        if (op == TokenKind::KwAs) {
            auto* cast = arena_.make<CastExpr>();
            cast->kind = Expr::Kind::Cast;
            cast->loc = opTok.loc;
            cast->operand = lhs;
            cast->target = parseType();
            lhs = cast;
            continue;
        }

        Expr* rhs = parseExpr(rBP);

        auto* bin = arena_.make<BinOpExpr>();
        bin->kind = Expr::Kind::BinOp;
        bin->loc = opTok.loc;
        bin->op = tokenToBinOp(op);
        bin->lhs = lhs;
        bin->rhs = rhs;
        lhs = bin;
    }

    return lhs;
}

Expr* Parser::parseExpr(uint8_t minBP) {
    Expr* lhs = parsePrimary();
    return parseExprInfix(lhs, minBP);
}

Expr* Parser::parsePrimary() {
    skipNewlines();
    Token tok = peek();

    // Prefix operators
    if (tok.kind == TokenKind::Minus || tok.kind == TokenKind::KwNot ||
        tok.kind == TokenKind::Tilde) {
        Token opTok = advance();
        uint8_t rBP = prefixBP(opTok.kind);
        Expr* operand = parseExpr(rBP);

        auto* unary = arena_.make<UnaryOpExpr>();
        unary->kind = Expr::Kind::UnaryOp;
        unary->loc = opTok.loc;
        unary->op = (opTok.kind == TokenKind::Minus) ? UnaryOpKind_t::Neg
                   : (opTok.kind == TokenKind::Tilde) ? UnaryOpKind_t::BitNot
                   : UnaryOpKind_t::Not;
        unary->operand = operand;
        return unary;
    }

    // Dereference: *expr
    if (tok.kind == TokenKind::Star) {
        Token opTok = advance();
        uint8_t rBP = prefixBP(opTok.kind);
        Expr* operand = parseExpr(rBP);

        auto* unary = arena_.make<UnaryOpExpr>();
        unary->kind = Expr::Kind::UnaryOp;
        unary->loc = opTok.loc;
        unary->op = UnaryOpKind_t::Deref;
        unary->operand = operand;
        return unary;
    }

    // Address-of: &expr or &var expr
    if (tok.kind == TokenKind::Ampersand) {
        Token opTok = advance();
        bool is_addr_var = false;
        if (check(TokenKind::KwVar)) {
            advance(); // consume 'var'
            is_addr_var = true;
        }
        uint8_t rBP = prefixBP(opTok.kind);
        Expr* operand = parseExpr(rBP);

        auto* unary = arena_.make<UnaryOpExpr>();
        unary->kind = Expr::Kind::UnaryOp;
        unary->loc = opTok.loc;
        unary->op = is_addr_var ? UnaryOpKind_t::AddrOfVar : UnaryOpKind_t::AddrOf;
        unary->operand = operand;
        return unary;
    }

    // Integer literal
    if (tok.kind == TokenKind::IntLit) {
        advance();
        auto* lit = arena_.make<IntLitExpr>();
        lit->kind = Expr::Kind::IntLit;
        lit->loc = tok.loc;
        lit->value = parseIntText(tok.text);
        lit->suffix = intSuffix(tok.text);
        return lit;
    }

    // Character literal: 'c' → IntLitExpr with u8 value
    if (tok.kind == TokenKind::CharLit) {
        advance();
        auto* lit = arena_.make<IntLitExpr>();
        lit->kind = Expr::Kind::IntLit;
        lit->loc = tok.loc;
        // Parse char value from token text (includes surrounding quotes)
        auto text = tok.text;
        if (text.size() >= 3 && text[1] == '\\') {
            // Escape sequence
            switch (text[2]) {
                case 'n':  lit->value = '\n'; break;
                case 't':  lit->value = '\t'; break;
                case '\\': lit->value = '\\'; break;
                case '\'': lit->value = '\''; break;
                case '0':  lit->value = '\0'; break;
                case 'r':  lit->value = '\r'; break;
                default:   lit->value = text[2]; break;
            }
        } else if (text.size() >= 3) {
            lit->value = static_cast<uint8_t>(text[1]);
        } else {
            lit->value = 0;
        }
        lit->suffix = "u8";
        return lit;
    }

    // Float literal
    if (tok.kind == TokenKind::FloatLit) {
        advance();
        auto* lit = arena_.make<FloatLitExpr>();
        lit->kind = Expr::Kind::FloatLit;
        lit->loc = tok.loc;
        std::string text(tok.text);
        lit->is_f32 = (!text.empty() && text.back() == 'f');
        if (lit->is_f32) text.pop_back();
        lit->value = std::stod(text);
        return lit;
    }

    // Bool literal
    if (tok.kind == TokenKind::KwTrue || tok.kind == TokenKind::KwFalse) {
        advance();
        auto* lit = arena_.make<BoolLitExpr>();
        lit->kind = Expr::Kind::BoolLit;
        lit->loc = tok.loc;
        lit->value = (tok.kind == TokenKind::KwTrue);
        return lit;
    }

    // Null literal
    if (tok.kind == TokenKind::KwNull) {
        advance();
        auto* lit = arena_.make<NullLitExpr>();
        lit->kind = Expr::Kind::NullLit;
        lit->loc = tok.loc;
        return lit;
    }

    // String literal
    if (tok.kind == TokenKind::StringLit) {
        advance();
        auto* lit = arena_.make<StringLitExpr>();
        lit->kind = Expr::Kind::StringLit;
        lit->loc = tok.loc;
        std::string_view raw = tok.text;
        // Allocate buffer for processed string (worst case same size as raw - 2 quotes)
        auto* buf = static_cast<char*>(arena_.allocate(raw.size(), 1));
        uint32_t len = 0;
        for (size_t i = 1; i + 1 < raw.size(); ++i) {
            if (raw[i] == '\\' && i + 2 < raw.size()) {
                char esc = raw[i + 1];
                if (esc == 'n')       { buf[len++] = '\n'; ++i; }
                else if (esc == 't')  { buf[len++] = '\t'; ++i; }
                else if (esc == '\\') { buf[len++] = '\\'; ++i; }
                else if (esc == '"')  { buf[len++] = '"'; ++i; }
                else                  { buf[len++] = raw[i]; }
            } else {
                buf[len++] = raw[i];
            }
        }
        lit->data = buf;
        lit->length = len;
        return lit;
    }

    // Identifier, function call, struct literal, enum access, or union variant
    if (tok.kind == TokenKind::Ident) {
        advance();
        if (check(TokenKind::LParen)) {
            return parseCallExpr(tok.text, tok.loc);
        }
        if (check(TokenKind::LBrace) && struct_names_.count(tok.text)) {
            return parseStructLit(tok.text, tok.loc);
        }
        // Generic function call: f<T1, T2>(args)
        // Speculatively try to parse <type_args>( before falling through to comparison
        if (check(TokenKind::Lt) && !struct_names_.count(tok.text) && !union_names_.count(tok.text)) {
            auto lexer_snap = lexer_.save();
            Token saved_current = current_;
            Token saved_previous = previous_;
            bool saved_has = has_current_;

            advance(); // consume '<'
            std::vector<TypeRef> type_args;
            bool valid = true;
            while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
                if (!check(TokenKind::Ident) && !check(TokenKind::LBracket) &&
                    !check(TokenKind::KwFn) && !check(TokenKind::Exclaim) &&
                    !check(TokenKind::IntLit)) {
                    valid = false;
                    break;
                }
                type_args.push_back(parseType());
                if (!check(TokenKind::Gt)) {
                    if (!match(TokenKind::Comma)) { valid = false; break; }
                }
            }
            if (valid && !type_args.empty() && check(TokenKind::Gt)) {
                advance(); // consume '>'
                if (check(TokenKind::LParen)) {
                    // It's a generic function call!
                    advance(); // consume '('
                    skipNewlines();
                    std::vector<Expr*> args;
                    if (!check(TokenKind::RParen)) {
                        args.push_back(parseExpr());
                        while (match(TokenKind::Comma)) {
                            skipNewlines();
                            args.push_back(parseExpr());
                        }
                    }
                    skipNewlines();
                    expect(TokenKind::RParen, "expected ')'");

                    auto* call = arena_.make<CallExpr>();
                    call->kind = Expr::Kind::Call;
                    call->loc = tok.loc;
                    call->callee = tok.text;
                    call->arg_count = static_cast<uint32_t>(args.size());
                    call->args = arena_.makeArray<Expr*>(args.size());
                    for (size_t i = 0; i < args.size(); ++i) {
                        call->args[i] = args[i];
                    }
                    call->type_arg_count = static_cast<uint32_t>(type_args.size());
                    call->type_args = arena_.makeArray<TypeRef>(type_args.size());
                    for (size_t i = 0; i < type_args.size(); ++i) {
                        call->type_args[i] = type_args[i];
                    }
                    return call;
                }
            }
            // Not a generic call — restore and fall through to comparison
            lexer_.restore(lexer_snap);
            current_ = saved_current;
            previous_ = saved_previous;
            has_current_ = saved_has;
        }
        // Generic struct literal or union variant: Name<T1, T2> { ... } or Name<T>::Variant(...)
        if (check(TokenKind::Lt) && (struct_names_.count(tok.text) || union_names_.count(tok.text))) {
            // Save state in case this is a comparison, not generic args
            auto lexer_snap = lexer_.save();
            Token saved_current = current_;
            Token saved_previous = previous_;
            bool saved_has = has_current_;

            // Try to parse type args
            advance(); // consume '<'
            std::vector<TypeRef> type_args;
            bool valid = true;
            while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
                // If we see something that can't be a type, bail out
                if (!check(TokenKind::Ident) && !check(TokenKind::LBracket) &&
                    !check(TokenKind::KwFn) && !check(TokenKind::Exclaim) &&
                    !check(TokenKind::IntLit)) {
                    valid = false;
                    break;
                }
                type_args.push_back(parseType());
                if (!check(TokenKind::Gt)) {
                    if (!match(TokenKind::Comma)) { valid = false; break; }
                }
            }
            if (valid && check(TokenKind::Gt)) {
                advance(); // consume '>'
                skipNewlines();

                // Build mangled name helper
                std::string mangled = std::string(tok.text);
                for (auto& ta : type_args) {
                    mangled += "_";
                    if (ta.kind == TypeRef::Kind::ConstVal)
                        mangled += std::to_string(ta.const_value);
                    else
                        mangled += ta.name;
                }
                char* buf = arena_.makeArray<char>(mangled.size());
                std::memcpy(buf, mangled.data(), mangled.size());
                std::string_view mangled_sv(buf, mangled.size());

                if (check(TokenKind::LBrace)) {
                    // Generic struct literal: Name<T> { ... }
                    return parseStructLit(mangled_sv, tok.loc);
                }
                if (check(TokenKind::ColonColon)) {
                    // Generic union variant: Name<T>::Variant(...)
                    advance(); // consume ::
                    Token variant_tok = expect(TokenKind::Ident, "expected variant name after '::'");
                    auto* uv = arena_.make<UnionVariantExpr>();
                    uv->kind = Expr::Kind::UnionVariant;
                    uv->loc = tok.loc;
                    uv->union_name = mangled_sv;
                    uv->variant_name = variant_tok.text;
                    uv->payload = nullptr;
                    if (match(TokenKind::LParen)) {
                        if (!check(TokenKind::RParen)) {
                            uv->payload = parseExpr();
                        }
                        expect(TokenKind::RParen, "expected ')' after variant payload");
                    }
                    return uv;
                }
            }

            // Not a generic construct — restore and fall through
            lexer_.restore(lexer_snap);
            current_ = saved_current;
            previous_ = saved_previous;
            has_current_ = saved_has;
        }
        // Union variant: Name::Variant or Name::Variant(payload)
        if (check(TokenKind::ColonColon) && union_names_.count(tok.text)) {
            advance(); // consume ::
            Token variant_tok = expect(TokenKind::Ident, "expected variant name after '::'");
            auto* uv = arena_.make<UnionVariantExpr>();
            uv->kind = Expr::Kind::UnionVariant;
            uv->loc = tok.loc;
            uv->union_name = tok.text;
            uv->variant_name = variant_tok.text;
            uv->payload = nullptr;
            // Check for struct literal shorthand: Shape::Circle { ... }
            if (check(TokenKind::LBrace) && struct_names_.count(variant_tok.text)) {
                uv->payload = parseStructLit(variant_tok.text, variant_tok.loc);
            } else if (match(TokenKind::LParen)) {
                // Explicit payload: Shape::Circle(expr) or Option::Some(42)
                if (!check(TokenKind::RParen)) {
                    uv->payload = parseExpr();
                }
                expect(TokenKind::RParen, "expected ')' after variant payload");
            }
            return uv;
        }
        // Enum access: Name.Variant
        if (check(TokenKind::Dot) && enum_names_.count(tok.text)) {
            advance(); // consume .
            Token variant_tok = expect(TokenKind::Ident, "expected variant name after '.'");
            auto* ea = arena_.make<EnumAccessExpr>();
            ea->kind = Expr::Kind::EnumAccess;
            ea->loc = tok.loc;
            ea->enum_name = tok.text;
            ea->variant_name = variant_tok.text;
            return ea;
        }
        auto* ident = arena_.make<IdentExpr>();
        ident->kind = Expr::Kind::Ident;
        ident->loc = tok.loc;
        ident->name = tok.text;
        return ident;
    }

    // Array literal: [expr1, expr2, ...]
    if (tok.kind == TokenKind::LBracket) {
        advance(); // consume '['
        skipNewlines();
        std::vector<Expr*> elems;
        if (!check(TokenKind::RBracket)) {
            elems.push_back(parseExpr());
            while (match(TokenKind::Comma)) {
                skipNewlines();
                if (check(TokenKind::RBracket)) break; // trailing comma
                elems.push_back(parseExpr());
            }
        }
        skipNewlines();
        expect(TokenKind::RBracket, "expected ']' after array elements");
        auto* arr = arena_.make<ArrayLitExpr>();
        arr->kind = Expr::Kind::ArrayLit;
        arr->loc = tok.loc;
        arr->count = static_cast<uint32_t>(elems.size());
        arr->elements = arena_.makeArray<Expr*>(elems.size());
        for (size_t i = 0; i < elems.size(); ++i) {
            arr->elements[i] = elems[i];
        }
        return arr;
    }

    // Match expression
    if (tok.kind == TokenKind::KwMatch) {
        return parseMatchExpr();
    }

    // If expression
    if (tok.kind == TokenKind::KwIf) {
        return parseIfExpr();
    }

    // sizeof(Type) -> compile-time integer constant
    if (tok.kind == TokenKind::KwSizeof) {
        advance(); // consume 'sizeof'
        expect(TokenKind::LParen, "expected '(' after 'sizeof'");
        TypeRef target = parseType();
        expect(TokenKind::RParen, "expected ')' after sizeof type");
        auto* e = arena_.make<SizeofExpr>();
        e->kind = Expr::Kind::Sizeof;
        e->loc = tok.loc;
        e->target = target;
        return e;
    }

    // alignof(Type) -> compile-time integer constant
    if (tok.kind == TokenKind::KwAlignof) {
        advance(); // consume 'alignof'
        expect(TokenKind::LParen, "expected '(' after 'alignof'");
        TypeRef target = parseType();
        expect(TokenKind::RParen, "expected ')' after alignof type");
        auto* e = arena_.make<AlignofExpr>();
        e->kind = Expr::Kind::Alignof;
        e->loc = tok.loc;
        e->target = target;
        return e;
    }

    // offsetof(Type, field) -> compile-time field offset
    if (tok.kind == TokenKind::KwOffsetof) {
        advance(); // consume 'offsetof'
        expect(TokenKind::LParen, "expected '(' after 'offsetof'");
        TypeRef target = parseType();
        expect(TokenKind::Comma, "expected ',' after offsetof type");
        Token field = expect(TokenKind::Ident, "expected field name in offsetof");
        expect(TokenKind::RParen, "expected ')' after offsetof field");
        auto* e = arena_.make<OffsetofExpr>();
        e->kind = Expr::Kind::Offsetof;
        e->loc = tok.loc;
        e->target = target;
        e->field_name = field.text;
        return e;
    }

    // Inline assembly: asm { "line1"; "line2"; }
    //   or extended:   asm("template" : "=r"(out) : "r"(in) : "cc")
    if (tok.kind == TokenKind::KwAsm) {
        advance(); // consume 'asm'

        auto* asmExpr = arena_.make<InlineAsmExpr>();
        asmExpr->kind = Expr::Kind::InlineAsm;
        asmExpr->loc = tok.loc;
        asmExpr->outputs = nullptr;
        asmExpr->output_count = 0;
        asmExpr->inputs = nullptr;
        asmExpr->input_count = 0;
        asmExpr->clobbers = nullptr;
        asmExpr->clobber_count = 0;

        if (check(TokenKind::LParen)) {
            // Extended asm: asm("template" : "=r"(out) : "r"(in) : "cc")
            advance(); // consume '('
            skipNewlines();

            // Parse template string(s) — one or more string literals
            std::vector<StringLitExpr*> lines;
            while (check(TokenKind::StringLit)) {
                Token str_tok = advance();
                auto* sle = arena_.make<StringLitExpr>();
                sle->kind = Expr::Kind::StringLit;
                sle->loc = str_tok.loc;
                sle->data = str_tok.text.data() + 1;
                sle->length = static_cast<uint32_t>(str_tok.text.size() - 2);
                lines.push_back(sle);
                while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
            }
            asmExpr->line_count = static_cast<uint32_t>(lines.size());
            asmExpr->lines = arena_.makeArray<StringLitExpr*>(lines.size());
            for (size_t i = 0; i < lines.size(); ++i) asmExpr->lines[i] = lines[i];

            // Parse output constraints (after first ':')
            if (match(TokenKind::Colon)) {
                skipNewlines();
                std::vector<AsmOperand> outputs;
                while (check(TokenKind::StringLit)) {
                    AsmOperand op;
                    Token ct = advance();
                    op.constraint = ct.text.substr(1, ct.text.size() - 2); // strip quotes
                    expect(TokenKind::LParen, "expected '(' after constraint");
                    Token name = expect(TokenKind::Ident, "expected variable name");
                    op.var_name = name.text;
                    expect(TokenKind::RParen, "expected ')' after variable name");
                    outputs.push_back(op);
                    if (!match(TokenKind::Comma)) break;
                    skipNewlines();
                }
                if (!outputs.empty()) {
                    asmExpr->output_count = static_cast<uint32_t>(outputs.size());
                    asmExpr->outputs = arena_.makeArray<AsmOperand>(outputs.size());
                    for (size_t i = 0; i < outputs.size(); ++i) asmExpr->outputs[i] = outputs[i];
                }

                // Parse input constraints (after second ':')
                if (match(TokenKind::Colon)) {
                    skipNewlines();
                    std::vector<AsmOperand> inputs;
                    while (check(TokenKind::StringLit)) {
                        AsmOperand op;
                        Token ct = advance();
                        op.constraint = ct.text.substr(1, ct.text.size() - 2);
                        expect(TokenKind::LParen, "expected '(' after constraint");
                        Token name = expect(TokenKind::Ident, "expected variable name");
                        op.var_name = name.text;
                        expect(TokenKind::RParen, "expected ')' after variable name");
                        inputs.push_back(op);
                        if (!match(TokenKind::Comma)) break;
                        skipNewlines();
                    }
                    if (!inputs.empty()) {
                        asmExpr->input_count = static_cast<uint32_t>(inputs.size());
                        asmExpr->inputs = arena_.makeArray<AsmOperand>(inputs.size());
                        for (size_t i = 0; i < inputs.size(); ++i) asmExpr->inputs[i] = inputs[i];
                    }

                    // Parse clobber list (after third ':')
                    if (match(TokenKind::Colon)) {
                        skipNewlines();
                        std::vector<std::string_view> clobbers;
                        while (check(TokenKind::StringLit)) {
                            Token ct = advance();
                            clobbers.push_back(ct.text.substr(1, ct.text.size() - 2));
                            if (!match(TokenKind::Comma)) break;
                            skipNewlines();
                        }
                        if (!clobbers.empty()) {
                            asmExpr->clobber_count = static_cast<uint32_t>(clobbers.size());
                            asmExpr->clobbers = arena_.makeArray<std::string_view>(clobbers.size());
                            for (size_t i = 0; i < clobbers.size(); ++i) asmExpr->clobbers[i] = clobbers[i];
                        }
                    }
                }
            }

            expect(TokenKind::RParen, "expected ')' after asm expression");
        } else {
            // Legacy asm: asm { "line1"; "line2"; }
            expect(TokenKind::LBrace, "expected '{' or '(' after 'asm'");
            skipNewlines();
            std::vector<StringLitExpr*> lines;
            while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
                Token str_tok = expect(TokenKind::StringLit, "expected string literal in asm block");
                auto* sle = arena_.make<StringLitExpr>();
                sle->kind = Expr::Kind::StringLit;
                sle->loc = str_tok.loc;
                sle->data = str_tok.text.data() + 1;
                sle->length = static_cast<uint32_t>(str_tok.text.size() - 2);
                lines.push_back(sle);
                while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
            }
            expect(TokenKind::RBrace, "expected '}' after asm block");
            asmExpr->line_count = static_cast<uint32_t>(lines.size());
            asmExpr->lines = arena_.makeArray<StringLitExpr*>(lines.size());
            for (size_t i = 0; i < lines.size(); ++i) asmExpr->lines[i] = lines[i];
        }

        return asmExpr;
    }

    // Labeled loop: 'label: loop/for/while { ... }
    if (tok.kind == TokenKind::Label) {
        std::string_view label_text = tok.text;  // includes leading '
        advance(); // consume label
        expect(TokenKind::Colon, "expected ':' after loop label");
        skipNewlines();
        if (check(TokenKind::KwLoop)) {
            auto* e = static_cast<LoopExpr*>(parseLoopExpr());
            if (e) e->label = label_text;
            return e;
        } else if (check(TokenKind::KwFor)) {
            auto* e = parseForRange();
            if (e && e->kind == Expr::Kind::ForRange)
                static_cast<ForRangeExpr*>(e)->label = label_text;
            else if (e && e->kind == Expr::Kind::ForEach)
                static_cast<ForEachExpr*>(e)->label = label_text;
            return e;
        } else if (check(TokenKind::KwWhile)) {
            auto* e = static_cast<WhileLoopExpr*>(parseWhileLoop());
            if (e) e->label = label_text;
            return e;
        } else {
            diag_.error(tok.loc, "expected 'loop', 'for', or 'while' after label");
            return nullptr;
        }
    }

    // Loop expression
    if (tok.kind == TokenKind::KwLoop) {
        return parseLoopExpr();
    }

    // For-in-range: for i in start..end { body }
    if (tok.kind == TokenKind::KwFor) {
        return parseForRange();
    }

    // While loop: while cond { body }
    if (tok.kind == TokenKind::KwWhile) {
        return parseWhileLoop();
    }

    // Lambda or Block expression
    if (tok.kind == TokenKind::LBrace) {
        if (isLambdaStart()) {
            SourceLocation loc = peek().loc;
            advance(); // consume '{'
            skipNewlines();
            return parseLambdaExpr(loc);
        }
        return parseBlockExpr();
    }

    // Parenthesized expression or tuple literal
    if (tok.kind == TokenKind::LParen) {
        advance();
        skipNewlines();
        Expr* first = parseExpr();
        skipNewlines();
        // If comma follows → tuple literal
        if (check(TokenKind::Comma)) {
            std::vector<Expr*> elems;
            elems.push_back(first);
            while (match(TokenKind::Comma)) {
                skipNewlines();
                if (check(TokenKind::RParen)) break; // trailing comma
                elems.push_back(parseExpr());
                skipNewlines();
            }
            expect(TokenKind::RParen, "expected ')' after tuple elements");
            auto* tuple = arena_.make<TupleLitExpr>();
            tuple->kind = Expr::Kind::TupleLit;
            tuple->loc = tok.loc;
            tuple->count = static_cast<uint32_t>(elems.size());
            tuple->elements = arena_.makeArray<Expr*>(elems.size());
            for (size_t i = 0; i < elems.size(); ++i)
                tuple->elements[i] = elems[i];
            return tuple;
        }
        expect(TokenKind::RParen, "expected ')'");
        return first;
    }

    // Uninit expression
    if (tok.kind == TokenKind::KwUninit) {
        advance();
        auto* e = arena_.make<Expr>();
        e->kind = Expr::Kind::Uninit;
        e->loc = tok.loc;
        return e;
    }

    // Return
    if (tok.kind == TokenKind::KwReturn) {
        advance();
        auto* ret = arena_.make<ReturnExpr>();
        ret->kind = Expr::Kind::Return;
        ret->loc = tok.loc;
        // Check if there's a value to return
        if (!check(TokenKind::RBrace) && !check(TokenKind::Newline) &&
            !check(TokenKind::Semicolon) && !check(TokenKind::Eof)) {
            ret->value = parseExpr();
        } else {
            ret->value = nullptr;
        }
        return ret;
    }

    diag_.error(tok.loc, std::string("unexpected token '") + std::string(tok.text) + "'");
    advance();
    // Return a dummy int literal to allow parsing to continue
    auto* dummy = arena_.make<IntLitExpr>();
    dummy->kind = Expr::Kind::IntLit;
    dummy->loc = tok.loc;
    dummy->value = 0;
    return dummy;
}

Expr* Parser::parseIfExpr() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwIf, "expected 'if'");

    Expr* cond = parseExpr();
    skipNewlines();
    Expr* then_br = parseBlockExpr();

    Expr* else_br = nullptr;
    skipNewlines();
    if (match(TokenKind::KwElse)) {
        skipNewlines();
        if (check(TokenKind::KwIf)) {
            else_br = parseIfExpr();
        } else {
            else_br = parseBlockExpr();
        }
    }

    auto* ifExpr = arena_.make<IfExpr>();
    ifExpr->kind = Expr::Kind::If;
    ifExpr->loc = loc;
    ifExpr->condition = cond;
    ifExpr->then_branch = then_br;
    ifExpr->else_branch = else_br;
    return ifExpr;
}

bool Parser::isDerefTarget(const Expr* expr) {
    if (!expr) return false;
    // *ptr = val
    if (expr->kind == Expr::Kind::UnaryOp) {
        auto* u = static_cast<const UnaryOpExpr*>(expr);
        return u->op == UnaryOpKind_t::Deref;
    }
    // (*ptr).field = val — FieldAccess chain rooted in a deref
    if (expr->kind == Expr::Kind::FieldAccess) {
        auto* fa = static_cast<const FieldAccessExpr*>(expr);
        return isDerefTarget(fa->object);
    }
    return false;
}

bool Parser::isCompoundAssign() {
    TokenKind k = peek().kind;
    return k == TokenKind::PlusEq || k == TokenKind::MinusEq ||
           k == TokenKind::StarEq || k == TokenKind::SlashEq ||
           k == TokenKind::PercentEq || k == TokenKind::PipeEq ||
           k == TokenKind::AmpEq || k == TokenKind::CaretEq ||
           k == TokenKind::ShlEq || k == TokenKind::ShrEq;
}

BinOpKind Parser::compoundAssignOp() {
    TokenKind k = advance().kind; // consume compound assign token
    switch (k) {
        case TokenKind::PlusEq:    return BinOpKind::Add;
        case TokenKind::MinusEq:   return BinOpKind::Sub;
        case TokenKind::StarEq:    return BinOpKind::Mul;
        case TokenKind::SlashEq:   return BinOpKind::Div;
        case TokenKind::PercentEq: return BinOpKind::Mod;
        case TokenKind::PipeEq:    return BinOpKind::BitOr;
        case TokenKind::AmpEq:     return BinOpKind::BitAnd;
        case TokenKind::CaretEq:   return BinOpKind::BitXor;
        case TokenKind::ShlEq:     return BinOpKind::Shl;
        case TokenKind::ShrEq:     return BinOpKind::Shr;
        default:                   return BinOpKind::Add; // unreachable
    }
}

Expr* Parser::wrapCompoundAssign(Expr* lhs, Expr* rhs, BinOpKind op, SourceLocation loc) {
    auto* binop = arena_.make<BinOpExpr>();
    binop->kind = Expr::Kind::BinOp;
    binop->loc = loc;
    binop->op = op;
    binop->lhs = lhs;
    binop->rhs = rhs;
    return binop;
}

Expr* Parser::parseBlockExpr() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::LBrace, "expected '{'");
    skipNewlines();

    std::vector<Stmt*> stmts;
    Expr* result = nullptr;

    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        // Try to parse val/var declaration
        if (check(TokenKind::KwVal) || check(TokenKind::KwVar)) {
            stmts.push_back(parseValDecl());
        } else if (check(TokenKind::KwBreak)) {
            Token brk_tok = advance(); // consume 'break'
            auto* bs = arena_.make<BreakStmt>();
            bs->kind = Stmt::Kind::Break;
            bs->loc = brk_tok.loc;
            // Check for label: break 'label
            if (check(TokenKind::Label)) {
                bs->label = advance().text;
            }
            // Check if there's a value to break with
            if (!check(TokenKind::RBrace) && !check(TokenKind::Newline) &&
                !check(TokenKind::Semicolon) && !check(TokenKind::Eof)) {
                bs->value = parseExpr();
            } else {
                bs->value = nullptr;
            }
            stmts.push_back(bs);
        } else if (check(TokenKind::KwContinue)) {
            Token cont_tok = advance(); // consume 'continue'
            auto* cs = arena_.make<ContinueStmt>();
            cs->kind = Stmt::Kind::Continue;
            cs->loc = cont_tok.loc;
            // Check for label: continue 'label
            if (check(TokenKind::Label)) {
                cs->label = advance().text;
            }
            // Parse accumulator update args: continue(expr1, expr2, ...)
            if (match(TokenKind::LParen)) {
                std::vector<Expr*> args;
                if (!check(TokenKind::RParen)) {
                    args.push_back(parseExpr());
                    while (match(TokenKind::Comma)) {
                        skipNewlines();
                        args.push_back(parseExpr());
                    }
                }
                expect(TokenKind::RParen, "expected ')' after continue args");
                cs->arg_count = static_cast<uint32_t>(args.size());
                cs->args = arena_.makeArray<Expr*>(args.size());
                for (size_t i = 0; i < args.size(); ++i) {
                    cs->args[i] = args[i];
                }
            } else {
                cs->arg_count = 0;
                cs->args = nullptr;
            }
            stmts.push_back(cs);
        } else if (check(TokenKind::KwDefer)) {
            Token defer_tok = advance(); // consume 'defer'
            auto* ds = arena_.make<DeferStmt>();
            ds->kind = Stmt::Kind::Defer;
            ds->loc = defer_tok.loc;
            ds->body = parseExpr();
            stmts.push_back(ds);
        } else if (check(TokenKind::Ident)) {
            // Could be assignment, field assignment, or an expression
            Token identTok = peek();
            advance();
            skipNewlines();

            // Build the initial LHS
            Expr* lhs;
            if (check(TokenKind::LParen)) {
                lhs = parseCallExpr(identTok.text, identTok.loc);
            } else if (check(TokenKind::LBrace) && struct_names_.count(identTok.text)) {
                lhs = parseStructLit(identTok.text, identTok.loc);
            } else if (check(TokenKind::Lt) && !struct_names_.count(identTok.text) && !union_names_.count(identTok.text)) {
                // Speculative generic function call: f<T1, T2>(args)
                auto lexer_snap = lexer_.save();
                Token saved_current2 = current_;
                Token saved_previous2 = previous_;
                bool saved_has2 = has_current_;

                advance(); // consume '<'
                std::vector<TypeRef> type_args;
                bool valid = true;
                while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
                    if (!check(TokenKind::Ident) && !check(TokenKind::LBracket) &&
                        !check(TokenKind::KwFn) && !check(TokenKind::Exclaim) &&
                        !check(TokenKind::IntLit)) {
                        valid = false;
                        break;
                    }
                    type_args.push_back(parseType());
                    if (!check(TokenKind::Gt)) {
                        if (!match(TokenKind::Comma)) { valid = false; break; }
                    }
                }
                if (valid && !type_args.empty() && check(TokenKind::Gt)) {
                    advance(); // consume '>'
                    if (check(TokenKind::LParen)) {
                        // It's a generic function call!
                        advance(); // consume '('
                        skipNewlines();
                        std::vector<Expr*> args;
                        if (!check(TokenKind::RParen)) {
                            args.push_back(parseExpr());
                            while (match(TokenKind::Comma)) {
                                skipNewlines();
                                args.push_back(parseExpr());
                            }
                        }
                        skipNewlines();
                        expect(TokenKind::RParen, "expected ')'");

                        auto* call = arena_.make<CallExpr>();
                        call->kind = Expr::Kind::Call;
                        call->loc = identTok.loc;
                        call->callee = identTok.text;
                        call->arg_count = static_cast<uint32_t>(args.size());
                        call->args = arena_.makeArray<Expr*>(args.size());
                        for (size_t i = 0; i < args.size(); ++i) {
                            call->args[i] = args[i];
                        }
                        call->type_arg_count = static_cast<uint32_t>(type_args.size());
                        call->type_args = arena_.makeArray<TypeRef>(type_args.size());
                        for (size_t i = 0; i < type_args.size(); ++i) {
                            call->type_args[i] = type_args[i];
                        }
                        lhs = call;
                        goto block_ident_done;
                    }
                }
                // Not a generic call — restore and fall through to plain ident
                lexer_.restore(lexer_snap);
                current_ = saved_current2;
                previous_ = saved_previous2;
                has_current_ = saved_has2;

                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            } else if (check(TokenKind::Lt) && (struct_names_.count(identTok.text) || union_names_.count(identTok.text))) {
                // Generic struct literal or union variant in block context
                auto lexer_snap = lexer_.save();
                Token saved_current2 = current_;
                Token saved_previous2 = previous_;
                bool saved_has2 = has_current_;

                advance(); // consume '<'
                std::vector<TypeRef> ta;
                bool va = true;
                while (!check(TokenKind::Gt) && !check(TokenKind::Eof)) {
                    if (!check(TokenKind::Ident) && !check(TokenKind::LBracket) &&
                        !check(TokenKind::KwFn) && !check(TokenKind::Exclaim) &&
                        !check(TokenKind::IntLit)) {
                        va = false; break;
                    }
                    ta.push_back(parseType());
                    if (!check(TokenKind::Gt)) {
                        if (!match(TokenKind::Comma)) { va = false; break; }
                    }
                }
                if (va && check(TokenKind::Gt)) {
                    advance(); // consume '>'
                    skipNewlines();

                    std::string mangled = std::string(identTok.text);
                    for (auto& t : ta) {
                        mangled += "_";
                        if (t.kind == TypeRef::Kind::ConstVal)
                            mangled += std::to_string(t.const_value);
                        else
                            mangled += t.name;
                    }
                    char* buf = arena_.makeArray<char>(mangled.size());
                    std::memcpy(buf, mangled.data(), mangled.size());
                    std::string_view mangled_sv(buf, mangled.size());

                    if (check(TokenKind::LBrace)) {
                        lhs = parseStructLit(mangled_sv, identTok.loc);
                        goto block_ident_done;
                    }
                    if (check(TokenKind::ColonColon)) {
                        advance();
                        Token variant_tok = expect(TokenKind::Ident, "expected variant name after '::'");
                        auto* uv = arena_.make<UnionVariantExpr>();
                        uv->kind = Expr::Kind::UnionVariant;
                        uv->loc = identTok.loc;
                        uv->union_name = mangled_sv;
                        uv->variant_name = variant_tok.text;
                        uv->payload = nullptr;
                        if (match(TokenKind::LParen)) {
                            if (!check(TokenKind::RParen)) uv->payload = parseExpr();
                            expect(TokenKind::RParen, "expected ')' after variant payload");
                        }
                        lhs = uv;
                        goto block_ident_done;
                    }
                }
                // Restore — not a generic construct
                lexer_.restore(lexer_snap);
                current_ = saved_current2;
                previous_ = saved_previous2;
                has_current_ = saved_has2;

                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            } else if (check(TokenKind::ColonColon) && union_names_.count(identTok.text)) {
                advance(); // consume ::
                Token variant_tok = expect(TokenKind::Ident, "expected variant name after '::'");
                auto* uv = arena_.make<UnionVariantExpr>();
                uv->kind = Expr::Kind::UnionVariant;
                uv->loc = identTok.loc;
                uv->union_name = identTok.text;
                uv->variant_name = variant_tok.text;
                uv->payload = nullptr;
                if (check(TokenKind::LBrace) && struct_names_.count(variant_tok.text)) {
                    uv->payload = parseStructLit(variant_tok.text, variant_tok.loc);
                } else if (match(TokenKind::LParen)) {
                    if (!check(TokenKind::RParen)) {
                        uv->payload = parseExpr();
                    }
                    expect(TokenKind::RParen, "expected ')' after variant payload");
                }
                lhs = uv;
            } else if (check(TokenKind::Dot) && enum_names_.count(identTok.text)) {
                advance(); // consume .
                Token variant_tok = expect(TokenKind::Ident, "expected variant name after '.'");
                auto* ea = arena_.make<EnumAccessExpr>();
                ea->kind = Expr::Kind::EnumAccess;
                ea->loc = identTok.loc;
                ea->enum_name = identTok.text;
                ea->variant_name = variant_tok.text;
                lhs = ea;
            } else {
                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            }

            block_ident_done:
            // Parse dot chains and index access (highest precedence postfix)
            while (check(TokenKind::Dot) || check(TokenKind::LBracket)) {
                if (check(TokenKind::Dot)) {
                    Token dotTok = advance();
                    Token field = check(TokenKind::IntLit) ? advance() : expect(TokenKind::Ident, "expected field name after '.'");
                    if (check(TokenKind::LParen)) {
                        advance(); // consume '('
                        std::vector<Expr*> mc_args;
                        if (!check(TokenKind::RParen)) {
                            mc_args.push_back(parseExpr());
                            while (match(TokenKind::Comma)) {
                                mc_args.push_back(parseExpr());
                            }
                        }
                        expect(TokenKind::RParen, "expected ')' after method arguments");
                        auto* mc = arena_.make<MethodCallExpr>();
                        mc->kind = Expr::Kind::MethodCall;
                        mc->loc = dotTok.loc;
                        mc->object = lhs;
                        mc->method_name = field.text;
                        mc->arg_count = static_cast<uint32_t>(mc_args.size());
                        mc->args = arena_.makeArray<Expr*>(mc_args.size());
                        for (size_t ai = 0; ai < mc_args.size(); ++ai) {
                            mc->args[ai] = mc_args[ai];
                        }
                        lhs = mc;
                    } else {
                        auto* fa = arena_.make<FieldAccessExpr>();
                        fa->kind = Expr::Kind::FieldAccess;
                        fa->loc = dotTok.loc;
                        fa->object = lhs;
                        fa->field_name = field.text;
                        lhs = fa;
                    }
                } else {
                    advance(); // consume '['
                    skipNewlines();
                    Expr* idx = parseExpr();
                    skipNewlines();
                    expect(TokenKind::RBracket, "expected ']' after index");
                    auto* ia = arena_.make<IndexAccessExpr>();
                    ia->kind = Expr::Kind::IndexAccess;
                    ia->loc = lhs->loc;
                    ia->array = lhs;
                    ia->index = idx;
                    lhs = ia;
                }
            }
            skipNewlines();

            if (check(TokenKind::Eq) || isCompoundAssign()) {
                BinOpKind compound_op = BinOpKind::Add; // unused default
                bool is_compound = isCompoundAssign();
                if (is_compound) {
                    compound_op = compoundAssignOp(); // consumes token
                } else {
                    advance(); // consume '='
                }
                skipNewlines();
                Expr* rhs = parseExpr();
                Expr* value = is_compound
                    ? wrapCompoundAssign(lhs, rhs, compound_op, identTok.loc)
                    : rhs;
                if (lhs->kind == Expr::Kind::IndexAccess) {
                    auto* ia_expr = static_cast<IndexAccessExpr*>(lhs);
                    auto* ias = arena_.make<IndexAssignStmt>();
                    ias->kind = Stmt::Kind::IndexAssign;
                    ias->loc = identTok.loc;
                    ias->array = ia_expr->array;
                    ias->index = ia_expr->index;
                    ias->value = value;
                    stmts.push_back(ias);
                } else if (lhs->kind == Expr::Kind::FieldAccess) {
                    auto* fa_stmt = arena_.make<FieldAssignStmt>();
                    fa_stmt->kind = Stmt::Kind::FieldAssign;
                    fa_stmt->loc = identTok.loc;
                    fa_stmt->target = lhs;
                    fa_stmt->value = value;
                    stmts.push_back(fa_stmt);
                } else {
                    // Simple variable assignment
                    auto* assign = arena_.make<AssignStmt>();
                    assign->kind = Stmt::Kind::Assign;
                    assign->loc = identTok.loc;
                    assign->name = identTok.text;
                    assign->value = value;
                    stmts.push_back(assign);
                }
            } else {
                // Continue with infix parsing (reuse Pratt parser)
                lhs = parseExprInfix(lhs, 0);
                skipNewlines();

                if (check(TokenKind::RBrace)) {
                    result = lhs;
                } else {
                    auto* exprStmt = arena_.make<ExprStmt>();
                    exprStmt->kind = Stmt::Kind::ExprStmt;
                    exprStmt->loc = lhs->loc;
                    exprStmt->expr = lhs;
                    stmts.push_back(exprStmt);
                }
            }
        } else {
            // Parse expression
            Expr* expr = parseExpr();
            skipNewlines();

            // Check for index assignment: arr[idx] = val or arr[idx] += val
            if ((check(TokenKind::Eq) || isCompoundAssign()) && expr->kind == Expr::Kind::IndexAccess) {
                bool is_compound = isCompoundAssign();
                BinOpKind compound_op = BinOpKind::Add;
                if (is_compound) {
                    compound_op = compoundAssignOp();
                } else {
                    advance(); // consume '='
                }
                skipNewlines();
                Expr* rhs = parseExpr();
                Expr* value = is_compound
                    ? wrapCompoundAssign(expr, rhs, compound_op, expr->loc)
                    : rhs;
                auto* ia_expr = static_cast<IndexAccessExpr*>(expr);
                auto* ias = arena_.make<IndexAssignStmt>();
                ias->kind = Stmt::Kind::IndexAssign;
                ias->loc = expr->loc;
                ias->array = ia_expr->array;
                ias->index = ia_expr->index;
                ias->value = value;
                stmts.push_back(ias);
            }
            // Check for deref assignment: *ptr = val or *ptr += val
            else if ((check(TokenKind::Eq) || isCompoundAssign()) && isDerefTarget(expr)) {
                bool is_compound = isCompoundAssign();
                BinOpKind compound_op = BinOpKind::Add;
                if (is_compound) {
                    compound_op = compoundAssignOp();
                } else {
                    advance(); // consume '='
                }
                skipNewlines();
                Expr* rhs = parseExpr();
                Expr* value = is_compound
                    ? wrapCompoundAssign(expr, rhs, compound_op, expr->loc)
                    : rhs;
                auto* da = arena_.make<DerefAssignStmt>();
                da->kind = Stmt::Kind::DerefAssign;
                da->loc = expr->loc;
                da->target = expr;
                da->value = value;
                stmts.push_back(da);
            } else if (check(TokenKind::RBrace)) {
                // If the next token is '}', this is the result expression
                result = expr;
            } else {
                // It's a statement
                auto* exprStmt = arena_.make<ExprStmt>();
                exprStmt->kind = Stmt::Kind::ExprStmt;
                exprStmt->loc = expr->loc;
                exprStmt->expr = expr;
                stmts.push_back(exprStmt);
            }
        }
        // Skip semicolons and newlines between statements
        while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }

    expect(TokenKind::RBrace, "expected '}'");

    auto* block = arena_.make<BlockExpr>();
    block->kind = Expr::Kind::Block;
    block->loc = loc;
    block->stmt_count = static_cast<uint32_t>(stmts.size());
    block->stmts = arena_.makeArray<Stmt*>(stmts.size());
    for (size_t i = 0; i < stmts.size(); ++i) {
        block->stmts[i] = stmts[i];
    }
    block->result = result;
    return block;
}

Expr* Parser::parseCallExpr(std::string_view name, SourceLocation loc) {
    expect(TokenKind::LParen, "expected '('");
    skipNewlines();

    std::vector<Expr*> args;
    if (!check(TokenKind::RParen)) {
        args.push_back(parseExpr());
        while (match(TokenKind::Comma)) {
            skipNewlines();
            args.push_back(parseExpr());
        }
    }
    skipNewlines();
    expect(TokenKind::RParen, "expected ')'");

    auto* call = arena_.make<CallExpr>();
    call->kind = Expr::Kind::Call;
    call->loc = loc;
    call->callee = name;
    call->arg_count = static_cast<uint32_t>(args.size());
    call->args = arena_.makeArray<Expr*>(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        call->args[i] = args[i];
    }
    return call;
}

// --- Struct Literal ---
Expr* Parser::parseStructLit(std::string_view name, SourceLocation loc) {
    expect(TokenKind::LBrace, "expected '{' in struct literal");
    skipNewlines();

    std::vector<FieldInit> fields;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        FieldInit fi;
        Token field_name = expect(TokenKind::Ident, "expected field name");
        fi.name = field_name.text;
        fi.loc = field_name.loc;
        expect(TokenKind::Colon, "expected ':' after field name");
        skipNewlines();
        fi.value = parseExpr();
        fields.push_back(fi);
        // Skip comma, semicolons, newlines between fields
        while (match(TokenKind::Comma) || match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' in struct literal");

    auto* sl = arena_.make<StructLitExpr>();
    sl->kind = Expr::Kind::StructLit;
    sl->loc = loc;
    sl->struct_name = name;
    sl->field_count = static_cast<uint32_t>(fields.size());
    sl->fields = arena_.makeArray<FieldInit>(fields.size());
    for (size_t i = 0; i < fields.size(); ++i) {
        sl->fields[i] = fields[i];
    }
    return sl;
}

// --- Match Expression ---
Expr* Parser::parseMatchExpr() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwMatch, "expected 'match'");

    Expr* scrutinee = parseExpr();
    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' after match scrutinee");
    skipNewlines();

    std::vector<MatchArm> arms;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        MatchArm arm;
        arm.loc = peek().loc;
        arm.pattern = parsePattern();
        arm.guard = nullptr;

        // Optional guard: `if <expr>`
        if (check(TokenKind::KwIf)) {
            advance();
            arm.guard = parseExpr();
        }

        expect(TokenKind::FatArrow, "expected '=>' after pattern");
        skipNewlines();
        arm.body = parseExpr();
        arms.push_back(arm);

        // Skip comma, semicolons, newlines between arms
        while (match(TokenKind::Comma) || match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after match arms");

    auto* matchExpr = arena_.make<MatchExpr>();
    matchExpr->kind = Expr::Kind::Match;
    matchExpr->loc = loc;
    matchExpr->scrutinee = scrutinee;
    matchExpr->arm_count = static_cast<uint32_t>(arms.size());
    matchExpr->arms = arena_.makeArray<MatchArm>(arms.size());
    for (size_t i = 0; i < arms.size(); ++i) {
        matchExpr->arms[i] = arms[i];
    }
    return matchExpr;
}

// loop(acc = init, ...) { body }
Expr* Parser::parseLoopExpr() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwLoop, "expected 'loop'");

    // Parse bindings: loop(acc = 0, count = 10) { ... }
    std::vector<LoopBinding> bindings;
    if (match(TokenKind::LParen)) {
        if (!check(TokenKind::RParen)) {
            do {
                skipNewlines();
                LoopBinding b;
                Token name_tok = expect(TokenKind::Ident, "expected binding name");
                b.name = name_tok.text;
                expect(TokenKind::Eq, "expected '=' after binding name");
                skipNewlines();
                b.init = parseExpr();
                bindings.push_back(b);
            } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RParen, "expected ')' after loop bindings");
    }
    skipNewlines();

    // Parse body as a block: { stmts... }
    // We parse the block body inline (not as a full BlockExpr) to get stmts + result
    expect(TokenKind::LBrace, "expected '{' for loop body");
    skipNewlines();

    std::vector<Stmt*> stmts;
    Expr* result = nullptr;

    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        // val/var declarations
        if (check(TokenKind::KwVal) || check(TokenKind::KwVar)) {
            stmts.push_back(parseValDecl());
        } else if (check(TokenKind::KwBreak)) {
            Token brk_tok = advance();
            auto* bs = arena_.make<BreakStmt>();
            bs->kind = Stmt::Kind::Break;
            bs->loc = brk_tok.loc;
            if (check(TokenKind::Label)) {
                bs->label = advance().text;
            }
            if (!check(TokenKind::RBrace) && !check(TokenKind::Newline) &&
                !check(TokenKind::Semicolon) && !check(TokenKind::Eof)) {
                bs->value = parseExpr();
            } else {
                bs->value = nullptr;
            }
            stmts.push_back(bs);
        } else if (check(TokenKind::KwContinue)) {
            Token cont_tok = advance();
            auto* cs = arena_.make<ContinueStmt>();
            cs->kind = Stmt::Kind::Continue;
            cs->loc = cont_tok.loc;
            if (check(TokenKind::Label)) {
                cs->label = advance().text;
            }
            if (match(TokenKind::LParen)) {
                std::vector<Expr*> args;
                if (!check(TokenKind::RParen)) {
                    args.push_back(parseExpr());
                    while (match(TokenKind::Comma)) {
                        skipNewlines();
                        args.push_back(parseExpr());
                    }
                }
                expect(TokenKind::RParen, "expected ')' after continue args");
                cs->arg_count = static_cast<uint32_t>(args.size());
                cs->args = arena_.makeArray<Expr*>(args.size());
                for (size_t i = 0; i < args.size(); ++i) {
                    cs->args[i] = args[i];
                }
            } else {
                cs->arg_count = 0;
                cs->args = nullptr;
            }
            stmts.push_back(cs);
        } else if (check(TokenKind::Ident)) {
            // Could be assignment or expression
            Token identTok = peek();
            advance();
            skipNewlines();

            Expr* lhs;
            if (check(TokenKind::LParen)) {
                lhs = parseCallExpr(identTok.text, identTok.loc);
            } else {
                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            }

            // Parse dot chains
            while (check(TokenKind::Dot)) {
                Token dotTok = advance();
                Token field = check(TokenKind::IntLit) ? advance() : expect(TokenKind::Ident, "expected field name after '.'");
                if (check(TokenKind::LParen)) {
                    advance(); // consume '('
                    std::vector<Expr*> mc_args;
                    if (!check(TokenKind::RParen)) {
                        mc_args.push_back(parseExpr());
                        while (match(TokenKind::Comma)) {
                            mc_args.push_back(parseExpr());
                        }
                    }
                    expect(TokenKind::RParen, "expected ')' after method arguments");
                    auto* mc = arena_.make<MethodCallExpr>();
                    mc->kind = Expr::Kind::MethodCall;
                    mc->loc = dotTok.loc;
                    mc->object = lhs;
                    mc->method_name = field.text;
                    mc->arg_count = static_cast<uint32_t>(mc_args.size());
                    mc->args = arena_.makeArray<Expr*>(mc_args.size());
                    for (size_t ai = 0; ai < mc_args.size(); ++ai) {
                        mc->args[ai] = mc_args[ai];
                    }
                    lhs = mc;
                } else {
                    auto* fa = arena_.make<FieldAccessExpr>();
                    fa->kind = Expr::Kind::FieldAccess;
                    fa->loc = dotTok.loc;
                    fa->object = lhs;
                    if (field.kind == TokenKind::IntLit) {
                        std::string fn = "_" + std::string(field.text);
                        char* b = arena_.makeArray<char>(fn.size());
                        std::memcpy(b, fn.data(), fn.size());
                        fa->field_name = std::string_view(b, fn.size());
                    } else {
                        fa->field_name = field.text;
                    }
                    lhs = fa;
                }
            }
            skipNewlines();

            if (check(TokenKind::Eq) || isCompoundAssign()) {
                bool is_compound = isCompoundAssign();
                BinOpKind compound_op = BinOpKind::Add;
                if (is_compound) {
                    compound_op = compoundAssignOp();
                } else {
                    advance();
                }
                skipNewlines();
                Expr* rhs = parseExpr();
                Expr* value = is_compound
                    ? wrapCompoundAssign(lhs, rhs, compound_op, identTok.loc)
                    : rhs;
                if (lhs->kind == Expr::Kind::FieldAccess) {
                    auto* fa_stmt = arena_.make<FieldAssignStmt>();
                    fa_stmt->kind = Stmt::Kind::FieldAssign;
                    fa_stmt->loc = identTok.loc;
                    fa_stmt->target = lhs;
                    fa_stmt->value = value;
                    stmts.push_back(fa_stmt);
                } else {
                    auto* assign = arena_.make<AssignStmt>();
                    assign->kind = Stmt::Kind::Assign;
                    assign->loc = identTok.loc;
                    assign->name = identTok.text;
                    assign->value = value;
                    stmts.push_back(assign);
                }
            } else {
                lhs = parseExprInfix(lhs, 0);
                skipNewlines();
                if (check(TokenKind::RBrace)) {
                    result = lhs;
                } else {
                    auto* exprStmt = arena_.make<ExprStmt>();
                    exprStmt->kind = Stmt::Kind::ExprStmt;
                    exprStmt->loc = lhs->loc;
                    exprStmt->expr = lhs;
                    stmts.push_back(exprStmt);
                }
            }
        } else {
            Expr* expr = parseExpr();
            skipNewlines();
            if (check(TokenKind::RBrace)) {
                result = expr;
            } else {
                auto* exprStmt = arena_.make<ExprStmt>();
                exprStmt->kind = Stmt::Kind::ExprStmt;
                exprStmt->loc = expr->loc;
                exprStmt->expr = expr;
                stmts.push_back(exprStmt);
            }
        }
        while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after loop body");

    auto* loop = arena_.make<LoopExpr>();
    loop->kind = Expr::Kind::Loop;
    loop->loc = loc;
    loop->binding_count = static_cast<uint32_t>(bindings.size());
    loop->bindings = arena_.makeArray<LoopBinding>(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        loop->bindings[i] = bindings[i];
    }
    loop->stmt_count = static_cast<uint32_t>(stmts.size());
    loop->stmts = arena_.makeArray<Stmt*>(stmts.size());
    for (size_t i = 0; i < stmts.size(); ++i) {
        loop->stmts[i] = stmts[i];
    }
    loop->result = result;
    return loop;
}

// for i in start..end { stmts }
Expr* Parser::parseForRange() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwFor, "expected 'for'");

    Token var_tok = expect(TokenKind::Ident, "expected loop variable name");
    expect(TokenKind::KwIn, "expected 'in' after loop variable");
    skipNewlines();

    Expr* start = parseExpr();

    // Detect for-each (no ..) vs for-range (..)
    if (!check(TokenKind::DotDot)) {
        // For-each: for item in collection { body }
        // Will be desugared into for-range in HIRBuilder
        skipNewlines();
        expect(TokenKind::LBrace, "expected '{' for for-each body");
        skipNewlines();

        std::vector<Stmt*> fe_stmts;
        while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
            if (check(TokenKind::KwVal) || check(TokenKind::KwVar)) {
                fe_stmts.push_back(parseValDecl());
            } else if (check(TokenKind::KwBreak)) {
                Token brk_tok = advance();
                auto* bs = arena_.make<BreakStmt>();
                bs->kind = Stmt::Kind::Break;
                bs->loc = brk_tok.loc;
                if (check(TokenKind::Label)) {
                    bs->label = advance().text;
                }
                bs->value = nullptr;
                fe_stmts.push_back(bs);
            } else if (check(TokenKind::KwContinue)) {
                Token cnt_tok = advance();
                auto* cs = arena_.make<ContinueStmt>();
                cs->kind = Stmt::Kind::Continue;
                cs->loc = cnt_tok.loc;
                if (check(TokenKind::Label)) {
                    cs->label = advance().text;
                }
                fe_stmts.push_back(cs);
            } else if (check(TokenKind::Ident)) {
                Token identTok = peek();
                advance();
                skipNewlines();
                Expr* lhs;
                if (check(TokenKind::LParen)) {
                    lhs = parseCallExpr(identTok.text, identTok.loc);
                } else {
                    auto* ident = arena_.make<IdentExpr>();
                    ident->kind = Expr::Kind::Ident;
                    ident->loc = identTok.loc;
                    ident->name = identTok.text;
                    lhs = ident;
                }
                while (check(TokenKind::Dot)) {
                    advance();
                    Token field = check(TokenKind::IntLit) ? advance() : expect(TokenKind::Ident, "expected field name after '.'");
                    auto* fa = arena_.make<FieldAccessExpr>();
                    fa->kind = Expr::Kind::FieldAccess;
                    fa->loc = field.loc;
                    fa->object = lhs;
                    if (field.kind == TokenKind::IntLit) {
                        std::string fn = "_" + std::string(field.text);
                        char* b = arena_.makeArray<char>(fn.size());
                        std::memcpy(b, fn.data(), fn.size());
                        fa->field_name = std::string_view(b, fn.size());
                    } else {
                        fa->field_name = field.text;
                    }
                    lhs = fa;
                }
                while (check(TokenKind::LBracket)) {
                    advance();
                    Expr* idx = parseExpr();
                    expect(TokenKind::RBracket, "expected ']'");
                    auto* ia = arena_.make<IndexAccessExpr>();
                    ia->kind = Expr::Kind::IndexAccess;
                    ia->loc = identTok.loc;
                    ia->array = lhs;
                    ia->index = idx;
                    lhs = ia;
                }
                skipNewlines();
                if (check(TokenKind::Eq) || isCompoundAssign()) {
                    bool is_compound = isCompoundAssign();
                    BinOpKind compound_op = BinOpKind::Add;
                    if (is_compound) {
                        compound_op = compoundAssignOp();
                    } else {
                        advance();
                    }
                    skipNewlines();
                    Expr* rhs = parseExpr();
                    Expr* value = is_compound
                        ? wrapCompoundAssign(lhs, rhs, compound_op, identTok.loc)
                        : rhs;
                    if (lhs->kind == Expr::Kind::FieldAccess || lhs->kind == Expr::Kind::IndexAccess) {
                        auto* fa_stmt = arena_.make<FieldAssignStmt>();
                        fa_stmt->kind = Stmt::Kind::FieldAssign;
                        fa_stmt->loc = identTok.loc;
                        fa_stmt->target = lhs;
                        fa_stmt->value = value;
                        fe_stmts.push_back(fa_stmt);
                    } else {
                        auto* assign = arena_.make<AssignStmt>();
                        assign->kind = Stmt::Kind::Assign;
                        assign->loc = identTok.loc;
                        assign->name = identTok.text;
                        assign->value = value;
                        fe_stmts.push_back(assign);
                    }
                } else {
                    lhs = parseExprInfix(lhs, 0);
                    auto* exprStmt = arena_.make<ExprStmt>();
                    exprStmt->kind = Stmt::Kind::ExprStmt;
                    exprStmt->loc = lhs->loc;
                    exprStmt->expr = lhs;
                    fe_stmts.push_back(exprStmt);
                }
            } else {
                Expr* e = parseExpr();
                auto* es = arena_.make<ExprStmt>();
                es->kind = Stmt::Kind::ExprStmt;
                es->loc = e->loc;
                es->expr = e;
                fe_stmts.push_back(es);
            }
            while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
        }
        expect(TokenKind::RBrace, "expected '}' to close for-each body");

        auto* fe = arena_.make<ForEachExpr>();
        fe->kind = Expr::Kind::ForEach;
        fe->loc = loc;
        fe->var_name = var_tok.text;
        fe->collection = start;
        fe->stmt_count = static_cast<uint32_t>(fe_stmts.size());
        fe->stmts = arena_.makeArray<Stmt*>(fe_stmts.size());
        for (size_t i = 0; i < fe_stmts.size(); ++i) fe->stmts[i] = fe_stmts[i];
        return fe;
    }

    advance(); // consume '..'
    skipNewlines();

    Expr* end = parseExpr();
    skipNewlines();

    // Parse body block
    expect(TokenKind::LBrace, "expected '{' for for-range body");
    skipNewlines();

    std::vector<Stmt*> stmts;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        if (check(TokenKind::KwVal) || check(TokenKind::KwVar)) {
            stmts.push_back(parseValDecl());
        } else if (check(TokenKind::Ident)) {
            Token identTok = peek();
            advance();
            skipNewlines();

            Expr* lhs;
            if (check(TokenKind::LParen)) {
                lhs = parseCallExpr(identTok.text, identTok.loc);
            } else {
                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            }

            // Field access chains
            while (check(TokenKind::Dot)) {
                advance();
                Token field = check(TokenKind::IntLit) ? advance() : expect(TokenKind::Ident, "expected field name after '.'");
                auto* fa = arena_.make<FieldAccessExpr>();
                fa->kind = Expr::Kind::FieldAccess;
                fa->loc = field.loc;
                fa->object = lhs;
                if (field.kind == TokenKind::IntLit) {
                    std::string fn = "_" + std::string(field.text);
                    char* b = arena_.makeArray<char>(fn.size());
                    std::memcpy(b, fn.data(), fn.size());
                    fa->field_name = std::string_view(b, fn.size());
                } else {
                    fa->field_name = field.text;
                }
                lhs = fa;
            }

            // Index access
            while (check(TokenKind::LBracket)) {
                advance();
                Expr* idx = parseExpr();
                expect(TokenKind::RBracket, "expected ']'");
                auto* ia = arena_.make<IndexAccessExpr>();
                ia->kind = Expr::Kind::IndexAccess;
                ia->loc = identTok.loc;
                ia->array = lhs;
                ia->index = idx;
                lhs = ia;
            }
            skipNewlines();

            if (check(TokenKind::Eq) || isCompoundAssign()) {
                bool is_compound = isCompoundAssign();
                BinOpKind compound_op = BinOpKind::Add;
                if (is_compound) {
                    compound_op = compoundAssignOp();
                } else {
                    advance();
                }
                skipNewlines();
                Expr* rhs = parseExpr();
                Expr* value = is_compound
                    ? wrapCompoundAssign(lhs, rhs, compound_op, identTok.loc)
                    : rhs;
                if (lhs->kind == Expr::Kind::FieldAccess) {
                    auto* fa_stmt = arena_.make<FieldAssignStmt>();
                    fa_stmt->kind = Stmt::Kind::FieldAssign;
                    fa_stmt->loc = identTok.loc;
                    fa_stmt->target = lhs;
                    fa_stmt->value = value;
                    stmts.push_back(fa_stmt);
                } else if (lhs->kind == Expr::Kind::IndexAccess) {
                    auto* ia_stmt = arena_.make<FieldAssignStmt>();
                    ia_stmt->kind = Stmt::Kind::FieldAssign;
                    ia_stmt->loc = identTok.loc;
                    ia_stmt->target = lhs;
                    ia_stmt->value = value;
                    stmts.push_back(ia_stmt);
                } else {
                    auto* assign = arena_.make<AssignStmt>();
                    assign->kind = Stmt::Kind::Assign;
                    assign->loc = identTok.loc;
                    assign->name = identTok.text;
                    assign->value = value;
                    stmts.push_back(assign);
                }
            } else {
                lhs = parseExprInfix(lhs, 0);
                auto* exprStmt = arena_.make<ExprStmt>();
                exprStmt->kind = Stmt::Kind::ExprStmt;
                exprStmt->loc = lhs->loc;
                exprStmt->expr = lhs;
                stmts.push_back(exprStmt);
            }
        } else {
            Expr* expr = parseExpr();
            auto* es = arena_.make<ExprStmt>();
            es->kind = Stmt::Kind::ExprStmt;
            es->loc = expr->loc;
            es->expr = expr;
            stmts.push_back(es);
        }
        while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after for-range body");

    auto* fr = arena_.make<ForRangeExpr>();
    fr->kind = Expr::Kind::ForRange;
    fr->loc = loc;
    fr->var_name = var_tok.text;
    fr->start = start;
    fr->end = end;
    fr->stmt_count = static_cast<uint32_t>(stmts.size());
    fr->stmts = arena_.makeArray<Stmt*>(stmts.size());
    for (size_t i = 0; i < stmts.size(); ++i) {
        fr->stmts[i] = stmts[i];
    }
    return fr;
}

// while cond { stmts }
Expr* Parser::parseWhileLoop() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwWhile, "expected 'while'");
    skipNewlines();

    Expr* cond = parseExpr();
    skipNewlines();

    // Parse body block (reuses for-range body logic with assignments)
    expect(TokenKind::LBrace, "expected '{' for while body");
    skipNewlines();

    std::vector<Stmt*> stmts;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        if (check(TokenKind::KwVal) || check(TokenKind::KwVar)) {
            stmts.push_back(parseValDecl());
        } else if (check(TokenKind::KwBreak)) {
            Token brk_tok = advance();
            auto* bs = arena_.make<BreakStmt>();
            bs->kind = Stmt::Kind::Break;
            bs->loc = brk_tok.loc;
            if (check(TokenKind::Label)) {
                bs->label = advance().text;
            }
            bs->value = nullptr;
            stmts.push_back(bs);
        } else if (check(TokenKind::KwContinue)) {
            Token cnt_tok = advance();
            auto* cs = arena_.make<ContinueStmt>();
            cs->kind = Stmt::Kind::Continue;
            cs->loc = cnt_tok.loc;
            if (check(TokenKind::Label)) {
                cs->label = advance().text;
            }
            cs->arg_count = 0;
            cs->args = nullptr;
            stmts.push_back(cs);
        } else if (check(TokenKind::Ident)) {
            Token identTok = peek();
            advance();
            skipNewlines();

            Expr* lhs;
            if (check(TokenKind::LParen)) {
                lhs = parseCallExpr(identTok.text, identTok.loc);
            } else {
                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            }

            while (check(TokenKind::Dot)) {
                advance();
                Token field = check(TokenKind::IntLit) ? advance() : expect(TokenKind::Ident, "expected field name after '.'");
                auto* fa = arena_.make<FieldAccessExpr>();
                fa->kind = Expr::Kind::FieldAccess;
                fa->loc = field.loc;
                fa->object = lhs;
                if (field.kind == TokenKind::IntLit) {
                    std::string fn = "_" + std::string(field.text);
                    char* b = arena_.makeArray<char>(fn.size());
                    std::memcpy(b, fn.data(), fn.size());
                    fa->field_name = std::string_view(b, fn.size());
                } else {
                    fa->field_name = field.text;
                }
                lhs = fa;
            }

            while (check(TokenKind::LBracket)) {
                advance();
                Expr* idx = parseExpr();
                expect(TokenKind::RBracket, "expected ']'");
                auto* ia = arena_.make<IndexAccessExpr>();
                ia->kind = Expr::Kind::IndexAccess;
                ia->loc = identTok.loc;
                ia->array = lhs;
                ia->index = idx;
                lhs = ia;
            }
            skipNewlines();

            if (check(TokenKind::Eq) || isCompoundAssign()) {
                bool is_compound = isCompoundAssign();
                BinOpKind compound_op = BinOpKind::Add;
                if (is_compound) {
                    compound_op = compoundAssignOp();
                } else {
                    advance();
                }
                skipNewlines();
                Expr* rhs = parseExpr();
                Expr* value = is_compound
                    ? wrapCompoundAssign(lhs, rhs, compound_op, identTok.loc)
                    : rhs;
                if (lhs->kind == Expr::Kind::FieldAccess || lhs->kind == Expr::Kind::IndexAccess) {
                    auto* fa_stmt = arena_.make<FieldAssignStmt>();
                    fa_stmt->kind = Stmt::Kind::FieldAssign;
                    fa_stmt->loc = identTok.loc;
                    fa_stmt->target = lhs;
                    fa_stmt->value = value;
                    stmts.push_back(fa_stmt);
                } else {
                    auto* assign = arena_.make<AssignStmt>();
                    assign->kind = Stmt::Kind::Assign;
                    assign->loc = identTok.loc;
                    assign->name = identTok.text;
                    assign->value = value;
                    stmts.push_back(assign);
                }
            } else {
                lhs = parseExprInfix(lhs, 0);
                auto* exprStmt = arena_.make<ExprStmt>();
                exprStmt->kind = Stmt::Kind::ExprStmt;
                exprStmt->loc = lhs->loc;
                exprStmt->expr = lhs;
                stmts.push_back(exprStmt);
            }
        } else {
            Expr* expr = parseExpr();
            auto* es = arena_.make<ExprStmt>();
            es->kind = Stmt::Kind::ExprStmt;
            es->loc = expr->loc;
            es->expr = expr;
            stmts.push_back(es);
        }
        while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }
    expect(TokenKind::RBrace, "expected '}' after while body");

    auto* wl = arena_.make<WhileLoopExpr>();
    wl->kind = Expr::Kind::WhileLoop;
    wl->loc = loc;
    wl->condition = cond;
    wl->stmt_count = static_cast<uint32_t>(stmts.size());
    wl->stmts = arena_.makeArray<Stmt*>(stmts.size());
    for (size_t i = 0; i < stmts.size(); ++i) {
        wl->stmts[i] = stmts[i];
    }
    return wl;
}

Pattern* Parser::parsePattern() {
    Token tok = peek();

    // Integer literal pattern (including negative), possibly range
    if (tok.kind == TokenKind::IntLit || tok.kind == TokenKind::Minus) {
        int64_t lo;
        if (tok.kind == TokenKind::Minus) {
            advance();
            Token num = expect(TokenKind::IntLit, "expected integer after '-' in pattern");
            lo = -parseIntText(num.text);
        } else {
            advance();
            lo = parseIntText(tok.text);
        }

        // Check for range pattern: lo..hi or lo..=hi
        if (check(TokenKind::DotDot) || check(TokenKind::DotDotEq)) {
            bool inclusive = (peek().kind == TokenKind::DotDotEq);
            advance(); // consume .. or ..=
            // Parse high bound (possibly negative)
            int64_t hi;
            if (match(TokenKind::Minus)) {
                Token num = expect(TokenKind::IntLit, "expected integer after '-' in range pattern");
                hi = -parseIntText(num.text);
            } else {
                Token num = expect(TokenKind::IntLit, "expected integer in range pattern");
                hi = parseIntText(num.text);
            }
            auto* pat = arena_.make<RangePattern>();
            pat->kind = Pattern::Kind::Range;
            pat->loc = tok.loc;
            pat->lo = lo;
            pat->hi = hi;
            pat->inclusive = inclusive;
            return pat;
        }

        auto* pat = arena_.make<IntLitPattern>();
        pat->kind = Pattern::Kind::IntLit;
        pat->loc = tok.loc;
        pat->value = lo;
        return pat;
    }

    // Bool literal pattern
    if (tok.kind == TokenKind::KwTrue || tok.kind == TokenKind::KwFalse) {
        advance();
        auto* pat = arena_.make<BoolLitPattern>();
        pat->kind = Pattern::Kind::BoolLit;
        pat->loc = tok.loc;
        pat->value = (tok.kind == TokenKind::KwTrue);
        return pat;
    }

    // Wildcard, variable, enum, or union variant pattern
    if (tok.kind == TokenKind::Ident) {
        advance();
        if (tok.text == "_") {
            auto* pat = arena_.make<WildcardPattern>();
            pat->kind = Pattern::Kind::Wildcard;
            pat->loc = tok.loc;
            return pat;
        }
        // Union pattern: Variant(binding) — e.g. Some(x)
        if (check(TokenKind::LParen)) {
            advance(); // consume (
            auto* upat = arena_.make<UnionPattern>();
            upat->kind = Pattern::Kind::Union;
            upat->loc = tok.loc;
            upat->variant_name = tok.text;
            upat->inner = nullptr;
            upat->field_bindings = nullptr;
            upat->field_binding_count = 0;
            if (!check(TokenKind::RParen)) {
                upat->inner = parsePattern();
            }
            expect(TokenKind::RParen, "expected ')' in union pattern");
            return upat;
        }
        // Struct destructuring pattern: Circle { radius: r, ... }
        if (check(TokenKind::LBrace) && struct_names_.count(tok.text)) {
            advance(); // consume {
            skipNewlines();
            std::vector<FieldBinding> bindings;
            while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
                Token fname = expect(TokenKind::Ident, "expected field name in pattern");
                expect(TokenKind::Colon, "expected ':' after field name in pattern");
                Token bname = expect(TokenKind::Ident, "expected binding name in pattern");
                FieldBinding fb;
                fb.field_name = fname.text;
                fb.binding_name = bname.text;
                fb.loc = fname.loc;
                bindings.push_back(fb);
                while (match(TokenKind::Comma) || match(TokenKind::Newline)) {}
            }
            expect(TokenKind::RBrace, "expected '}' in struct pattern");
            auto* upat = arena_.make<UnionPattern>();
            upat->kind = Pattern::Kind::Union;
            upat->loc = tok.loc;
            upat->variant_name = tok.text;
            upat->inner = nullptr;
            upat->field_binding_count = static_cast<uint32_t>(bindings.size());
            upat->field_bindings = arena_.makeArray<FieldBinding>(bindings.size());
            for (size_t i = 0; i < bindings.size(); ++i) {
                upat->field_bindings[i] = bindings[i];
            }
            return upat;
        }
        // Plain identifier — could be enum variant or variable binding.
        // Ambiguous at parse time; TypeChecker resolves based on scrutinee type.
        auto* pat = arena_.make<VariablePattern>();
        pat->kind = Pattern::Kind::Variable;
        pat->loc = tok.loc;
        pat->name = tok.text;
        return pat;
    }

    diag_.error(tok.loc, "expected pattern (integer, bool, identifier, or '_')");
    advance();
    auto* pat = arena_.make<WildcardPattern>();
    pat->kind = Pattern::Kind::Wildcard;
    pat->loc = tok.loc;
    return pat;
}

// --- Statements ---
Stmt* Parser::parseValDecl() {
    Token kw = advance(); // val or var
    bool is_var = (kw.kind == TokenKind::KwVar);

    // Tuple destructuring: val (a, b, c) = expr
    if (check(TokenKind::LParen)) {
        advance(); // consume '('
        std::vector<std::string_view> names;
        while (!check(TokenKind::RParen) && !check(TokenKind::Eof)) {
            Token n = expect(TokenKind::Ident, "expected variable name in tuple destructuring");
            names.push_back(n.text);
            if (!check(TokenKind::RParen)) {
                expect(TokenKind::Comma, "expected ',' or ')' in tuple destructuring");
            }
        }
        expect(TokenKind::RParen, "expected ')' after tuple destructuring names");
        expect(TokenKind::Eq, "expected '=' in tuple destructuring");
        Expr* init = parseExpr();

        auto* stmt = arena_.make<TupleDestructStmt>();
        stmt->kind = Stmt::Kind::TupleDestruct;
        stmt->loc = kw.loc;
        stmt->is_var = is_var;
        stmt->name_count = static_cast<uint32_t>(names.size());
        stmt->names = arena_.makeArray<std::string_view>(names.size());
        for (size_t i = 0; i < names.size(); ++i) {
            stmt->names[i] = names[i];
        }
        stmt->init = init;
        return stmt;
    }

    Token name = expect(TokenKind::Ident, "expected variable name");
    TypeRef type{};
    if (check(TokenKind::Colon)) {
        advance(); // consume ':'
        type = parseType();
    }
    // If no type annotation, type.name will be empty — HIRBuilder infers from init
    expect(TokenKind::Eq, "expected '=' in binding");
    Expr* init = parseExpr();

    if (is_var) {
        auto* decl = arena_.make<VarDeclStmt>();
        decl->kind = Stmt::Kind::VarDecl;
        decl->loc = kw.loc;
        decl->name = name.text;
        decl->type = type;
        decl->init = init;
        return decl;
    } else {
        auto* decl = arena_.make<ValDeclStmt>();
        decl->kind = Stmt::Kind::ValDecl;
        decl->loc = kw.loc;
        decl->name = name.text;
        decl->type = type;
        decl->init = init;
        return decl;
    }
}

// --- Lambda ---

bool Parser::isLambdaStart() {
    // We are looking at '{'. Detect lambda patterns:
    //   { => body }              — zero-param lambda
    //   { x => body }            — untyped single param
    //   { x: T => body }         — typed single param
    //   { x: T, y: U => body }   — multiple typed params
    // Save full parser + lexer state, scan ahead, then restore.

    auto lexer_snap = lexer_.save();
    Token saved_current = current_;
    Token saved_previous = previous_;
    bool saved_has = has_current_;

    // Consume '{'
    advance();
    while (check(TokenKind::Newline)) advance();

    bool is_lambda = false;

    if (check(TokenKind::FatArrow)) {
        is_lambda = true;
    } else if (check(TokenKind::Ident)) {
        advance();
        while (check(TokenKind::Newline)) advance();
        if (check(TokenKind::FatArrow)) {
            is_lambda = true;
        } else if (check(TokenKind::Colon)) {
            // { x: T ... => } pattern — scan forward for '=>' before '}'
            int depth = 0;
            while (!check(TokenKind::Eof)) {
                if (check(TokenKind::FatArrow) && depth == 0) {
                    is_lambda = true;
                    break;
                }
                if (check(TokenKind::RBrace)) break;
                if (check(TokenKind::LBrace)) break; // nested block, not lambda
                if (check(TokenKind::LParen)) depth++;
                if (check(TokenKind::RParen)) depth--;
                advance();
            }
        }
    }

    // Restore state
    lexer_.restore(lexer_snap);
    current_ = saved_current;
    previous_ = saved_previous;
    has_current_ = saved_has;

    return is_lambda;
}

Expr* Parser::parseLambdaExpr(SourceLocation loc) {
    // Already consumed '{' and skipped newlines. Parse: params => body }

    std::vector<Param> params;

    // Zero-param lambda: { => body }
    if (!check(TokenKind::FatArrow)) {
        // Parse params: x: T, y: U, ... =>
        do {
            Param p;
            Token name = expect(TokenKind::Ident, "expected parameter name in lambda");
            p.name = name.text;
            p.loc = name.loc;

            if (match(TokenKind::Colon)) {
                p.type = parseType();
            } else {
                // Untyped param — type will be inferred from context
                p.type = {};
                p.type.kind = TypeRef::Kind::Named;
                p.type.name = "";
                p.type.loc = name.loc;
            }
            params.push_back(p);
            skipNewlines();
        } while (match(TokenKind::Comma));
        skipNewlines();
    }

    expect(TokenKind::FatArrow, "expected '=>' in lambda expression");
    skipNewlines();

    // Parse body — can be a single expression or multiple stmts + result
    // For simplicity: parse as a sequence of stmts ending with an expression
    std::vector<Stmt*> stmts;
    Expr* result = nullptr;

    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        if (check(TokenKind::KwVal) || check(TokenKind::KwVar)) {
            stmts.push_back(parseValDecl());
        } else {
            Expr* expr = parseExpr();
            // Check if this is followed by newline/semicolon/rbrace
            skipNewlines();
            if (check(TokenKind::RBrace) || check(TokenKind::Eof)) {
                result = expr;
                break;
            }
            // Treat as statement
            auto* es = arena_.make<ExprStmt>();
            es->kind = Stmt::Kind::ExprStmt;
            es->loc = expr->loc;
            es->expr = expr;
            stmts.push_back(es);
        }
        // Skip semicolons/newlines between statements
        while (match(TokenKind::Semicolon) || match(TokenKind::Newline)) {}
    }

    expect(TokenKind::RBrace, "expected '}' at end of lambda");

    // Build lambda body as a block
    auto* body = arena_.make<BlockExpr>();
    body->kind = Expr::Kind::Block;
    body->loc = loc;
    body->stmt_count = static_cast<uint32_t>(stmts.size());
    body->stmts = arena_.makeArray<Stmt*>(stmts.size());
    for (size_t i = 0; i < stmts.size(); ++i) body->stmts[i] = stmts[i];
    body->result = result;

    auto* lambda = arena_.make<LambdaExpr>();
    lambda->kind = Expr::Kind::Lambda;
    lambda->loc = loc;
    lambda->param_count = static_cast<uint32_t>(params.size());
    lambda->params = arena_.makeArray<Param>(params.size());
    for (size_t i = 0; i < params.size(); ++i) lambda->params[i] = params[i];
    lambda->body = body;

    // Return type is inferred (empty)
    lambda->return_type = {};
    lambda->return_type.kind = TypeRef::Kind::Named;
    lambda->return_type.name = "";
    lambda->return_type.loc = loc;

    return lambda;
}

} // namespace kern
