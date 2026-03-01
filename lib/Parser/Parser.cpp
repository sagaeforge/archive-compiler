#include "kern/parser/Parser.h"
#include <sstream>
#include <iostream>

namespace kern {

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
        case Expr::Kind::Ident:
            out << "Ident(" << static_cast<const IdentExpr*>(expr)->name << ")\n";
            break;
        case Expr::Kind::BinOp: {
            auto* b = static_cast<const BinOpExpr*>(expr);
            const char* ops[] = {"+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">=", "and", "or"};
            out << "BinOp(" << ops[static_cast<int>(b->op)] << ")\n";
            dumpExpr(b->lhs, out, ind + 1);
            dumpExpr(b->rhs, out, ind + 1);
            break;
        }
        case Expr::Kind::UnaryOp: {
            auto* u = static_cast<const UnaryOpExpr*>(expr);
            const char* ops[] = {"-", "not", "*", "&"};
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
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        auto* fn = mod->functions[i];
        out << "FnDecl(" << fn->name << "(";
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            if (j > 0) out << ", ";
            out << fn->params[j].name << ": " << fn->params[j].type.name;
        }
        out << ") -> " << fn->return_type.name;
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
    skipNewlines();
    while (!check(TokenKind::Eof)) {
        if (check(TokenKind::KwStruct)) {
            StructDecl* sd = parseStructDecl();
            if (sd) {
                structs.push_back(sd);
                struct_names_.insert(sd->name);
            }
        } else if (check(TokenKind::KwFn)) {
            FnDecl* fn = parseFnDecl();
            if (fn) fns.push_back(fn);
        } else {
            diag_.error(peek().loc, "expected function or struct declaration");
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
    return mod;
}

// --- Struct Declaration ---
StructDecl* Parser::parseStructDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwStruct, "expected 'struct'");
    Token nameTok = expect(TokenKind::Ident, "expected struct name");
    skipNewlines();
    expect(TokenKind::LBrace, "expected '{' after struct name");
    skipNewlines();

    std::vector<FieldDecl> fields;
    while (!check(TokenKind::RBrace) && !check(TokenKind::Eof)) {
        FieldDecl fd;
        fd.is_mutable = false;
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
    return sd;
}

// --- Function Declaration ---
FnDecl* Parser::parseFnDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwFn, "expected 'fn'");

    Token nameTok = expect(TokenKind::Ident, "expected function name");

    expect(TokenKind::LParen, "expected '(' after function name");

    std::vector<Param> params;
    bool has_pattern_params = false;
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
            pat->value = static_cast<int64_t>(std::stoull(std::string(first.text)));
            pattern_param = pat;
        } else if (first.kind == TokenKind::Minus) {
            // fn f(-1) -> i64 { ... }
            advance();
            Token num = expect(TokenKind::IntLit, "expected integer after '-' in pattern parameter");
            has_pattern_params = true;
            auto* pat = arena_.make<IntLitPattern>();
            pat->kind = Pattern::Kind::IntLit;
            pat->loc = first.loc;
            pat->value = -static_cast<int64_t>(std::stoull(std::string(num.text)));
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
                params.push_back(parseParam());
            }
        }
    }
    expect(TokenKind::RParen, "expected ')' after parameters");

    expect(TokenKind::Arrow, "expected '->' for return type");
    TypeRef ret = parseType();

    skipNewlines();

    // Check for `= intrinsic`
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
    } else {
        body = parseBlockExpr();
    }

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
    fn->has_pattern_params = has_pattern_params;
    fn->pattern_param = pattern_param;
    return fn;
}

Param Parser::parseParam() {
    Token name = expect(TokenKind::Ident, "expected parameter name");
    expect(TokenKind::Colon, "expected ':' after parameter name");
    TypeRef type = parseType();
    return {name.text, type, name.loc};
}

TypeRef Parser::parseType() {
    Token tok = expect(TokenKind::Ident, "expected type name");
    return {TypeRef::Kind::Named, tok.text, tok.loc};
}

// --- Expressions ---

Parser::InfixBP Parser::infixBP(TokenKind kind) {
    switch (kind) {
        case TokenKind::Pipe:    return {1, 2};
        case TokenKind::KwOr:    return {3, 4};
        case TokenKind::KwAnd:   return {5, 6};
        case TokenKind::EqEq:
        case TokenKind::NotEq:   return {7, 8};
        case TokenKind::Lt:
        case TokenKind::LtEq:
        case TokenKind::Gt:
        case TokenKind::GtEq:    return {9, 10};
        case TokenKind::Plus:
        case TokenKind::Minus:   return {11, 12};
        case TokenKind::Star:
        case TokenKind::Slash:   return {20, 21};
        case TokenKind::Dot:     return {30, 31};
        default:                 return {0, 0};
    }
}

uint8_t Parser::prefixBP(TokenKind kind) {
    switch (kind) {
        case TokenKind::Minus:
        case TokenKind::KwNot:
            return 25;
        default:
            return 0;
    }
}

BinOpKind Parser::tokenToBinOp(TokenKind kind) {
    switch (kind) {
        case TokenKind::Plus:    return BinOpKind::Add;
        case TokenKind::Minus:   return BinOpKind::Sub;
        case TokenKind::Star:    return BinOpKind::Mul;
        case TokenKind::Slash:   return BinOpKind::Div;
        case TokenKind::EqEq:    return BinOpKind::Eq;
        case TokenKind::NotEq:   return BinOpKind::NotEq;
        case TokenKind::Lt:      return BinOpKind::Lt;
        case TokenKind::LtEq:    return BinOpKind::LtEq;
        case TokenKind::Gt:      return BinOpKind::Gt;
        case TokenKind::GtEq:    return BinOpKind::GtEq;
        case TokenKind::KwAnd:   return BinOpKind::And;
        case TokenKind::KwOr:    return BinOpKind::Or;
        default:                 return BinOpKind::Add; // unreachable
    }
}

Expr* Parser::parseExprInfix(Expr* lhs, uint8_t minBP) {
    while (true) {
        skipNewlines();
        TokenKind op = peek().kind;
        auto [lBP, rBP] = infixBP(op);
        if (lBP == 0 || lBP < minBP) break;

        Token opTok = advance();
        skipNewlines();

        // Dot operator: field access
        if (op == TokenKind::Dot) {
            Token field = expect(TokenKind::Ident, "expected field name after '.'");
            auto* fa = arena_.make<FieldAccessExpr>();
            fa->kind = Expr::Kind::FieldAccess;
            fa->loc = opTok.loc;
            fa->object = lhs;
            fa->field_name = field.text;
            lhs = fa;
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
    if (tok.kind == TokenKind::Minus || tok.kind == TokenKind::KwNot) {
        Token opTok = advance();
        uint8_t rBP = prefixBP(opTok.kind);
        Expr* operand = parseExpr(rBP);

        auto* unary = arena_.make<UnaryOpExpr>();
        unary->kind = Expr::Kind::UnaryOp;
        unary->loc = opTok.loc;
        unary->op = (opTok.kind == TokenKind::Minus) ? UnaryOpKind_t::Neg : UnaryOpKind_t::Not;
        unary->operand = operand;
        return unary;
    }

    // Integer literal
    if (tok.kind == TokenKind::IntLit) {
        advance();
        auto* lit = arena_.make<IntLitExpr>();
        lit->kind = Expr::Kind::IntLit;
        lit->loc = tok.loc;
        // Parse int value
        if (tok.text.size() > 2 && tok.text[0] == '0' && (tok.text[1] == 'x' || tok.text[1] == 'X')) {
            lit->value = static_cast<int64_t>(std::stoull(std::string(tok.text), nullptr, 16));
        } else {
            lit->value = static_cast<int64_t>(std::stoull(std::string(tok.text)));
        }
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

    // Identifier, function call, or struct literal
    if (tok.kind == TokenKind::Ident) {
        advance();
        if (check(TokenKind::LParen)) {
            return parseCallExpr(tok.text, tok.loc);
        }
        if (check(TokenKind::LBrace) && struct_names_.count(tok.text)) {
            return parseStructLit(tok.text, tok.loc);
        }
        auto* ident = arena_.make<IdentExpr>();
        ident->kind = Expr::Kind::Ident;
        ident->loc = tok.loc;
        ident->name = tok.text;
        return ident;
    }

    // Match expression
    if (tok.kind == TokenKind::KwMatch) {
        return parseMatchExpr();
    }

    // If expression
    if (tok.kind == TokenKind::KwIf) {
        return parseIfExpr();
    }

    // Block expression
    if (tok.kind == TokenKind::LBrace) {
        return parseBlockExpr();
    }

    // Parenthesized expression
    if (tok.kind == TokenKind::LParen) {
        advance();
        skipNewlines();
        Expr* expr = parseExpr();
        skipNewlines();
        expect(TokenKind::RParen, "expected ')'");
        return expr;
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
            } else {
                auto* ident = arena_.make<IdentExpr>();
                ident->kind = Expr::Kind::Ident;
                ident->loc = identTok.loc;
                ident->name = identTok.text;
                lhs = ident;
            }

            // Parse dot chains only (just Dot infix, highest precedence)
            while (check(TokenKind::Dot)) {
                Token dotTok = advance();
                Token field = expect(TokenKind::Ident, "expected field name after '.'");
                auto* fa = arena_.make<FieldAccessExpr>();
                fa->kind = Expr::Kind::FieldAccess;
                fa->loc = dotTok.loc;
                fa->object = lhs;
                fa->field_name = field.text;
                lhs = fa;
            }
            skipNewlines();

            if (check(TokenKind::Eq)) {
                advance(); // consume '='
                skipNewlines();
                Expr* value = parseExpr();
                if (lhs->kind == Expr::Kind::FieldAccess) {
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

            // If the next token is '}', this is the result expression
            if (check(TokenKind::RBrace)) {
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

Pattern* Parser::parsePattern() {
    Token tok = peek();

    // Integer literal pattern (including negative)
    if (tok.kind == TokenKind::IntLit) {
        advance();
        auto* pat = arena_.make<IntLitPattern>();
        pat->kind = Pattern::Kind::IntLit;
        pat->loc = tok.loc;
        pat->value = static_cast<int64_t>(std::stoull(std::string(tok.text)));
        return pat;
    }

    // Negative integer literal pattern
    if (tok.kind == TokenKind::Minus) {
        advance();
        Token num = expect(TokenKind::IntLit, "expected integer after '-' in pattern");
        auto* pat = arena_.make<IntLitPattern>();
        pat->kind = Pattern::Kind::IntLit;
        pat->loc = tok.loc;
        pat->value = -static_cast<int64_t>(std::stoull(std::string(num.text)));
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

    // Wildcard or variable pattern
    if (tok.kind == TokenKind::Ident) {
        advance();
        if (tok.text == "_") {
            auto* pat = arena_.make<WildcardPattern>();
            pat->kind = Pattern::Kind::Wildcard;
            pat->loc = tok.loc;
            return pat;
        }
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

    Token name = expect(TokenKind::Ident, "expected variable name");
    expect(TokenKind::Colon, "expected ':' after variable name");
    TypeRef type = parseType();
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

} // namespace kern
