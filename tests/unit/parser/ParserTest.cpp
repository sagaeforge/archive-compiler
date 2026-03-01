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
