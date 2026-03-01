#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRDump.h"
#include "kern/hir/HIRPasses.h"
#include "kern/ir/Metadata.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class HIRPassesTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    HIRModule* buildAndRun(const char* source) {
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return nullptr;

        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return nullptr;

        HIRPassManager pm;
        pm.add<PurityAnalysisPass>();
        pm.add<TailCallAnalysisPass>();
        pm.run(*hir, ctx);
        return hir;
    }

    HIRFnDecl* findFn(HIRModule* mod, std::string_view name) {
        for (uint32_t i = 0; i < mod->fn_count; ++i) {
            if (mod->functions[i]->name == name) return mod->functions[i];
        }
        return nullptr;
    }
};

// ============================================================================
// PurityAnalysisPass tests
// ============================================================================

TEST_F(HIRPassesTest, PureFn) {
    auto* hir = buildAndRun("fn add(a: i64, b: i64) -> i64 { a + b }\n"
                             "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    auto* add_fn = findFn(hir, "add");
    ASSERT_NE(add_fn, nullptr);
    EXPECT_EQ(static_cast<Purity>(add_fn->purity), Purity::Pure);
}

TEST_F(HIRPassesTest, ImpureMut) {
    auto* hir = buildAndRun(
        "fn mutator() -> i64 {\n"
        "    var x: i64 = 0\n"
        "    x = 42\n"
        "    x\n"
        "}\n"
        "fn main() -> i64 { mutator() }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "mutator");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(static_cast<Purity>(fn->purity), Purity::ImpureMut);
}

TEST_F(HIRPassesTest, ImpureMutDoesNotPropagate) {
    auto* hir = buildAndRun(
        "fn mutator() -> i64 {\n"
        "    var x: i64 = 0\n"
        "    x = 42\n"
        "    x\n"
        "}\n"
        "fn caller() -> i64 { mutator() }\n"
        "fn main() -> i64 { caller() }");
    ASSERT_NE(hir, nullptr);
    auto* caller = findFn(hir, "caller");
    ASSERT_NE(caller, nullptr);
    // ImpureMut does NOT propagate
    EXPECT_EQ(static_cast<Purity>(caller->purity), Purity::Pure);
}

TEST_F(HIRPassesTest, IntrinsicImpureIo) {
    auto* hir = buildAndRun(
        "fn write(code: i64) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "write");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(static_cast<Purity>(fn->purity), Purity::ImpureIo);
}

TEST_F(HIRPassesTest, ImpureIoPropagates) {
    auto* hir = buildAndRun(
        "fn write(code: i64) -> Unit = intrinsic\n"
        "fn caller() -> Unit { write(0) }\n"
        "fn main() -> i64 {\n"
        "    caller()\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    auto* caller = findFn(hir, "caller");
    ASSERT_NE(caller, nullptr);
    EXPECT_EQ(static_cast<Purity>(caller->purity), Purity::ImpureIo);

    auto* main_fn = findFn(hir, "main");
    ASSERT_NE(main_fn, nullptr);
    EXPECT_EQ(static_cast<Purity>(main_fn->purity), Purity::ImpureIo);
}

TEST_F(HIRPassesTest, ImpureMemFromPtrWrite) {
    auto* hir = buildAndRun(
        "fn writer() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    val p: Ptr<var i64> = &var x\n"
        "    *p = 42\n"
        "    x\n"
        "}\n"
        "fn main() -> i64 { writer() }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "writer");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(static_cast<Purity>(fn->purity), Purity::ImpureMem);
}

TEST_F(HIRPassesTest, ImpureMemPropagates) {
    auto* hir = buildAndRun(
        "fn writer() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    val p: Ptr<var i64> = &var x\n"
        "    *p = 42\n"
        "    x\n"
        "}\n"
        "fn caller() -> i64 { writer() }\n"
        "fn main() -> i64 { caller() }");
    ASSERT_NE(hir, nullptr);
    auto* caller = findFn(hir, "caller");
    ASSERT_NE(caller, nullptr);
    EXPECT_EQ(static_cast<Purity>(caller->purity), Purity::ImpureMem);
}

// ============================================================================
// TailCallAnalysisPass tests
// ============================================================================

TEST_F(HIRPassesTest, TailCallSimple) {
    auto* hir = buildAndRun(
        "fn foo() -> i64 { 0 }\n"
        "fn main() -> i64 { foo() }");
    ASSERT_NE(hir, nullptr);
    // main's body is a block with result = call foo. The call is in tail position.
    auto* main_fn = findFn(hir, "main");
    ASSERT_NE(main_fn, nullptr);
    auto* body = static_cast<HIRBlockExpr*>(main_fn->body);
    ASSERT_NE(body->result, nullptr);
    ASSERT_EQ(body->result->kind, HIRExpr::Kind::Call);
    auto* call = static_cast<HIRCallExpr*>(body->result);
    EXPECT_TRUE(call->is_tail_call);
}

TEST_F(HIRPassesTest, NonTailCallInBinOp) {
    auto* hir = buildAndRun(
        "fn foo() -> i64 { 0 }\n"
        "fn main() -> i64 { foo() + 1 }");
    ASSERT_NE(hir, nullptr);
    // foo() is inside a BinOp, so NOT in tail position
    auto* main_fn = findFn(hir, "main");
    auto* body = static_cast<HIRBlockExpr*>(main_fn->body);
    auto* binop = static_cast<HIRBinOpExpr*>(body->result);
    ASSERT_EQ(binop->lhs->kind, HIRExpr::Kind::Call);
    auto* call = static_cast<HIRCallExpr*>(binop->lhs);
    EXPECT_FALSE(call->is_tail_call);
}

TEST_F(HIRPassesTest, RecursiveFunction) {
    auto* hir = buildAndRun(
        "fn fib(n: i64) -> i64 {\n"
        "    if n < 2 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}\n"
        "fn main() -> i64 { fib(10) }");
    ASSERT_NE(hir, nullptr);
    auto* fib = findFn(hir, "fib");
    ASSERT_NE(fib, nullptr);
    EXPECT_TRUE(fib->is_recursive);
    EXPECT_FALSE(fib->is_tail_recursive); // fib(n-1)+fib(n-2) is NOT tail
}

TEST_F(HIRPassesTest, TailRecursiveFunction) {
    auto* hir = buildAndRun(
        "fn count(n: i64, acc: i64) -> i64 {\n"
        "    if n < 1 { acc } else { count(n - 1, acc + 1) }\n"
        "}\n"
        "fn main() -> i64 { count(100, 0) }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "count");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_recursive);
    EXPECT_TRUE(fn->is_tail_recursive);
}

TEST_F(HIRPassesTest, TailCallInIfBranches) {
    auto* hir = buildAndRun(
        "fn foo(n: i64) -> i64 { n }\n"
        "fn bar(n: i64) -> i64 { n + 1 }\n"
        "fn main() -> i64 {\n"
        "    if true { foo(1) } else { bar(2) }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    // Both calls should be in tail position (if branches are tail)
}

TEST_F(HIRPassesTest, TailCallInMatchArms) {
    auto* hir = buildAndRun(
        "fn foo(n: i64) -> i64 { n }\n"
        "fn bar(n: i64) -> i64 { n + 1 }\n"
        "fn main() -> i64 {\n"
        "    match true {\n"
        "        true => foo(1)\n"
        "        false => bar(2)\n"
        "    }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    // Both calls in match arms should be tail calls
}

TEST_F(HIRPassesTest, NonRecursiveFunction) {
    auto* hir = buildAndRun(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "add");
    ASSERT_NE(fn, nullptr);
    EXPECT_FALSE(fn->is_recursive);
    EXPECT_FALSE(fn->is_tail_recursive);
}

TEST_F(HIRPassesTest, ReturnTailPosition) {
    auto* hir = buildAndRun(
        "fn foo() -> i64 { 0 }\n"
        "fn main() -> i64 { return foo() }");
    ASSERT_NE(hir, nullptr);
    // return foo() — foo is in tail position via return
}

// ============================================================================
// Dump integration with passes
// ============================================================================

TEST_F(HIRPassesTest, DumpShowsPurity) {
    auto* hir = buildAndRun(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    std::ostringstream out;
    dumpHIR(hir, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("[pure]") != std::string::npos);
}

TEST_F(HIRPassesTest, DumpShowsTailCall) {
    auto* hir = buildAndRun(
        "fn count(n: i64, acc: i64) -> i64 {\n"
        "    if n < 1 { acc } else { count(n - 1, acc + 1) }\n"
        "}\n"
        "fn main() -> i64 { count(100, 0) }");
    ASSERT_NE(hir, nullptr);
    std::ostringstream out;
    dumpHIR(hir, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("[tail]") != std::string::npos);
    EXPECT_TRUE(s.find("[tail-recursive]") != std::string::npos);
}

TEST_F(HIRPassesTest, DumpShowsRecursive) {
    auto* hir = buildAndRun(
        "fn fib(n: i64) -> i64 {\n"
        "    if n < 2 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}\n"
        "fn main() -> i64 { fib(10) }");
    ASSERT_NE(hir, nullptr);
    std::ostringstream out;
    dumpHIR(hir, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("[recursive]") != std::string::npos);
    // Should NOT be tail-recursive
    EXPECT_TRUE(s.find("[tail-recursive]") == std::string::npos);
}

} // namespace kern
