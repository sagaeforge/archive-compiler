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
    }
}

void dumpAST(const Module* mod, std::ostream& out, int /*ind*/) {
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        auto* fn = mod->functions[i];
        out << "FnDecl(" << fn->name << "(";
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            if (j > 0) out << ", ";
            out << fn->params[j].name << ": " << fn->params[j].type.name;
        }
        out << ") -> " << fn->return_type.name << ")\n";
        dumpExpr(fn->body, out, 1);
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
    skipNewlines();
    while (!check(TokenKind::Eof)) {
        if (check(TokenKind::KwFn)) {
            FnDecl* fn = parseFnDecl();
            if (fn) fns.push_back(fn);
        } else {
            diag_.error(peek().loc, "expected function declaration");
            advance();
        }
        skipNewlines();
    }

    auto* mod = arena_.make<Module>();
    mod->fn_count = static_cast<uint32_t>(fns.size());
    mod->functions = arena_.makeArray<FnDecl*>(fns.size());
    for (size_t i = 0; i < fns.size(); ++i) {
        mod->functions[i] = fns[i];
    }
    return mod;
}

// --- Function Declaration ---
FnDecl* Parser::parseFnDecl() {
    SourceLocation loc = peek().loc;
    expect(TokenKind::KwFn, "expected 'fn'");

    Token nameTok = expect(TokenKind::Ident, "expected function name");

    expect(TokenKind::LParen, "expected '(' after function name");

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

    skipNewlines();
    Expr* body = parseBlockExpr();

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
        case TokenKind::KwOr:    return {2, 3};
        case TokenKind::KwAnd:   return {4, 5};
        case TokenKind::EqEq:
        case TokenKind::NotEq:   return {6, 7};
        case TokenKind::Lt:
        case TokenKind::LtEq:
        case TokenKind::Gt:
        case TokenKind::GtEq:    return {8, 9};
        case TokenKind::Plus:
        case TokenKind::Minus:   return {10, 11};
        case TokenKind::Star:
        case TokenKind::Slash:   return {20, 21};
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

    // Identifier or function call
    if (tok.kind == TokenKind::Ident) {
        advance();
        if (check(TokenKind::LParen)) {
            return parseCallExpr(tok.text, tok.loc);
        }
        auto* ident = arena_.make<IdentExpr>();
        ident->kind = Expr::Kind::Ident;
        ident->loc = tok.loc;
        ident->name = tok.text;
        return ident;
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
            // Could be assignment (ident = expr) or an expression
            Token identTok = peek();
            advance();
            skipNewlines();
            if (check(TokenKind::Eq)) {
                // Assignment: ident = expr
                advance(); // consume '='
                skipNewlines();
                Expr* value = parseExpr();
                auto* assign = arena_.make<AssignStmt>();
                assign->kind = Stmt::Kind::Assign;
                assign->loc = identTok.loc;
                assign->name = identTok.text;
                assign->value = value;
                stmts.push_back(assign);
            } else {
                // Not assignment — was an expression starting with ident
                // Build the ident expression, then continue Pratt parsing
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
