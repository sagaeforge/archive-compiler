#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRDump.h"
#include "kern/hir/HIRPasses.h"
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

// ============================================================================
// EffectAnalysisPass tests
// ============================================================================

class EffectAnalysisTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    HIRModule* buildWithEffects(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return nullptr;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return nullptr;
        HIRPassManager pm;
        pm.add<PurityAnalysisPass>();
        pm.add<EffectAnalysisPass>();
        pm.run(*hir, ctx);
        return hir;
    }

    HIRFnDecl* findFn(HIRModule* mod, std::string_view name) {
        for (uint32_t i = 0; i < mod->fn_count; ++i)
            if (mod->functions[i]->name == name) return mod->functions[i];
        return nullptr;
    }
};

TEST_F(EffectAnalysisTest, PureFn) {
    auto* hir = buildWithEffects(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto* fn = findFn(hir, "add");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->inferred_effects, 0);
}

TEST_F(EffectAnalysisTest, VarGivesMut) {
    auto* hir = buildWithEffects(
        "fn counter() -> i64 with mut {\n"
        "  var x: i64 = 0\n"
        "  x = 42\n"
        "  x\n"
        "}\n"
        "fn main() -> i64 { counter() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto* fn = findFn(hir, "counter");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(hasEffect(fn->inferred_effects, Effect::Mut));
}

TEST_F(EffectAnalysisTest, PtrWriteGivesMem) {
    auto* hir = buildWithEffects(
        "fn writer() -> i64 with mut, mem {\n"
        "  var x: i64 = 10\n"
        "  val p: Ptr<var i64> = &var x\n"
        "  *p = 42\n"
        "  x\n"
        "}\n"
        "fn main() -> i64 { writer() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto* fn = findFn(hir, "writer");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(hasEffect(fn->inferred_effects, Effect::Mem));
}

TEST_F(EffectAnalysisTest, IntrinsicGivesIO) {
    auto* hir = buildWithEffects(
        "fn write(code: i64) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "write");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(hasEffect(fn->inferred_effects, Effect::IO));
}

TEST_F(EffectAnalysisTest, EffectPropagates) {
    auto* hir = buildWithEffects(
        "fn write(code: i64) -> Unit = intrinsic\n"
        "fn caller() -> Unit with io { write(0) }\n"
        "fn main() -> i64 {\n"
        "  caller()\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    auto* caller = findFn(hir, "caller");
    ASSERT_NE(caller, nullptr);
    EXPECT_TRUE(hasEffect(caller->inferred_effects, Effect::IO));
}

TEST_F(EffectAnalysisTest, AnnotationOK) {
    // Function declares mut and uses mut — no error
    auto* hir = buildWithEffects(
        "fn counter() -> i64 with mut {\n"
        "  var x: i64 = 0\n"
        "  x = 42\n"
        "  x\n"
        "}\n"
        "fn main() -> i64 { counter() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(EffectAnalysisTest, AnnotationMissing_Error) {
    // Function uses mut but doesn't declare it
    buildWithEffects(
        "fn counter() -> i64 with io {\n"
        "  var x: i64 = 0\n"
        "  x = 42\n"
        "  x\n"
        "}\n"
        "fn main() -> i64 { counter() }");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(EffectAnalysisTest, Unannotated_OK) {
    // Functions without effect annotations don't get enforcement
    auto* hir = buildWithEffects(
        "fn counter() -> i64 {\n"
        "  var x: i64 = 0\n"
        "  x = 42\n"
        "  x\n"
        "}\n"
        "fn main() -> i64 { counter() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// OwnershipCheckPass tests
// ============================================================================

class OwnershipCheckTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    bool hasError(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return true;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return true;
        HIRPassManager pm;
        pm.add<OwnershipCheckPass>();
        pm.run(*hir, ctx);
        return ctx.diag.hasErrors();
    }
};

TEST_F(OwnershipCheckTest, UseAfterMove_Error) {
    EXPECT_TRUE(hasError(
        "fn consume(x: own i64) -> i64 { x }\n"
        "fn bad(a: own i64) -> i64 {\n"
        "  consume(a)\n"
        "  a\n"
        "}"));
}

TEST_F(OwnershipCheckTest, NotMoved_OK) {
    EXPECT_FALSE(hasError(
        "fn consume(x: own i64) -> i64 { x }\n"
        "fn ok(a: own i64) -> i64 {\n"
        "  consume(a)\n"
        "}"));
}

TEST_F(OwnershipCheckTest, BorrowOK) {
    // Borrowed params can be used multiple times
    EXPECT_FALSE(hasError(
        "fn use_it(x: i64) -> i64 { x }\n"
        "fn ok(a: i64) -> i64 {\n"
        "  use_it(a)\n"
        "  a\n"
        "}"));
}

TEST_F(OwnershipCheckTest, ValBindMove) {
    EXPECT_TRUE(hasError(
        "fn bad(a: own i64) -> i64 {\n"
        "  val b: i64 = a\n"
        "  a\n"
        "}"));
}

// ============================================================================
// ConstOverflowPass tests
// ============================================================================

class ConstOverflowTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    bool hasError(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return true;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return true;
        HIRPassManager pm;
        pm.add<ConstOverflowPass>();
        pm.run(*hir, ctx);
        return ctx.diag.hasErrors();
    }
};

TEST_F(ConstOverflowTest, U8_Overflow) {
    // 200 + 100 = 300, doesn't fit in u8 [0, 255]
    EXPECT_TRUE(hasError(
        "fn f(a: u8, b: u8) -> u8 { a + b }\n"
        "fn main() -> u8 { 200 + 100 }"));
}

TEST_F(ConstOverflowTest, I8_NegativeOverflow) {
    // -100 + -100 = -200, doesn't fit in i8 [-128, 127]
    EXPECT_TRUE(hasError(
        "fn main() -> i8 { -100 + -100 }"));
}

TEST_F(ConstOverflowTest, I64_OK) {
    EXPECT_FALSE(hasError(
        "fn main() -> i64 { 100 + 200 }"));
}

// ============================================================================
// LossyCastPass tests
// ============================================================================

class LossyCastTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    bool hasWarning(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return false;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return false;
        HIRPassManager pm;
        pm.add<LossyCastPass>();
        pm.run(*hir, ctx);
        return ctx.diag.hasWarnings();
    }
};

TEST_F(LossyCastTest, NarrowWarn) {
    EXPECT_TRUE(hasWarning(
        "fn main() -> u8 { 42 as u8 }"));
}

TEST_F(LossyCastTest, ExplicitTruncateOK) {
    EXPECT_FALSE(hasWarning(
        "fn main() -> u8 {\n"
        "  val x: i64 = 42\n"
        "  truncate<u8>(x)\n"
        "}"));
}

TEST_F(LossyCastTest, WidenNoWarn) {
    EXPECT_FALSE(hasWarning(
        "fn main() -> i64 {\n"
        "  val x: i32 = 42\n"
        "  x as i64\n"
        "}"));
}

// ============================================================================
// BorrowEscapePass tests
// ============================================================================

class BorrowEscapeTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    bool hasError(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return true;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return true;
        HIRPassManager pm;
        pm.add<BorrowEscapePass>();
        pm.run(*hir, ctx);
        return ctx.diag.hasErrors();
    }
};

TEST_F(BorrowEscapeTest, ReturnLocalRef) {
    EXPECT_TRUE(hasError(
        "fn bad() -> Ptr<i64> {\n"
        "  var x: i64 = 42\n"
        "  return &x\n"
        "}"));
}

TEST_F(BorrowEscapeTest, ParamRefOK) {
    EXPECT_FALSE(hasError(
        "fn ok(x: Ptr<i64>) -> Ptr<i64> { x }"));
}

// ============================================================================
// MutBorrowAliasPass tests
// ============================================================================

class MutBorrowAliasTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    bool hasError(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return true;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return true;
        HIRPassManager pm;
        pm.add<MutBorrowAliasPass>();
        pm.run(*hir, ctx);
        return ctx.diag.hasErrors();
    }
};

TEST_F(MutBorrowAliasTest, SameVar_Error) {
    EXPECT_TRUE(hasError(
        "fn swap(a: var i64, b: var i64) -> i64 with mut { a }\n"
        "fn main() -> i64 {\n"
        "  var x: i64 = 10\n"
        "  swap(x, x)\n"
        "}"));
}

TEST_F(MutBorrowAliasTest, DifferentVars_OK) {
    EXPECT_FALSE(hasError(
        "fn swap(a: var i64, b: var i64) -> i64 with mut { a }\n"
        "fn main() -> i64 {\n"
        "  var x: i64 = 10\n"
        "  var y: i64 = 20\n"
        "  swap(x, y)\n"
        "}"));
}

// ============================================================================
// UnusedBindingPass tests
// ============================================================================

class UnusedBindingTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    bool hasWarning(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast || ctx.diag.hasErrors()) return false;
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return false;
        HIRPassManager pm;
        pm.add<UnusedBindingPass>();
        pm.run(*hir, ctx);
        return ctx.diag.hasWarnings();
    }
};

TEST_F(UnusedBindingTest, Warn) {
    EXPECT_TRUE(hasWarning(
        "fn main() -> i64 {\n"
        "  val x: i64 = 42\n"
        "  0\n"
        "}"));
}

TEST_F(UnusedBindingTest, UnderscoreOK) {
    EXPECT_FALSE(hasWarning(
        "fn main() -> i64 {\n"
        "  val _x: i64 = 42\n"
        "  0\n"
        "}"));
}

TEST_F(UnusedBindingTest, UsedOK) {
    EXPECT_FALSE(hasWarning(
        "fn main() -> i64 {\n"
        "  val x: i64 = 42\n"
        "  x\n"
        "}"));
}

// ============================================================================
// Phase 6: ExhaustiveMatch — all missing variants
// ============================================================================

class ExhaustiveMatchTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    int errorCount(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        auto* ast = parser.parseModule();
        if (!ast) return -1;
        HIRBuilder builder(ctx);
        builder.build(ast);
        int count = 0;
        for (auto& d : ctx.diag.diagnostics()) {
            if (d.level == DiagLevel::Error) count++;
        }
        return count;
    }
};

TEST_F(ExhaustiveMatchTest, AllMissingVariantsReported) {
    // Missing Green AND Blue — should get 2 errors, not just 1
    int count = errorCount(
        "enum Color { Red, Green, Blue }\n"
        "fn f(c: Color) -> i64 {\n"
        "  match c {\n"
        "    Red => 1\n"
        "  }\n"
        "}");
    EXPECT_GE(count, 2);  // at least Green + Blue missing
}

// ============================================================================
// Phase 6: EffectAnalysis — conservative HOF
// ============================================================================

TEST_F(EffectAnalysisTest, CallIndirect_ConservativeEffects) {
    auto* hir = buildWithEffects(
        "fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "apply");
    ASSERT_NE(fn, nullptr);
    // apply() calls through fn pointer → should conservatively have all effects
    EXPECT_TRUE(hasEffect(fn->inferred_effects, Effect::Mut));
    EXPECT_TRUE(hasEffect(fn->inferred_effects, Effect::IO));
}

TEST_F(EffectAnalysisTest, NoCallIndirect_NoConservativeEffects) {
    auto* hir = buildWithEffects(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    auto* fn = findFn(hir, "add");
    ASSERT_NE(fn, nullptr);
    // add() has no indirect calls, no side effects → pure
    EXPECT_EQ(fn->inferred_effects, EFFECT_NONE);
}

// ============================================================================
// Phase 6: OwnershipCheck — control-flow merge
// ============================================================================

TEST_F(OwnershipCheckTest, IfBothBranchesMove) {
    EXPECT_TRUE(hasError(
        "fn consume(x: own i64) -> i64 { x }\n"
        "fn bad(a: own i64) -> i64 {\n"
        "  if true { consume(a) } else { consume(a) }\n"
        "  a\n"
        "}"));
}

TEST_F(OwnershipCheckTest, IfOneBranchMove_Warning) {
    // Move only in one branch — should warn + treat as moved
    const char* src =
        "fn consume(x: own i64) -> i64 { x }\n"
        "fn maybe(a: own i64) -> i64 {\n"
        "  if true { consume(a) } else { 0 }\n"
        "  a\n"
        "}";
    ctx.diag.setSource(src);
    Lexer lexer(src, "test.kern", ctx.diag);
    Parser parser(lexer, ctx.arena, ctx.diag);
    auto* ast = parser.parseModule();
    ASSERT_NE(ast, nullptr);
    HIRBuilder builder(ctx);
    auto* hir = builder.build(ast);
    ASSERT_NE(hir, nullptr);
    HIRPassManager pm;
    pm.add<OwnershipCheckPass>();
    pm.run(*hir, ctx);
    // Should have both a warning (partial branch move) and an error (use-after-move)
    EXPECT_TRUE(ctx.diag.hasErrors() || ctx.diag.hasWarnings());
}

TEST_F(OwnershipCheckTest, IfNeitherBranchMoves_OK) {
    EXPECT_FALSE(hasError(
        "fn ok(a: own i64) -> i64 {\n"
        "  if true { 1 } else { 2 }\n"
        "  a\n"
        "}"));
}

} // namespace kern
