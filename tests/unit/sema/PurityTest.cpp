#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/sema/TypeChecker.h"
#include "kern/sema/PurityChecker.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

struct PurityTestResult {
    std::string source;
    Arena arena;
    DiagnosticEngine diag;
    std::unordered_map<std::string_view, PurityResult> results;
    std::string warnings;
};

static PurityTestResult analyzePurity(std::string src) {
    PurityTestResult r;
    r.source = std::move(src);
    Lexer lexer(r.source, "test.kern", r.diag);
    Parser parser(lexer, r.arena, r.diag);
    Module* mod = parser.parseModule();
    EXPECT_FALSE(r.diag.hasErrors());

    TypeChecker tc(r.diag);
    tc.check(mod);
    EXPECT_FALSE(r.diag.hasErrors());

    PurityChecker pc(r.diag);
    r.results = pc.analyze(mod);

    std::ostringstream out;
    r.diag.printAll(out);
    r.warnings = out.str();

    return r;
}

// --- Pure function (arithmetic only) ---
TEST(PurityTest, PureArithmetic) {
    auto r = analyzePurity("fn add(a: i64, b: i64) -> i64 { a + b }");
    auto it = r.results.find("add");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
    EXPECT_FALSE(it->second.is_recursive);
}

// --- Function with var is ImpureMut ---
TEST(PurityTest, VarUsageImpureMut) {
    auto r = analyzePurity(
        "fn f() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureMut);
    EXPECT_TRUE(it->second.uses_var);
}

// --- ImpureMut does NOT propagate ---
TEST(PurityTest, ImpureMutDoesNotPropagate) {
    auto r = analyzePurity(
        "fn impure_fn() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}\n"
        "fn caller() -> i64 { impure_fn() }"
    );
    auto it = r.results.find("caller");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

// --- Recursive function detected ---
TEST(PurityTest, RecursiveDetected) {
    auto r = analyzePurity(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    auto it = r.results.find("fib");
    ASSERT_NE(it, r.results.end());
    EXPECT_TRUE(it->second.is_recursive);
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

// --- Simple countdown recursive ---
TEST(PurityTest, RecursiveCountdown) {
    auto r = analyzePurity(
        "fn countdown(n: i64) -> i64 {\n"
        "    if n <= 0 { 0 } else { countdown(n - 1) }\n"
        "}"
    );
    auto it = r.results.find("countdown");
    ASSERT_NE(it, r.results.end());
    EXPECT_TRUE(it->second.is_recursive);
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

// --- Non-recursive pure ---
TEST(PurityTest, NonRecursivePure) {
    auto r = analyzePurity(
        "fn double_val(x: i64) -> i64 { x + x }\n"
        "fn quad(x: i64) -> i64 { double_val(double_val(x)) }"
    );
    auto it = r.results.find("quad");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
    EXPECT_FALSE(it->second.is_recursive);
}

// --- Multiple functions, only one impure ---
TEST(PurityTest, MixedPurity) {
    auto r = analyzePurity(
        "fn pure_fn(x: i64) -> i64 { x + 1 }\n"
        "fn impure_fn() -> i64 {\n"
        "    var y: i64 = 5\n"
        "    y\n"
        "}\n"
        "fn main() -> i64 { pure_fn(impure_fn()) }"
    );
    auto pure_it = r.results.find("pure_fn");
    ASSERT_NE(pure_it, r.results.end());
    EXPECT_EQ(pure_it->second.purity, Purity::Pure);

    auto impure_it = r.results.find("impure_fn");
    ASSERT_NE(impure_it, r.results.end());
    EXPECT_EQ(impure_it->second.purity, Purity::ImpureMut);

    // main calls impure_fn (which is ImpureMut) — but mut doesn't propagate
    auto main_it = r.results.find("main");
    ASSERT_NE(main_it, r.results.end());
    EXPECT_EQ(main_it->second.purity, Purity::Pure);
}

// --- var warning message ---
TEST(PurityTest, VarWarningMessage) {
    auto r = analyzePurity(
        "fn f() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    );
    EXPECT_NE(r.warnings.find("impure(mut)"), std::string::npos);
    EXPECT_NE(r.warnings.find("val + recursion"), std::string::npos);
}

// --- Pure function with if ---
TEST(PurityTest, PureWithIf) {
    auto r = analyzePurity(
        "fn abs(x: i64) -> i64 {\n"
        "    if x < 0 { -x } else { x }\n"
        "}"
    );
    auto it = r.results.find("abs");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
    EXPECT_FALSE(it->second.is_recursive);
}

// --- Pure function with val (not var) ---
TEST(PurityTest, PureWithVal) {
    auto r = analyzePurity(
        "fn f() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    val y: i64 = 20\n"
        "    x + y\n"
        "}"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

// --- Recursive with var ---
TEST(PurityTest, RecursiveWithVar) {
    auto r = analyzePurity(
        "fn f(n: i64) -> i64 {\n"
        "    var x: i64 = n\n"
        "    if x <= 0 { 0 } else { f(x - 1) }\n"
        "}"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureMut);
    EXPECT_TRUE(it->second.is_recursive);
}

// --- Constant function is pure ---
TEST(PurityTest, ConstantPure) {
    auto r = analyzePurity("fn always42() -> i64 { 42 }");
    auto it = r.results.find("always42");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

// --- No var warning for val-only code ---
TEST(PurityTest, NoVarWarningForVal) {
    auto r = analyzePurity(
        "fn f() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    x\n"
        "}"
    );
    EXPECT_EQ(r.warnings.find("impure"), std::string::npos);
}

// --- Purity result count matches function count ---
TEST(PurityTest, ResultCountMatchesFnCount) {
    auto r = analyzePurity(
        "fn a() -> i64 { 1 }\n"
        "fn b() -> i64 { 2 }\n"
        "fn c() -> i64 { 3 }"
    );
    EXPECT_EQ(r.results.size(), 3u);
}
