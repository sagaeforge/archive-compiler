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
    EXPECT_NE(out.str().find("expected function declaration"), std::string::npos);
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
