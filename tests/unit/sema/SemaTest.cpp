#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/sema/TypeChecker.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

static bool checkSource(std::string src, std::string* errors = nullptr) {
    // src is owned here, so string_views into it remain valid
    DiagnosticEngine diag;
    Arena arena;
    Lexer lexer(src, "test.kern", diag);
    Parser parser(lexer, arena, diag);
    Module* mod = parser.parseModule();
    if (diag.hasErrors()) {
        if (errors) {
            std::ostringstream out;
            diag.printAll(out);
            *errors = out.str();
        }
        return false;
    }

    TypeChecker tc(diag);
    tc.check(mod);
    if (errors) {
        std::ostringstream out;
        diag.printAll(out);
        *errors = out.str();
    }
    return !diag.hasErrors();
}

// ===== Existing tests =====

TEST(SemaTest, ValidSimpleFunction) {
    EXPECT_TRUE(checkSource("fn main() -> i64 { 42 }"));
}

TEST(SemaTest, ValidFibonacci) {
    EXPECT_TRUE(checkSource(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n }\n"
        "    else { fib(n - 1) + fib(n - 2) }\n"
        "}\n"
        "fn main() -> i64 { fib(10) }"
    ));
}

TEST(SemaTest, TypeMismatchReturn) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> i64 { true }", &errors));
    EXPECT_NE(errors.find("type"), std::string::npos);
}

TEST(SemaTest, UndeclaredFunction) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> i64 { unknown(1) }", &errors));
    EXPECT_NE(errors.find("undeclared"), std::string::npos);
}

TEST(SemaTest, UndeclaredVariable) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> i64 { x }", &errors));
    EXPECT_NE(errors.find("undeclared"), std::string::npos);
}

TEST(SemaTest, WrongArgCount) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1) }",
        &errors
    ));
    EXPECT_NE(errors.find("expects"), std::string::npos);
}

TEST(SemaTest, ArithmeticOnBool) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> i64 { true + false }", &errors));
    EXPECT_NE(errors.find("i64"), std::string::npos);
}

// ===== New TDD tests =====

// --- Valid: bool return ---
TEST(SemaTest, ValidBoolReturn) {
    EXPECT_TRUE(checkSource("fn main() -> bool { true }"));
}

// --- Valid: comparison returns bool ---
TEST(SemaTest, ValidComparison) {
    EXPECT_TRUE(checkSource("fn main() -> bool { 1 < 2 }"));
}

// --- Valid: logical operators ---
TEST(SemaTest, ValidLogical) {
    EXPECT_TRUE(checkSource("fn main() -> bool { true and false }"));
    EXPECT_TRUE(checkSource("fn main() -> bool { true or false }"));
}

// --- Valid: not operator ---
TEST(SemaTest, ValidNot) {
    EXPECT_TRUE(checkSource("fn main() -> bool { not true }"));
}

// --- Valid: negation ---
TEST(SemaTest, ValidNegation) {
    EXPECT_TRUE(checkSource("fn main() -> i64 { -42 }"));
}

// --- Valid: val binding ---
TEST(SemaTest, ValidValBinding) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    x\n"
        "}"
    ));
}

// --- Valid: val binding used in arithmetic ---
TEST(SemaTest, ValidValBindingArith) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    val y: i64 = 20\n"
        "    x + y\n"
        "}"
    ));
}

// --- Error: val type mismatch ---
TEST(SemaTest, ValTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i64 = true\n"
        "    x\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("mismatch"), std::string::npos);
}

// --- Error: if condition must be bool ---
TEST(SemaTest, IfConditionNotBool) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 { if 1 { 42 } else { 0 } }", &errors
    ));
    EXPECT_NE(errors.find("bool"), std::string::npos);
}

// --- Error: if branches type mismatch ---
TEST(SemaTest, IfBranchTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 { if true { 42 } else { true } }", &errors
    ));
    EXPECT_NE(errors.find("different types"), std::string::npos);
}

// --- Error: logical and on i64 ---
TEST(SemaTest, LogicalAndOnI64) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> bool { 1 and 2 }", &errors));
    EXPECT_NE(errors.find("bool"), std::string::npos);
}

// --- Error: not on i64 ---
TEST(SemaTest, NotOnI64) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> bool { not 42 }", &errors));
    EXPECT_NE(errors.find("bool"), std::string::npos);
}

// --- Error: negation on bool ---
TEST(SemaTest, NegOnBool) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> i64 { -true }", &errors));
    EXPECT_NE(errors.find("i64"), std::string::npos);
}

// --- Error: too many args ---
TEST(SemaTest, TooManyArgs) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1, 2, 3) }",
        &errors
    ));
    EXPECT_NE(errors.find("expects"), std::string::npos);
}

// --- Error: arg type mismatch ---
TEST(SemaTest, ArgTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { f(true) }",
        &errors
    ));
    EXPECT_NE(errors.find("mismatch"), std::string::npos);
}

// --- Valid: forward reference (function defined after use) ---
TEST(SemaTest, ForwardReference) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 { helper(10) }\n"
        "fn helper(x: i64) -> i64 { x + 1 }"
    ));
}

// --- Valid: recursive function ---
TEST(SemaTest, RecursiveFunction) {
    EXPECT_TRUE(checkSource(
        "fn countdown(n: i64) -> i64 {\n"
        "    if n <= 0 { 0 } else { countdown(n - 1) }\n"
        "}"
    ));
}

// --- Valid: multiple functions calling each other ---
TEST(SemaTest, MutualCalls) {
    EXPECT_TRUE(checkSource(
        "fn double_val(x: i64) -> i64 { x + x }\n"
        "fn quad(x: i64) -> i64 { double_val(double_val(x)) }\n"
        "fn main() -> i64 { quad(5) }"
    ));
}

// --- Error: comparison of different types ---
TEST(SemaTest, ComparisonTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> bool { 1 == true }",
        &errors
    ));
    EXPECT_NE(errors.find("same-type"), std::string::npos);
}
