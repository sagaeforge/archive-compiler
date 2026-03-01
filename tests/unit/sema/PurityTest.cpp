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

    TypeChecker tc(r.diag, &r.arena);
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

// --- Nested var inside if is detected ---
TEST(PurityTest, NestedVarInIf) {
    auto r = analyzePurity(
        "fn f(x: i64) -> i64 {\n"
        "    if x > 0 {\n"
        "        var y: i64 = x\n"
        "        y\n"
        "    } else { 0 }\n"
        "}"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureMut);
    EXPECT_TRUE(it->second.uses_var);
}

// --- Nested var inside nested block is detected ---
TEST(PurityTest, NestedVarInBlock) {
    auto r = analyzePurity(
        "fn f() -> i64 {\n"
        "    val x: i64 = {\n"
        "        var y: i64 = 10\n"
        "        y\n"
        "    }\n"
        "    x\n"
        "}"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureMut);
}

// --- Tail recursion detected for simple countdown ---
TEST(PurityTest, TailRecursiveCountdown) {
    auto r = analyzePurity(
        "fn countdown(n: i64) -> i64 {\n"
        "    if n <= 0 { 0 } else { countdown(n - 1) }\n"
        "}"
    );
    auto it = r.results.find("countdown");
    ASSERT_NE(it, r.results.end());
    EXPECT_TRUE(it->second.is_recursive);
    EXPECT_TRUE(it->second.is_tailrec);
}

// --- Non-tail recursion (fib adds after recursive call) ---
TEST(PurityTest, NonTailRecursiveFib) {
    auto r = analyzePurity(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    auto it = r.results.find("fib");
    ASSERT_NE(it, r.results.end());
    EXPECT_TRUE(it->second.is_recursive);
    EXPECT_FALSE(it->second.is_tailrec);
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

// ===== M4a: Intrinsic purity =====

TEST(PurityTest, IntrinsicPurity) {
    auto r = analyzePurity(
        "fn halt() -> Unit = intrinsic"
    );
    auto it = r.results.find("halt");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureIo);
}

TEST(PurityTest, IntrinsicPropagation) {
    auto r = analyzePurity(
        "fn write(port: u16, value: u8) -> Unit = intrinsic\n"
        "fn do_write() -> Unit { write(0, 0) }"
    );
    auto it = r.results.find("do_write");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureIo);
}

// ===== M4b: Match purity analysis =====

TEST(PurityTest, MatchPure) {
    auto r = analyzePurity(
        "fn f(n: i64) -> i64 { match n { 0 => 10, _ => 20 } }"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

TEST(PurityTest, MatchVarInArm) {
    auto r = analyzePurity(
        "fn f(n: i64) -> i64 { match n { x => x } }"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

TEST(PurityTest, MatchCalleeInArm) {
    auto r = analyzePurity(
        "fn io_fn() -> i64 = intrinsic\n"
        "fn f(n: i64) -> i64 { match n { 0 => io_fn(), _ => 0 } }"
    );
    auto it = r.results.find("f");
    ASSERT_NE(it, r.results.end());
    // io_fn is intrinsic (ImpureIo), propagated to f through match arm
    EXPECT_EQ(it->second.purity, Purity::ImpureIo);
}

// ===== M5a: struct purity tests =====

TEST(PurityTest, StructCreationIsPure) {
    auto r = analyzePurity(
        "struct Point { x: i64, y: i64 }\n"
        "fn make() -> Point { Point { x: 1, y: 2 } }"
    );
    auto it = r.results.find("make");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

TEST(PurityTest, StructFieldAssignIsImpureMut) {
    auto r = analyzePurity(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn mutate(p: Point) -> i64 {\n"
        "    var q: Point = p\n"
        "    q.x = 42\n"
        "    q.x\n"
        "}"
    );
    auto it = r.results.find("mutate");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::ImpureMut);
}

// ===== M5b: enum/union purity tests =====

TEST(PurityTest, EnumAccessIsPure) {
    auto r = analyzePurity(
        "enum Color { Red, Green, Blue }\n"
        "fn get_color() -> Color { Color.Red }"
    );
    auto it = r.results.find("get_color");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}

TEST(PurityTest, UnionCreationIsPure) {
    auto r = analyzePurity(
        "union Shape { Circle(i64), Square(i64) }\n"
        "fn make_circle(r: i64) -> Shape { Shape::Circle(r) }"
    );
    auto it = r.results.find("make_circle");
    ASSERT_NE(it, r.results.end());
    EXPECT_EQ(it->second.purity, Purity::Pure);
}
