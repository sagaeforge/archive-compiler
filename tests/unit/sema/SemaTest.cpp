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
    EXPECT_NE(errors.find("integer"), std::string::npos);
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
    EXPECT_NE(errors.find("signed"), std::string::npos);
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

// ===== M1 coverage improvement tests =====

// --- Comparison Gt ---
TEST(SemaTest, ComparisonGt) {
    EXPECT_TRUE(checkSource("fn main() -> bool { 1 > 2 }"));
}

// --- Comparison GtEq ---
TEST(SemaTest, ComparisonGtEq) {
    EXPECT_TRUE(checkSource("fn main() -> bool { 1 >= 2 }"));
}

// --- Comparison NotEq ---
TEST(SemaTest, ComparisonNotEq) {
    EXPECT_TRUE(checkSource("fn main() -> bool { 1 != 2 }"));
}

// --- Unknown type name resolves to Error ---
TEST(SemaTest, UnknownTypeNameM1) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: Foo = 42\n"
        "    x\n"
        "}", &errors
    ));
}

// --- Error: or on i64 ---
TEST(SemaTest, LogicalOrOnI64) {
    std::string errors;
    EXPECT_FALSE(checkSource("fn main() -> bool { 1 or 2 }", &errors));
    EXPECT_NE(errors.find("bool"), std::string::npos);
}

// --- Valid: var declaration ---
TEST(SemaTest, VarDeclValid) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    ));
}

// --- Error: var type mismatch ---
TEST(SemaTest, VarDeclTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    var x: i64 = true\n"
        "    x\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("var type mismatch"), std::string::npos);
}

// ===== M2.1: Extended type system tests =====

// --- i32 function declaration and return ---
TEST(SemaTest, I32FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_i32(x: i32) -> i32 { x }"
    ));
}

// --- i16 function ---
TEST(SemaTest, I16FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_i16(x: i16) -> i16 { x }"
    ));
}

// --- i8 function ---
TEST(SemaTest, I8FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_i8(x: i8) -> i8 { x }"
    ));
}

// --- u8 function ---
TEST(SemaTest, U8FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_u8(x: u8) -> u8 { x }"
    ));
}

// --- u16 function ---
TEST(SemaTest, U16FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_u16(x: u16) -> u16 { x }"
    ));
}

// --- u32 function ---
TEST(SemaTest, U32FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_u32(x: u32) -> u32 { x }"
    ));
}

// --- u64 function ---
TEST(SemaTest, U64FunctionReturn) {
    EXPECT_TRUE(checkSource(
        "fn get_u64(x: u64) -> u64 { x }"
    ));
}

// --- Same-type i32 arithmetic ---
TEST(SemaTest, I32Arithmetic) {
    EXPECT_TRUE(checkSource(
        "fn add32(a: i32, b: i32) -> i32 { a + b }"
    ));
}

// --- Same-type u64 arithmetic ---
TEST(SemaTest, U64Arithmetic) {
    EXPECT_TRUE(checkSource(
        "fn add_u64(a: u64, b: u64) -> u64 { a + b }"
    ));
}

// --- Error: mixed type arithmetic (i32 + i64) ---
TEST(SemaTest, MixedTypeArithmeticError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(a: i32, b: i64) -> i32 { a + b }",
        &errors
    ));
    EXPECT_NE(errors.find("i32"), std::string::npos);
    EXPECT_NE(errors.find("i64"), std::string::npos);
}

// --- Error: unsigned negation ---
TEST(SemaTest, UnsignedNegationError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(x: u32) -> u32 { -x }",
        &errors
    ));
    EXPECT_NE(errors.find("signed"), std::string::npos);
}

// --- Error: i64 literal assigned to i32 val ---
TEST(SemaTest, LiteralToI32ValError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i32 = 42\n"
        "    0\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("mismatch"), std::string::npos);
}

// --- Error: unknown type name ---
TEST(SemaTest, UnknownTypeName) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i128 { 0 }",
        &errors
    ));
    EXPECT_NE(errors.find("unknown type"), std::string::npos);
}

// --- Error: float type unknown ---
TEST(SemaTest, FloatTypeUnknown) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> float { 0 }",
        &errors
    ));
    EXPECT_NE(errors.find("unknown type"), std::string::npos);
}

// --- Signed negation on i32 ---
TEST(SemaTest, SignedI32Negation) {
    EXPECT_TRUE(checkSource(
        "fn neg32(x: i32) -> i32 { -x }"
    ));
}

// --- u64 negation error ---
TEST(SemaTest, U64NegationError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(x: u64) -> u64 { -x }",
        &errors
    ));
    EXPECT_NE(errors.find("signed"), std::string::npos);
}

// --- i32 comparison returns bool ---
TEST(SemaTest, I32ComparisonReturnsBool) {
    EXPECT_TRUE(checkSource(
        "fn less32(a: i32, b: i32) -> bool { a < b }"
    ));
}

// --- Cross-type function call error ---
TEST(SemaTest, CrossTypeCallError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn take_i32(x: i32) -> i32 { x }\n"
        "fn main() -> i64 { take_i32(42) }",
        &errors
    ));
    EXPECT_NE(errors.find("mismatch"), std::string::npos);
}

// --- Chained i32 arithmetic ---
TEST(SemaTest, ChainedI32Arithmetic) {
    EXPECT_TRUE(checkSource(
        "fn calc(a: i32, b: i32, c: i32) -> i32 { a + b * c - a }"
    ));
}
