#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>
#include <memory>

using namespace kern;

struct ParseResult {
    std::string source;
    std::unique_ptr<Arena> arena;
    DiagnosticEngine diag;
    Module* mod = nullptr;

    ParseResult() : arena(std::make_unique<Arena>()) {}
};

static ParseResult parse(std::string src) {
    ParseResult r;
    r.source = std::move(src);
    Lexer lexer(r.source, "test.kern", r.diag);
    Parser parser(lexer, *r.arena, r.diag);
    r.mod = parser.parseModule();
    return r;
}

// ===== Existing tests =====

TEST(ParserTest, SimpleFunctionDecl) {
    auto r = parse("fn main() -> i64 { 42 }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_NE(r.mod, nullptr);
    ASSERT_EQ(r.mod->fn_count, 1u);
    EXPECT_EQ(r.mod->functions[0]->name, "main");
    EXPECT_EQ(r.mod->functions[0]->param_count, 0u);
    EXPECT_EQ(r.mod->functions[0]->return_type.name, "i64");
}

TEST(ParserTest, FunctionWithParams) {
    auto r = parse("fn add(a: i64, b: i64) -> i64 { a + b }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);
    EXPECT_EQ(r.mod->functions[0]->param_count, 2u);
    EXPECT_EQ(r.mod->functions[0]->params[0].name, "a");
    EXPECT_EQ(r.mod->functions[0]->params[1].name, "b");
}

TEST(ParserTest, IfElseExpression) {
    auto r = parse(
        "fn max(a: i64, b: i64) -> i64 {\n"
        "    if a > b { a } else { b }\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);

    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::If);
}

TEST(ParserTest, FibonacciFunction) {
    auto r = parse(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n }\n"
        "    else { fib(n - 1) + fib(n - 2) }\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors()) << [&]() {
        std::ostringstream out;
        r.diag.printAll(out);
        return out.str();
    }();
    ASSERT_EQ(r.mod->fn_count, 1u);
    EXPECT_EQ(r.mod->functions[0]->name, "fib");
}

TEST(ParserTest, MultipleFunctions) {
    auto r = parse(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n }\n"
        "    else { fib(n - 1) + fib(n - 2) }\n"
        "}\n"
        "fn main() -> i64 { fib(10) }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 2u);
}

TEST(ParserTest, ASTDump) {
    auto r = parse("fn main() -> i64 { 42 }");
    ASSERT_FALSE(r.diag.hasErrors());

    std::ostringstream out;
    dumpAST(r.mod, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("FnDecl(main"), std::string::npos);
    EXPECT_NE(dump.find("IntLit(42)"), std::string::npos);
}

// ===== New TDD tests =====

// --- Operator precedence: * binds tighter than + ---
TEST(ParserTest, OperatorPrecedenceMulAdd) {
    auto r = parse("fn main() -> i64 { 1 + 2 * 3 }");
    ASSERT_FALSE(r.diag.hasErrors());

    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    ASSERT_EQ(body->result->kind, Expr::Kind::BinOp);

    auto* top = static_cast<BinOpExpr*>(body->result);
    // top should be Add(1, Mul(2, 3))
    EXPECT_EQ(top->op, BinOpKind::Add);
    ASSERT_EQ(top->lhs->kind, Expr::Kind::IntLit);
    ASSERT_EQ(top->rhs->kind, Expr::Kind::BinOp);

    auto* rhs = static_cast<BinOpExpr*>(top->rhs);
    EXPECT_EQ(rhs->op, BinOpKind::Mul);
}

// --- Operator precedence: comparison vs arithmetic ---
TEST(ParserTest, OperatorPrecedenceCompArith) {
    auto r = parse("fn main() -> bool { 1 + 2 < 3 + 4 }");
    ASSERT_FALSE(r.diag.hasErrors());

    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* top = static_cast<BinOpExpr*>(body->result);
    // should be Lt(Add(1,2), Add(3,4))
    EXPECT_EQ(top->op, BinOpKind::Lt);
    ASSERT_EQ(top->lhs->kind, Expr::Kind::BinOp);
    ASSERT_EQ(top->rhs->kind, Expr::Kind::BinOp);
    EXPECT_EQ(static_cast<BinOpExpr*>(top->lhs)->op, BinOpKind::Add);
    EXPECT_EQ(static_cast<BinOpExpr*>(top->rhs)->op, BinOpKind::Add);
}

// --- Left-associativity: a - b - c = (a - b) - c ---
TEST(ParserTest, LeftAssociativity) {
    auto r = parse("fn main() -> i64 { 10 - 3 - 2 }");
    ASSERT_FALSE(r.diag.hasErrors());

    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* top = static_cast<BinOpExpr*>(body->result);
    // should be Sub(Sub(10,3), 2)
    EXPECT_EQ(top->op, BinOpKind::Sub);
    ASSERT_EQ(top->lhs->kind, Expr::Kind::BinOp);
    EXPECT_EQ(static_cast<BinOpExpr*>(top->lhs)->op, BinOpKind::Sub);
    ASSERT_EQ(top->rhs->kind, Expr::Kind::IntLit);
}

// --- Nested function calls: f(g(x)) ---
TEST(ParserTest, NestedFunctionCalls) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn g(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { f(g(1)) }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 3u);

    auto* body = static_cast<BlockExpr*>(r.mod->functions[2]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* outer = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(outer->callee, "f");
    ASSERT_EQ(outer->arg_count, 1u);
    ASSERT_EQ(outer->args[0]->kind, Expr::Kind::Call);
    auto* inner = static_cast<CallExpr*>(outer->args[0]);
    EXPECT_EQ(inner->callee, "g");
}

// --- Unary negation ---
TEST(ParserTest, UnaryNegation) {
    auto r = parse("fn main() -> i64 { -42 }");
    ASSERT_FALSE(r.diag.hasErrors());

    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::UnaryOp);
    auto* unary = static_cast<UnaryOpExpr*>(body->result);
    EXPECT_EQ(unary->op, UnaryOpKind_t::Neg);
    ASSERT_EQ(unary->operand->kind, Expr::Kind::IntLit);
}

// --- Unary not ---
TEST(ParserTest, UnaryNot) {
    auto r = parse("fn main() -> bool { not true }");
    ASSERT_FALSE(r.diag.hasErrors());

    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::UnaryOp);
    auto* unary = static_cast<UnaryOpExpr*>(body->result);
    EXPECT_EQ(unary->op, UnaryOpKind_t::Not);
}

// --- Boolean literals ---
TEST(ParserTest, BooleanLiterals) {
    auto r = parse("fn main() -> bool { true }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::BoolLit);
    EXPECT_TRUE(static_cast<BoolLitExpr*>(body->result)->value);
}

// --- Val declaration in block ---
TEST(ParserTest, ValDeclaration) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    x\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->stmt_count, 1u);
    ASSERT_EQ(body->stmts[0]->kind, Stmt::Kind::ValDecl);
    auto* decl = static_cast<ValDeclStmt*>(body->stmts[0]);
    EXPECT_EQ(decl->name, "x");
}

// --- Multiple val declarations ---
TEST(ParserTest, MultipleValDeclarations) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    val y: i64 = 20\n"
        "    x + y\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->stmt_count, 2u);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::BinOp);
}

// --- If without else ---
TEST(ParserTest, IfWithoutElse) {
    auto r = parse("fn main() -> i64 { if true { 42 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::If);
    auto* ifExpr = static_cast<IfExpr*>(body->result);
    EXPECT_NE(ifExpr->then_branch, nullptr);
    EXPECT_EQ(ifExpr->else_branch, nullptr);
}

// --- Return expression ---
TEST(ParserTest, ReturnExpression) {
    auto r = parse("fn main() -> i64 { return 42 }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Return);
}

// --- Function call with multiple args ---
TEST(ParserTest, FunctionCallMultipleArgs) {
    auto r = parse(
        "fn add3(a: i64, b: i64, c: i64) -> i64 { a + b + c }\n"
        "fn main() -> i64 { add3(1, 2, 3) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* call = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(call->arg_count, 3u);
}

// --- No-arg function call ---
TEST(ParserTest, NoArgFunctionCall) {
    auto r = parse(
        "fn zero() -> i64 { 0 }\n"
        "fn main() -> i64 { zero() }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* call = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(call->arg_count, 0u);
}

// --- Binary expression in call argument ---
TEST(ParserTest, ExprInCallArg) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { f(1 + 2) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    auto* call = static_cast<CallExpr*>(body->result);
    ASSERT_EQ(call->arg_count, 1u);
    EXPECT_EQ(call->args[0]->kind, Expr::Kind::BinOp);
}

// --- Logical operators: and / or ---
TEST(ParserTest, LogicalOperators) {
    auto r = parse("fn main() -> bool { true and false or true }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    // or binds less tightly than and => Or(And(true, false), true)
    ASSERT_EQ(body->result->kind, Expr::Kind::BinOp);
    auto* top = static_cast<BinOpExpr*>(body->result);
    EXPECT_EQ(top->op, BinOpKind::Or);
    ASSERT_EQ(top->lhs->kind, Expr::Kind::BinOp);
    EXPECT_EQ(static_cast<BinOpExpr*>(top->lhs)->op, BinOpKind::And);
}

// ===== Coverage improvement tests =====

// --- Error: unexpected top-level token ---
TEST(ParserTest, ErrorUnexpectedTopLevel) {
    auto r = parse("42");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected function, struct, enum, union, type, newtype, trait, or impl declaration"), std::string::npos);
}

// --- Error: missing function name ---
TEST(ParserTest, ErrorMissingFnName) {
    auto r = parse("fn () -> i64 { 42 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected function name"), std::string::npos);
}

// --- Error: missing '(' ---
TEST(ParserTest, ErrorMissingLParen) {
    auto r = parse("fn main -> i64 { 42 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected '('"), std::string::npos);
}

// --- Error: missing colon in parameter ---
TEST(ParserTest, ErrorMissingParamColon) {
    auto r = parse("fn f(a i64) -> i64 { a }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected ':'"), std::string::npos);
}

// --- Error: missing arrow ---
TEST(ParserTest, ErrorMissingArrow) {
    auto r = parse("fn main() i64 { 42 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected '->'"), std::string::npos);
}

// --- Error: unexpected token in expression position ---
TEST(ParserTest, ErrorUnexpectedExprToken) {
    auto r = parse("fn main() -> i64 { ] }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("unexpected token"), std::string::npos);
}

// --- Error: missing ')' in call ---
TEST(ParserTest, ErrorMissingCallRParen) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { f(1 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected ')'"), std::string::npos);
}

// --- Empty block ---
TEST(ParserTest, EmptyBlock) {
    auto r = parse("fn main() -> i64 { }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    EXPECT_EQ(body->stmt_count, 0u);
    EXPECT_EQ(body->result, nullptr);
}

// --- Block with statements only, no result ---
TEST(ParserTest, BlockWithStatementsOnly) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    EXPECT_EQ(body->stmt_count, 1u);
    EXPECT_EQ(body->result, nullptr);
}

// --- Semicolon separated statements ---
TEST(ParserTest, SemicolonSeparatedStatements) {
    auto r = parse("fn main() -> i64 { val x: i64 = 1; val y: i64 = 2; x + y }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    EXPECT_EQ(body->stmt_count, 2u);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::BinOp);
}

// --- Else-if chain ---
TEST(ParserTest, ElseIfChain) {
    auto r = parse("fn main() -> i64 { if false { 1 } else if false { 2 } else { 3 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::If);
    auto* top = static_cast<IfExpr*>(body->result);
    ASSERT_NE(top->else_branch, nullptr);
    // else branch is another if expression
    EXPECT_EQ(top->else_branch->kind, Expr::Kind::If);
}

// --- Nested if-else ---
TEST(ParserTest, NestedIfElse) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    if true { if true { 1 } else { 2 } } else { 3 }\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* outer = static_cast<IfExpr*>(body->result);
    ASSERT_EQ(outer->then_branch->kind, Expr::Kind::Block);
    auto* thenBlock = static_cast<BlockExpr*>(outer->then_branch);
    ASSERT_NE(thenBlock->result, nullptr);
    EXPECT_EQ(thenBlock->result->kind, Expr::Kind::If);
}

// --- Parenthesized expression ---
TEST(ParserTest, ParenthesizedExpr) {
    auto r = parse("fn main() -> i64 { (1 + 2) * 3 }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::BinOp);
    auto* top = static_cast<BinOpExpr*>(body->result);
    EXPECT_EQ(top->op, BinOpKind::Mul);
    ASSERT_EQ(top->lhs->kind, Expr::Kind::BinOp);
    EXPECT_EQ(static_cast<BinOpExpr*>(top->lhs)->op, BinOpKind::Add);
}

// --- Nested parenthesized ---
TEST(ParserTest, NestedParenthesized) {
    auto r = parse("fn main() -> i64 { ((42)) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::IntLit);
    EXPECT_EQ(static_cast<IntLitExpr*>(body->result)->value, 42);
}

// --- Division operator ---
TEST(ParserTest, DivisionOp) {
    auto r = parse("fn main() -> i64 { 10 / 2 }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::BinOp);
    EXPECT_EQ(static_cast<BinOpExpr*>(body->result)->op, BinOpKind::Div);
}

// --- All comparison operators ---
TEST(ParserTest, AllComparisonOps) {
    struct Case { const char* op_str; BinOpKind expected; };
    Case cases[] = {
        {"1 == 2", BinOpKind::Eq},
        {"1 != 2", BinOpKind::NotEq},
        {"1 < 2",  BinOpKind::Lt},
        {"1 <= 2", BinOpKind::LtEq},
        {"1 > 2",  BinOpKind::Gt},
        {"1 >= 2", BinOpKind::GtEq},
    };
    for (auto& c : cases) {
        std::string src = std::string("fn main() -> bool { ") + c.op_str + " }";
        auto r = parse(src);
        ASSERT_FALSE(r.diag.hasErrors()) << "Failed for: " << c.op_str;
        auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
        ASSERT_EQ(body->result->kind, Expr::Kind::BinOp) << "Failed for: " << c.op_str;
        EXPECT_EQ(static_cast<BinOpExpr*>(body->result)->op, c.expected) << "Failed for: " << c.op_str;
    }
}

// --- Var declaration ---
TEST(ParserTest, VarDeclaration) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->stmt_count, 1u);
    EXPECT_EQ(body->stmts[0]->kind, Stmt::Kind::VarDecl);
    auto* decl = static_cast<VarDeclStmt*>(body->stmts[0]);
    EXPECT_EQ(decl->name, "x");
}

// --- Return without value ---
TEST(ParserTest, ReturnWithoutValue) {
    auto r = parse("fn main() -> i64 { return }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Return);
    auto* ret = static_cast<ReturnExpr*>(body->result);
    EXPECT_EQ(ret->value, nullptr);
}

// --- Expression statement in block ---
TEST(ParserTest, ExprStatementInBlock) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 {\n"
        "    f(1)\n"
        "    42\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    EXPECT_EQ(body->stmt_count, 1u);
    EXPECT_EQ(body->stmts[0]->kind, Stmt::Kind::ExprStmt);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::IntLit);
}

// --- AST dump of var declaration ---
TEST(ParserTest, ASTDumpVarDecl) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("VarDecl(x: i64)"), std::string::npos);
}

// --- AST dump of expression statement ---
TEST(ParserTest, ASTDumpExprStmt) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 {\n"
        "    f(1)\n"
        "    42\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("ExprStmt"), std::string::npos);
}

// ===== Parser error recovery tests =====

// --- Error has correct location ---
TEST(ParserTest, ErrorHasLocation) {
    auto r = parse("fn main( -> i64 { 42 }");
    EXPECT_TRUE(r.diag.hasErrors());
    auto& diags = r.diag.diagnostics();
    ASSERT_GE(diags.size(), 1u);
    EXPECT_GT(diags[0].loc.line, 0u);
    EXPECT_GT(diags[0].loc.col, 0u);
}

// --- Parser continues after error to report more ---
TEST(ParserTest, MultipleFnsAfterError) {
    // First function is malformed, second is fine
    auto r = parse(
        "fn bad( -> i64 { 0 }\n"
        "fn good() -> i64 { 42 }");
    // Should parse at least one function even with errors
    EXPECT_NE(r.mod, nullptr);
}

// --- Missing return type arrow ---
TEST(ParserTest, MissingReturnTypeArrow) {
    auto r = parse("fn main() i64 { 42 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("->"), std::string::npos);
}

// --- Float literal f64 ---
TEST(ParserTest, FloatLiteralF64) {
    auto r = parse("fn main() -> f64 { 3.14 }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::FloatLit);
    auto* fl = static_cast<FloatLitExpr*>(body->result);
    EXPECT_DOUBLE_EQ(fl->value, 3.14);
    EXPECT_FALSE(fl->is_f32);
}

// --- Float literal f32 ---
TEST(ParserTest, FloatLiteralF32) {
    auto r = parse("fn main() -> f32 { 3.14f }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::FloatLit);
    auto* fl = static_cast<FloatLitExpr*>(body->result);
    EXPECT_DOUBLE_EQ(fl->value, 3.14);
    EXPECT_TRUE(fl->is_f32);
}

// --- Float literal AST dump ---
TEST(ParserTest, ASTDumpFloat) {
    auto r = parse("fn main() -> f64 { 3.14 }");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("FloatLit("), std::string::npos);
}

// --- Assignment in AST dump ---
TEST(ParserTest, ASTDumpAssign) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x = 20\n"
        "    x\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("Assign(x)"), std::string::npos);
}

// ===== M4a: Pipe operator tests =====

TEST(ParserTest, PipeSimple) {
    auto r = parse(
        "fn double(x: i64) -> i64 { x * 2 }\n"
        "fn main() -> i64 { 21 |> double }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* call = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(call->callee, "double");
    EXPECT_EQ(call->arg_count, 1u);
    EXPECT_EQ(call->args[0]->kind, Expr::Kind::IntLit);
    EXPECT_EQ(static_cast<IntLitExpr*>(call->args[0])->value, 21);
}

TEST(ParserTest, PipeWithArgs) {
    auto r = parse(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { 10 |> add(32) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* call = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(call->callee, "add");
    EXPECT_EQ(call->arg_count, 2u);
    EXPECT_EQ(static_cast<IntLitExpr*>(call->args[0])->value, 10);
    EXPECT_EQ(static_cast<IntLitExpr*>(call->args[1])->value, 32);
}

TEST(ParserTest, PipeChain) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn g(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { 1 |> f |> g }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[2]->body);
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* outer = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(outer->callee, "g");
    EXPECT_EQ(outer->arg_count, 1u);
    ASSERT_EQ(outer->args[0]->kind, Expr::Kind::Call);
    auto* inner = static_cast<CallExpr*>(outer->args[0]);
    EXPECT_EQ(inner->callee, "f");
}

TEST(ParserTest, PipeChainWithArgs) {
    auto r = parse(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn mul(a: i64, b: i64) -> i64 { a * b }\n"
        "fn main() -> i64 { 10 |> add(11) |> mul(2) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[2]->body);
    auto* outer = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(outer->callee, "mul");
    EXPECT_EQ(outer->arg_count, 2u);
    auto* inner = static_cast<CallExpr*>(outer->args[0]);
    EXPECT_EQ(inner->callee, "add");
    EXPECT_EQ(inner->arg_count, 2u);
}

TEST(ParserTest, PipePrecedence) {
    // Pipe has lower precedence than arithmetic
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { 1 + 2 |> f }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    // (1 + 2) |> f → f(1 + 2)
    ASSERT_EQ(body->result->kind, Expr::Kind::Call);
    auto* call = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(call->callee, "f");
    EXPECT_EQ(call->args[0]->kind, Expr::Kind::BinOp);
}

TEST(ParserTest, PipeMultipleArgs) {
    auto r = parse(
        "fn add3(a: i64, b: i64, c: i64) -> i64 { a + b + c }\n"
        "fn main() -> i64 { 10 |> add3(20, 30) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    auto* call = static_cast<CallExpr*>(body->result);
    EXPECT_EQ(call->callee, "add3");
    EXPECT_EQ(call->arg_count, 3u);
}

TEST(ParserTest, PipeInBlock) {
    auto r = parse(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 {\n"
        "    val x: i64 = 21 |> f\n"
        "    x\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[1]->body);
    EXPECT_EQ(body->stmt_count, 1u);
    auto* decl = static_cast<ValDeclStmt*>(body->stmts[0]);
    EXPECT_EQ(decl->init->kind, Expr::Kind::Call);
}

TEST(ParserTest, PipeError) {
    auto r = parse("fn main() -> i64 { 42 |> 99 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected function name or call after '|>'"), std::string::npos);
}

// ===== M4a: Intrinsic function tests =====

TEST(ParserTest, IntrinsicDecl) {
    auto r = parse("fn halt() -> Unit = intrinsic");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);
    EXPECT_TRUE(r.mod->functions[0]->is_intrinsic);
    EXPECT_EQ(r.mod->functions[0]->body, nullptr);
    EXPECT_EQ(r.mod->functions[0]->name, "halt");
}

TEST(ParserTest, IntrinsicWithParams) {
    auto r = parse("fn write(port: u16, value: u8) -> Unit = intrinsic");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);
    EXPECT_TRUE(r.mod->functions[0]->is_intrinsic);
    EXPECT_EQ(r.mod->functions[0]->param_count, 2u);
}

TEST(ParserTest, IntrinsicAST) {
    auto r = parse("fn halt() -> Unit = intrinsic");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("[intrinsic]"), std::string::npos);
}

TEST(ParserTest, IntrinsicAndNormalFn) {
    auto r = parse(
        "fn halt() -> Unit = intrinsic\n"
        "fn main() -> i64 { 42 }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 2u);
    EXPECT_TRUE(r.mod->functions[0]->is_intrinsic);
    EXPECT_FALSE(r.mod->functions[1]->is_intrinsic);
}

TEST(ParserTest, IntrinsicError) {
    auto r = parse("fn halt() -> Unit = foobar");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected 'intrinsic'"), std::string::npos);
}

// ===== M4b: Match expression parsing =====

TEST(ParserTest, MatchBasic) {
    auto r = parse("fn main() -> i64 { match 1 { 0 => 10, _ => 20 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = r.mod->functions[0]->body;
    ASSERT_EQ(body->kind, Expr::Kind::Block);
    auto* block = static_cast<BlockExpr*>(body);
    ASSERT_EQ(block->result->kind, Expr::Kind::Match);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 2u);
}

TEST(ParserTest, MatchWithGuard) {
    auto r = parse("fn main() -> i64 { match 1 { x if x > 0 => 1, _ => 0 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* block = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 2u);
    EXPECT_NE(match_expr->arms[0].guard, nullptr);
    EXPECT_EQ(match_expr->arms[0].pattern->kind, Pattern::Kind::Variable);
    EXPECT_EQ(match_expr->arms[1].guard, nullptr);
}

TEST(ParserTest, MultipleLiteralArms) {
    auto r = parse("fn main() -> i64 { match 1 { 0 => 10, 1 => 20, 2 => 30, _ => 40 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* block = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 4u);
    EXPECT_EQ(match_expr->arms[0].pattern->kind, Pattern::Kind::IntLit);
    EXPECT_EQ(match_expr->arms[3].pattern->kind, Pattern::Kind::Wildcard);
}

TEST(ParserTest, BoolMatch) {
    auto r = parse("fn main() -> i64 { match true { true => 1, false => 0 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* block = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 2u);
    EXPECT_EQ(match_expr->arms[0].pattern->kind, Pattern::Kind::BoolLit);
    EXPECT_EQ(match_expr->arms[1].pattern->kind, Pattern::Kind::BoolLit);
}

TEST(ParserTest, MatchBlockBody) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    match 1 {\n"
        "        0 => { val x: i64 = 10\n x },\n"
        "        _ => 20\n"
        "    }\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* block = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 2u);
    EXPECT_EQ(match_expr->arms[0].body->kind, Expr::Kind::Block);
}

TEST(ParserTest, MatchVariableBinding) {
    auto r = parse("fn main() -> i64 { match 42 { x => x } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* block = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 1u);
    EXPECT_EQ(match_expr->arms[0].pattern->kind, Pattern::Kind::Variable);
    auto* vp = static_cast<VariablePattern*>(match_expr->arms[0].pattern);
    EXPECT_EQ(vp->name, "x");
}

TEST(ParserTest, FnLevelPatternBasic) {
    auto r = parse(
        "fn fib(0) -> i64 { 0 }\n"
        "fn fib(1) -> i64 { 1 }\n"
        "fn fib(n: i64) -> i64 { n }");
    ASSERT_FALSE(r.diag.hasErrors());
    // After desugaring: one merged fib + no separate overloads
    ASSERT_EQ(r.mod->fn_count, 1u);
    EXPECT_EQ(r.mod->functions[0]->name, "fib");
    EXPECT_EQ(r.mod->functions[0]->param_count, 1u);
}

TEST(ParserTest, FnLevelPatternThreeOverloads) {
    auto r = parse(
        "fn f(0) -> i64 { 100 }\n"
        "fn f(1) -> i64 { 200 }\n"
        "fn f(2) -> i64 { 300 }\n"
        "fn f(n: i64) -> i64 { n }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);
    // The merged body should contain a match with 4 arms
    auto* body = r.mod->functions[0]->body;
    ASSERT_EQ(body->kind, Expr::Kind::Block);
    auto* block = static_cast<BlockExpr*>(body);
    ASSERT_EQ(block->result->kind, Expr::Kind::Match);
    auto* match_expr = static_cast<MatchExpr*>(block->result);
    EXPECT_EQ(match_expr->arm_count, 4u);
}

// ===== M5a: struct parsing tests =====

TEST(ParserTest, StructDecl) {
    auto r = parse(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->struct_count, 1u);
    auto* sd = r.mod->structs[0];
    EXPECT_EQ(sd->name, "Point");
    ASSERT_EQ(sd->field_count, 2u);
    EXPECT_EQ(sd->fields[0].name, "x");
    EXPECT_EQ(sd->fields[0].type.name, "i64");
    EXPECT_FALSE(sd->fields[0].is_mutable);
    EXPECT_EQ(sd->fields[1].name, "y");
}

TEST(ParserTest, StructDeclVarFields) {
    auto r = parse(
        "struct Mutable { val a: i64, var b: i64 }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->struct_count, 1u);
    EXPECT_FALSE(r.mod->structs[0]->fields[0].is_mutable);
    EXPECT_TRUE(r.mod->structs[0]->fields[1].is_mutable);
}

TEST(ParserTest, StructLiteral) {
    auto r = parse(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: i64 = 0\n"
        "    Point { x: 1, y: 2 }\n"
        "}");
    // We just check it parses without error
    // The StructLit should be the result expression
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::StructLit);
    auto* sl = static_cast<StructLitExpr*>(body->result);
    EXPECT_EQ(sl->struct_name, "Point");
    ASSERT_EQ(sl->field_count, 2u);
    EXPECT_EQ(sl->fields[0].name, "x");
    EXPECT_EQ(sl->fields[1].name, "y");
}

TEST(ParserTest, FieldAccess) {
    auto r = parse(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: i64 = 0\n"
        "    p.x\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::FieldAccess);
    auto* fa = static_cast<FieldAccessExpr*>(body->result);
    EXPECT_EQ(fa->field_name, "x");
}

TEST(ParserTest, NestedFieldAccess) {
    auto r = parse(
        "struct Inner { v: i64 }\n"
        "struct Outer { inner: Inner }\n"
        "fn main() -> i64 {\n"
        "    val o: i64 = 0\n"
        "    o.inner.v\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::FieldAccess);
    auto* fa = static_cast<FieldAccessExpr*>(body->result);
    EXPECT_EQ(fa->field_name, "v");
    EXPECT_EQ(fa->object->kind, Expr::Kind::FieldAccess);
}

TEST(ParserTest, FieldAssign) {
    auto r = parse(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn main() -> i64 {\n"
        "    var p: i64 = 0\n"
        "    p.x = 42\n"
        "    0\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_GE(body->stmt_count, 2u);
    // Second stmt should be FieldAssign
    auto* stmt = body->stmts[1];
    EXPECT_EQ(stmt->kind, Stmt::Kind::FieldAssign);
    auto* fas = static_cast<FieldAssignStmt*>(stmt);
    EXPECT_EQ(fas->target->kind, Expr::Kind::FieldAccess);
}

// ===== M5b: Enum + Union parsing =====

TEST(ParserTest, EnumDecl) {
    auto r = parse("enum Color { Red, Green, Blue }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->enum_count, 1u);
    auto* ed = r.mod->enums[0];
    EXPECT_EQ(ed->name, "Color");
    ASSERT_EQ(ed->variant_count, 3u);
    EXPECT_EQ(ed->variants[0].name, "Red");
    EXPECT_EQ(ed->variants[1].name, "Green");
    EXPECT_EQ(ed->variants[2].name, "Blue");
}

TEST(ParserTest, EnumAccess) {
    auto r = parse(
        "enum Color { Red, Green, Blue }\n"
        "fn main() -> i64 { Color.Red }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::EnumAccess);
    auto* ea = static_cast<EnumAccessExpr*>(body->result);
    EXPECT_EQ(ea->enum_name, "Color");
    EXPECT_EQ(ea->variant_name, "Red");
}

TEST(ParserTest, UnionDecl) {
    auto r = parse(
        "union Option { Some(i64), None }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->union_count, 1u);
    auto* ud = r.mod->unions[0];
    EXPECT_EQ(ud->name, "Option");
    ASSERT_EQ(ud->variant_count, 2u);
    EXPECT_EQ(ud->variants[0].name, "Some");
    ASSERT_NE(ud->variants[0].payload_type, nullptr);
    EXPECT_EQ(ud->variants[0].payload_type->name, "i64");
    EXPECT_EQ(ud->variants[1].name, "None");
    EXPECT_EQ(ud->variants[1].payload_type, nullptr);
}

TEST(ParserTest, UnionVariantExpr) {
    auto r = parse(
        "union Option { Some(i64), None }\n"
        "fn main() -> i64 { Option::Some(42) }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::UnionVariant);
    auto* uv = static_cast<UnionVariantExpr*>(body->result);
    EXPECT_EQ(uv->union_name, "Option");
    EXPECT_EQ(uv->variant_name, "Some");
    ASSERT_NE(uv->payload, nullptr);
    EXPECT_EQ(uv->payload->kind, Expr::Kind::IntLit);
}

TEST(ParserTest, UnionVariantEmpty) {
    auto r = parse(
        "union Option { Some(i64), None }\n"
        "fn main() -> i64 { Option::None }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::UnionVariant);
    auto* uv = static_cast<UnionVariantExpr*>(body->result);
    EXPECT_EQ(uv->union_name, "Option");
    EXPECT_EQ(uv->variant_name, "None");
    EXPECT_EQ(uv->payload, nullptr);
}

TEST(ParserTest, UnionVariantStructShorthand) {
    auto r = parse(
        "struct Circle { radius: i64 }\n"
        "union Shape { Circle(Circle), Empty }\n"
        "fn main() -> i64 { Shape::Circle { radius: 5 } }");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    ASSERT_NE(body->result, nullptr);
    EXPECT_EQ(body->result->kind, Expr::Kind::UnionVariant);
    auto* uv = static_cast<UnionVariantExpr*>(body->result);
    EXPECT_EQ(uv->union_name, "Shape");
    EXPECT_EQ(uv->variant_name, "Circle");
    ASSERT_NE(uv->payload, nullptr);
    EXPECT_EQ(uv->payload->kind, Expr::Kind::StructLit);
}

TEST(ParserTest, UnionPatternPayload) {
    auto r = parse(
        "union Option { Some(i64), None }\n"
        "fn main() -> i64 {\n"
        "    val x: i64 = 0\n"
        "    match x { Some(v) => v, _ => 0 }\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* matchE = static_cast<MatchExpr*>(body->result);
    ASSERT_GE(matchE->arm_count, 1u);
    auto* pat = matchE->arms[0].pattern;
    EXPECT_EQ(pat->kind, Pattern::Kind::Union);
    auto* up = static_cast<UnionPattern*>(pat);
    EXPECT_EQ(up->variant_name, "Some");
    ASSERT_NE(up->inner, nullptr);
    EXPECT_EQ(up->inner->kind, Pattern::Kind::Variable);
}

TEST(ParserTest, UnionPatternStructDestructure) {
    auto r = parse(
        "struct Circle { radius: i64 }\n"
        "union Shape { Circle(Circle), Empty }\n"
        "fn main() -> i64 {\n"
        "    val x: i64 = 0\n"
        "    match x { Circle { radius: r } => r, _ => 0 }\n"
        "}");
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* matchE = static_cast<MatchExpr*>(body->result);
    ASSERT_GE(matchE->arm_count, 1u);
    auto* pat = matchE->arms[0].pattern;
    EXPECT_EQ(pat->kind, Pattern::Kind::Union);
    auto* up = static_cast<UnionPattern*>(pat);
    EXPECT_EQ(up->variant_name, "Circle");
    ASSERT_EQ(up->field_binding_count, 1u);
    EXPECT_EQ(up->field_bindings[0].field_name, "radius");
    EXPECT_EQ(up->field_bindings[0].binding_name, "r");
}

TEST(ParserTest, EnumAndUnionDumpAST) {
    auto r = parse(
        "enum Color { Red, Green }\n"
        "union Option { Some(i64), None }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("EnumDecl(Color)"), std::string::npos);
    EXPECT_NE(dump.find("Red"), std::string::npos);
    EXPECT_NE(dump.find("UnionDecl(Option)"), std::string::npos);
    EXPECT_NE(dump.find("Some(i64)"), std::string::npos);
    EXPECT_NE(dump.find("None"), std::string::npos);
}

// ===== M5c: Pointer parsing =====

TEST(ParserTest, PtrTypeParam) {
    auto r = parse("fn read(p: Ptr<i64>) -> i64 { *p }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);
    auto* fn = r.mod->functions[0];
    ASSERT_EQ(fn->param_count, 1u);
    EXPECT_EQ(fn->params[0].type.kind, TypeRef::Kind::Ptr);
    EXPECT_FALSE(fn->params[0].type.is_ptr_var);
    ASSERT_NE(fn->params[0].type.pointee, nullptr);
    EXPECT_EQ(fn->params[0].type.pointee->name, "i64");
}

TEST(ParserTest, PtrVarTypeParam) {
    auto r = parse("fn write(p: Ptr<var i64>) -> Unit { *p = 42 }");
    ASSERT_FALSE(r.diag.hasErrors());
    ASSERT_EQ(r.mod->fn_count, 1u);
    auto* fn = r.mod->functions[0];
    ASSERT_EQ(fn->param_count, 1u);
    EXPECT_EQ(fn->params[0].type.kind, TypeRef::Kind::Ptr);
    EXPECT_TRUE(fn->params[0].type.is_ptr_var);
    ASSERT_NE(fn->params[0].type.pointee, nullptr);
    EXPECT_EQ(fn->params[0].type.pointee->name, "i64");
}

TEST(ParserTest, AddrOfExpr) {
    auto r = parse(
        "fn f() -> Ptr<i64> {\n"
        "    val x: i64 = 42\n"
        "    &x\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("UnaryOp(&)"), std::string::npos);
}

TEST(ParserTest, AddrOfVarExpr) {
    auto r = parse(
        "fn f() -> Ptr<var i64> {\n"
        "    var x: i64 = 42\n"
        "    &var x\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("UnaryOp(&var)"), std::string::npos);
}

TEST(ParserTest, DerefExpr) {
    auto r = parse("fn f(p: Ptr<i64>) -> i64 { *p }");
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("UnaryOp(*)"), std::string::npos);
}

TEST(ParserTest, DerefAssignStmt) {
    auto r = parse(
        "fn f(p: Ptr<var i64>) -> Unit {\n"
        "    *p = 99\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("DerefAssign"), std::string::npos);
}

TEST(ParserTest, DerefFieldAccess) {
    auto r = parse(
        "struct Point { x: i64, y: i64 }\n"
        "fn f(p: Ptr<Point>) -> i64 { (*p).x }"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    // Should contain FieldAccess with a Deref inside
    EXPECT_NE(out.str().find("FieldAccess(.x)"), std::string::npos);
    EXPECT_NE(out.str().find("UnaryOp(*)"), std::string::npos);
}

TEST(ParserTest, DerefFieldAssign) {
    auto r = parse(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn f(p: Ptr<var Point>) -> Unit {\n"
        "    (*p).x = 42\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    std::ostringstream out;
    dumpAST(r.mod, out);
    EXPECT_NE(out.str().find("DerefAssign"), std::string::npos);
}

TEST(ParserTest, PtrReturnType) {
    auto r = parse("fn f(x: i64) -> Ptr<i64> { &x }");
    ASSERT_FALSE(r.diag.hasErrors());
    EXPECT_EQ(r.mod->functions[0]->return_type.kind, TypeRef::Kind::Ptr);
}

// ===== String literal tests =====

TEST(ParserTest, StringLitExpr) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val s: String = \"hello\"\n"
        "    42\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* decl = static_cast<ValDeclStmt*>(body->stmts[0]);
    EXPECT_EQ(decl->init->kind, Expr::Kind::StringLit);
    auto* sl = static_cast<StringLitExpr*>(decl->init);
    EXPECT_EQ(sl->length, 5u);
    EXPECT_EQ(sl->data[0], 'h');
    EXPECT_EQ(sl->data[4], 'o');
}

TEST(ParserTest, StringLitEscapeProcessed) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val s: String = \"a\\nb\"\n"
        "    42\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* decl = static_cast<ValDeclStmt*>(body->stmts[0]);
    auto* sl = static_cast<StringLitExpr*>(decl->init);
    EXPECT_EQ(sl->length, 3u);
    EXPECT_EQ(sl->data[1], '\n');
}

TEST(ParserTest, StringLitEmpty) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val s: String = \"\"\n"
        "    42\n"
        "}"
    );
    ASSERT_FALSE(r.diag.hasErrors());
    auto* body = static_cast<BlockExpr*>(r.mod->functions[0]->body);
    auto* decl = static_cast<ValDeclStmt*>(body->stmts[0]);
    auto* sl = static_cast<StringLitExpr*>(decl->init);
    EXPECT_EQ(sl->length, 0u);
}

// ===== M5 Parser error path coverage =====

// --- Struct declaration: missing name ---
TEST(ParserTest, ErrorStructMissingName) {
    auto r = parse("struct { x: i64 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected struct name"), std::string::npos);
}

// --- Struct declaration: missing '{' ---
TEST(ParserTest, ErrorStructMissingLBrace) {
    auto r = parse("struct Foo x: i64 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected '{' after struct name"), std::string::npos);
}

// --- Struct declaration: missing ':' after field name ---
TEST(ParserTest, ErrorStructFieldMissingColon) {
    auto r = parse("struct Foo { x i64 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected ':'"), std::string::npos);
}

// --- Enum declaration: missing name ---
TEST(ParserTest, ErrorEnumMissingName) {
    auto r = parse("enum { Red, Green }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected enum name"), std::string::npos);
}

// --- Enum declaration: missing '{' ---
TEST(ParserTest, ErrorEnumMissingLBrace) {
    auto r = parse("enum Color Red, Green }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected '{' after enum name"), std::string::npos);
}

// --- Union declaration: missing name ---
TEST(ParserTest, ErrorUnionMissingName) {
    auto r = parse("union { Some(i64), None }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected union name"), std::string::npos);
}

// --- Union declaration: missing '{' ---
TEST(ParserTest, ErrorUnionMissingLBrace) {
    auto r = parse("union Shape Circle(i64) }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected '{' after union name"), std::string::npos);
}

// --- Union declaration: missing ')' after variant type ---
TEST(ParserTest, ErrorUnionVariantMissingRParen) {
    auto r = parse("union Shape { Circle(i64 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected ')'"), std::string::npos);
}

// --- Ptr type: missing '>' ---
TEST(ParserTest, ErrorPtrMissingGt) {
    auto r = parse("fn f(p: Ptr<i64) -> i64 { 0 }");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected '>'"), std::string::npos);
}

// --- Match: invalid pattern token ---
TEST(ParserTest, ErrorInvalidPatternToken) {
    auto r = parse(
        "fn main() -> i64 {\n"
        "    val x: i64 = 0\n"
        "    match x { [1] => 0, _ => 1 }\n"
        "}");
    EXPECT_TRUE(r.diag.hasErrors());
    std::ostringstream out;
    r.diag.printAll(out);
    EXPECT_NE(out.str().find("expected pattern"), std::string::npos);
}
