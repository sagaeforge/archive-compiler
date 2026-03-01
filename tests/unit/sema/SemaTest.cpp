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

    TypeChecker tc(diag, &arena);
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
    EXPECT_NE(errors.find("numeric"), std::string::npos);
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

// --- Valid: integer literal coerces to i32 ---
TEST(SemaTest, LiteralToI32ValOk) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i32 = 42\n"
        "    0\n"
        "}"
    ));
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

// --- Error: 'float' is not a valid type (use f32/f64) ---
TEST(SemaTest, FloatTypeUnknown) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> float { 0 }",
        &errors
    ));
    EXPECT_NE(errors.find("unknown type"), std::string::npos);
}

// ===== M3.2: Float type system =====

// --- f64 return ---
TEST(SemaTest, F64Return) {
    EXPECT_TRUE(checkSource("fn main() -> f64 { 3.14 }"));
}

// --- f32 return ---
TEST(SemaTest, F32Return) {
    EXPECT_TRUE(checkSource("fn main() -> f32 { 3.14f }"));
}

// --- f64 arithmetic ---
TEST(SemaTest, F64Arith) {
    EXPECT_TRUE(checkSource(
        "fn add_f64(a: f64, b: f64) -> f64 { a + b }"
    ));
}

// --- float negation ---
TEST(SemaTest, FloatNeg) {
    EXPECT_TRUE(checkSource("fn main() -> f64 { -3.14 }"));
}

// --- float comparison returns bool ---
TEST(SemaTest, FloatCmp) {
    EXPECT_TRUE(checkSource(
        "fn cmp(a: f64, b: f64) -> bool { a < b }"
    ));
}

// --- float + int mix error ---
TEST(SemaTest, FloatIntMixError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(a: f64, b: i64) -> f64 { a + b }",
        &errors
    ));
    EXPECT_NE(errors.find("arithmetic"), std::string::npos);
}

// --- f32 + f64 mix error ---
TEST(SemaTest, F32F64MixError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(a: f32, b: f64) -> f64 { a + b }",
        &errors
    ));
    EXPECT_NE(errors.find("arithmetic"), std::string::npos);
}

// --- f64 val binding ---
TEST(SemaTest, F64ValBinding) {
    EXPECT_TRUE(checkSource(
        "fn main() -> f64 {\n"
        "    val x: f64 = 3.14\n"
        "    x\n"
        "}"
    ));
}

// --- f64 literal context: 3.14 adopts f32 with ctx ---
TEST(SemaTest, FloatLiteralContextF32) {
    EXPECT_TRUE(checkSource(
        "fn main() -> f32 { 3.14 }"
    ));
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

// --- Cross-type function call: literal coerces to param type ---
TEST(SemaTest, CrossTypeCallLiteralCoerce) {
    EXPECT_TRUE(checkSource(
        "fn take_i32(x: i32) -> i32 { x }\n"
        "fn main() -> i32 { take_i32(42) }"
    ));
}

// --- Chained i32 arithmetic ---
TEST(SemaTest, ChainedI32Arithmetic) {
    EXPECT_TRUE(checkSource(
        "fn calc(a: i32, b: i32, c: i32) -> i32 { a + b * c - a }"
    ));
}

// --- >6 parameters error ---
TEST(SemaTest, TooManyParams) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn many(a: i64, b: i64, c: i64, d: i64, e: i64, f: i64, g: i64) -> i64 { a }",
        &errors
    ));
    EXPECT_NE(errors.find("maximum is 6"), std::string::npos);
}

// --- 6 parameters OK ---
TEST(SemaTest, SixParamsOk) {
    EXPECT_TRUE(checkSource(
        "fn six(a: i64, b: i64, c: i64, d: i64, e: i64, f: i64) -> i64 { a + b + c + d + e + f }"
    ));
}

// --- var reassignment valid ---
TEST(SemaTest, VarReassignValid) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x = 20\n"
        "    x\n"
        "}"
    ));
}

// --- val reassignment error ---
TEST(SemaTest, ValReassignError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    x = 20\n"
        "    x\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("immutable"), std::string::npos);
}

// --- var reassignment type mismatch ---
TEST(SemaTest, VarReassignTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x = true\n"
        "    x\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("assignment type mismatch"), std::string::npos);
}

// --- reassignment to undeclared variable ---
TEST(SemaTest, ReassignUndeclaredError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    x = 20\n"
        "    0\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("undeclared"), std::string::npos);
}

// ===== M3.1: Context-based integer literal inference =====

// --- i8 literal out of range ---
TEST(SemaTest, IntLiteralOutOfRangeI8) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i8 = 300\n"
        "    0\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("out of range"), std::string::npos);
}

// --- u8 literal out of range ---
TEST(SemaTest, IntLiteralU8OutOfRange) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: u8 = 256\n"
        "    0\n"
        "}", &errors
    ));
    EXPECT_NE(errors.find("out of range"), std::string::npos);
}

// --- fn returning i32 literal ---
TEST(SemaTest, FnReturnI32Literal) {
    EXPECT_TRUE(checkSource(
        "fn get() -> i32 { 42 }"
    ));
}

// --- call arg: i32 literal ---
TEST(SemaTest, IntLiteralToI32Param) {
    EXPECT_TRUE(checkSource(
        "fn f(x: i32) -> i32 { x }\n"
        "fn main() -> i32 { f(10) }"
    ));
}

// --- BinOp propagates context: val x: i32 = 3 + 4 ---
TEST(SemaTest, BinOpContextPropagation) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    val x: i32 = 3 + 4\n"
        "    0\n"
        "}"
    ));
}

// --- Negative literal coerces to i8 ---
TEST(SemaTest, NegativeLiteralI8) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i8 { -1 }"
    ));
}

// --- Negative literal out of range for i8 ---
TEST(SemaTest, NegativeLiteralOutOfRangeI8) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i8 { -129 }", &errors
    ));
    EXPECT_NE(errors.find("out of range"), std::string::npos);
}

// --- var assignment context: literal coerces ---
TEST(SemaTest, VarAssignLiteralCoerce) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    var x: i32 = 0\n"
        "    x = 42\n"
        "    0\n"
        "}"
    ));
}

// --- return statement context ---
TEST(SemaTest, ReturnLiteralCoerce) {
    EXPECT_TRUE(checkSource(
        "fn f() -> i32 { return 42 }"
    ));
}

// ===== M4a: Pipe type check =====

TEST(SemaTest, PipeTypeCheck) {
    EXPECT_TRUE(checkSource(
        "fn double(x: i64) -> i64 { x * 2 }\n"
        "fn main() -> i64 { 21 |> double }"
    ));
}

TEST(SemaTest, PipeTypeError) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(x: bool) -> bool { x }\n"
        "fn main() -> bool { 42 |> f }",
        &errors
    ));
    EXPECT_NE(errors.find("mismatch"), std::string::npos);
}

// ===== M4a: Intrinsic type check =====

TEST(SemaTest, IntrinsicTypeCheck) {
    EXPECT_TRUE(checkSource(
        "fn halt() -> Unit = intrinsic\n"
        "fn main() -> i64 { 42 }"
    ));
}

TEST(SemaTest, IntrinsicReturnType) {
    // Calling an intrinsic should use its declared return type
    EXPECT_TRUE(checkSource(
        "fn read_port(port: u16) -> u8 = intrinsic\n"
        "fn main() -> u8 { read_port(0) }"
    ));
}

TEST(SemaTest, IntrinsicWrongArgs) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn halt() -> Unit = intrinsic\n"
        "fn main() -> Unit { halt(42) }",
        &errors
    ));
    EXPECT_NE(errors.find("expects"), std::string::npos);
}

// ===== M4b: Match expression type checking =====

TEST(SemaTest, ValidIntMatch) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 { match 1 { 0 => 10, _ => 20 } }"
    ));
}

TEST(SemaTest, AllArmsSameType) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 { match 1 { 0 => 10, 1 => 20, _ => 30 } }"
    ));
}

TEST(SemaTest, ArmTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 { match 1 { 0 => 10, _ => true } }",
        &errors
    ));
    EXPECT_NE(errors.find("match arm type mismatch"), std::string::npos);
}

TEST(SemaTest, NonExhaustiveInt) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 { match 1 { 0 => 10, 1 => 20 } }",
        &errors
    ));
    EXPECT_NE(errors.find("non-exhaustive"), std::string::npos);
}

TEST(SemaTest, ExhaustiveBool) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 { match true { true => 1, false => 0 } }"
    ));
}

TEST(SemaTest, NonExhaustiveBool) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 { match true { true => 1 } }",
        &errors
    ));
    EXPECT_NE(errors.find("non-exhaustive"), std::string::npos);
}

TEST(SemaTest, GuardMustBeBool) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn main() -> i64 { match 1 { x if 42 => x, _ => 0 } }",
        &errors
    ));
    EXPECT_NE(errors.find("bool"), std::string::npos);
}

TEST(SemaTest, VariableBindingScope) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 { match 42 { x => x } }"
    ));
}

TEST(SemaTest, MatchContextPropagation) {
    // Context type should propagate to match arm bodies
    EXPECT_TRUE(checkSource(
        "fn id(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { match 1 { 0 => id(10), _ => id(20) } }"
    ));
}

TEST(SemaTest, MatchNested) {
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 {\n"
        "    match 1 {\n"
        "        0 => match 2 { 0 => 10, _ => 20 },\n"
        "        _ => 30\n"
        "    }\n"
        "}"
    ));
}

TEST(SemaTest, FnLevelPatternTypeCheck) {
    EXPECT_TRUE(checkSource(
        "fn fib(0) -> i64 { 0 }\n"
        "fn fib(1) -> i64 { 1 }\n"
        "fn fib(n: i64) -> i64 { fib(n - 1) + fib(n - 2) }\n"
        "fn main() -> i64 { fib(10) }"
    ));
}

TEST(SemaTest, MatchVariableScopedToArm) {
    // Variable bound in one arm should not leak to others
    EXPECT_TRUE(checkSource(
        "fn main() -> i64 { match 1 { x => x, _ => 0 } }"
    ));
}

// ===== M5a: struct type checking =====

TEST(SemaTest, StructBasicValid) {
    EXPECT_TRUE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, y: 2 }\n"
        "    p.x\n"
        "}"
    ));
}

TEST(SemaTest, StructFieldTypeMismatch) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: true, y: 2 }\n"
        "    p.x\n"
        "}", &errors));
    EXPECT_NE(errors.find("field 'x' expects i64"), std::string::npos);
}

TEST(SemaTest, StructMissingField) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1 }\n"
        "    p.x\n"
        "}", &errors));
    EXPECT_NE(errors.find("expects 2 fields, got 1"), std::string::npos);
}

TEST(SemaTest, StructExtraField) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, y: 2, z: 3 }\n"
        "    p.x\n"
        "}", &errors));
    EXPECT_NE(errors.find("expects 2 fields, got 3"), std::string::npos);
}

TEST(SemaTest, StructNonexistentField) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, z: 2 }\n"
        "    p.x\n"
        "}", &errors));
    EXPECT_NE(errors.find("no field named 'z'"), std::string::npos);
}

TEST(SemaTest, StructFieldAccessNonexistent) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, y: 2 }\n"
        "    p.z\n"
        "}", &errors));
    EXPECT_NE(errors.find("no field named 'z'"), std::string::npos);
}

TEST(SemaTest, StructValBindingMutation) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, y: 2 }\n"
        "    p.x = 42\n"
        "    p.x\n"
        "}", &errors));
    EXPECT_NE(errors.find("immutable binding"), std::string::npos);
}

TEST(SemaTest, StructValFieldMutation) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    var p: Point = Point { x: 1, y: 2 }\n"
        "    p.x = 42\n"
        "    p.x\n"
        "}", &errors));
    EXPECT_NE(errors.find("immutable field"), std::string::npos);
}

TEST(SemaTest, StructVarBindingVarField) {
    EXPECT_TRUE(checkSource(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn main() -> i64 {\n"
        "    var p: Point = Point { x: 1, y: 2 }\n"
        "    p.x = 42\n"
        "    p.x\n"
        "}"
    ));
}

TEST(SemaTest, StructNested) {
    EXPECT_TRUE(checkSource(
        "struct Inner { v: i64 }\n"
        "struct Outer { inner: Inner }\n"
        "fn main() -> i64 {\n"
        "    val o: Outer = Outer { inner: Inner { v: 42 } }\n"
        "    o.inner.v\n"
        "}"
    ));
}

TEST(SemaTest, StructParam) {
    EXPECT_TRUE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn sum(p: Point) -> i64 { p.x + p.y }\n"
        "fn main() -> i64 { sum(Point { x: 10, y: 20 }) }"
    ));
}

TEST(SemaTest, StructReturn) {
    EXPECT_TRUE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn make() -> Point { Point { x: 1, y: 2 } }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = make()\n"
        "    p.x + p.y\n"
        "}"
    ));
}

// ===== M5b: Enum + Union type checking =====

TEST(SemaTest, EnumDeclAndAccess) {
    EXPECT_TRUE(checkSource(
        "enum Color { Red, Green, Blue }\n"
        "fn main() -> Color { Color.Red }"));
}

TEST(SemaTest, EnumParam) {
    EXPECT_TRUE(checkSource(
        "enum Color { Red, Green, Blue }\n"
        "fn pick(c: Color) -> Color { c }\n"
        "fn main() -> Color { pick(Color.Green) }"));
}

TEST(SemaTest, EnumMatchExhaustive) {
    EXPECT_TRUE(checkSource(
        "enum Color { Red, Green, Blue }\n"
        "fn to_int(c: Color) -> i64 {\n"
        "    match c { Red => 0, Green => 1, Blue => 2 }\n"
        "}\n"
        "fn main() -> i64 { to_int(Color.Red) }"));
}

TEST(SemaTest, EnumMatchWithWildcard) {
    EXPECT_TRUE(checkSource(
        "enum Color { Red, Green, Blue }\n"
        "fn is_red(c: Color) -> i64 {\n"
        "    match c { Red => 1, _ => 0 }\n"
        "}\n"
        "fn main() -> i64 { is_red(Color.Red) }"));
}

TEST(SemaTest, ErrorEnumNonExhaustive) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "enum Color { Red, Green, Blue }\n"
        "fn bad(c: Color) -> i64 {\n"
        "    match c { Red => 0, Green => 1 }\n"
        "}\n"
        "fn main() -> i64 { bad(Color.Red) }", &errors));
    EXPECT_NE(errors.find("non-exhaustive"), std::string::npos);
}

TEST(SemaTest, ErrorEnumUnknownVariant) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "enum Color { Red, Green, Blue }\n"
        "fn main() -> Color { Color.Yellow }", &errors));
    EXPECT_NE(errors.find("no variant"), std::string::npos);
}

TEST(SemaTest, UnionDeclAndCreate) {
    EXPECT_TRUE(checkSource(
        "union Option { Some(i64), None }\n"
        "fn main() -> Option { Option::Some(42) }"));
}

TEST(SemaTest, UnionEmptyVariant) {
    EXPECT_TRUE(checkSource(
        "union Option { Some(i64), None }\n"
        "fn main() -> Option { Option::None }"));
}

TEST(SemaTest, UnionMatchExhaustive) {
    EXPECT_TRUE(checkSource(
        "union Option { Some(i64), None }\n"
        "fn value(o: Option) -> i64 {\n"
        "    match o { Some(x) => x, None => 0 }\n"
        "}\n"
        "fn main() -> i64 { value(Option::Some(42)) }"));
}

TEST(SemaTest, ErrorUnionNonExhaustive) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "union Option { Some(i64), None }\n"
        "fn bad(o: Option) -> i64 {\n"
        "    match o { Some(x) => x }\n"
        "}\n"
        "fn main() -> i64 { bad(Option::Some(42)) }", &errors));
    EXPECT_NE(errors.find("non-exhaustive"), std::string::npos);
}

TEST(SemaTest, ErrorUnionPayloadMissing) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "union Option { Some(i64), None }\n"
        "fn main() -> Option { Option::Some }", &errors));
    EXPECT_NE(errors.find("requires a payload"), std::string::npos);
}

TEST(SemaTest, ErrorUnionUnexpectedPayload) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "union Option { Some(i64), None }\n"
        "fn main() -> Option { Option::None(42) }", &errors));
    EXPECT_NE(errors.find("takes no payload"), std::string::npos);
}

// ===== M5c: Pointer type checking =====

TEST(SemaTest, PtrTypeResolves) {
    EXPECT_TRUE(checkSource(
        "fn f(p: Ptr<i64>) -> i64 { *p }"));
}

TEST(SemaTest, PtrVarTypeResolves) {
    EXPECT_TRUE(checkSource(
        "fn f(p: Ptr<var i64>) -> Unit { *p = 42 }"));
}

TEST(SemaTest, AddrOfFromVal) {
    EXPECT_TRUE(checkSource(
        "fn f() -> Ptr<i64> {\n"
        "    val x: i64 = 42\n"
        "    &x\n"
        "}"));
}

TEST(SemaTest, AddrOfFromVar) {
    EXPECT_TRUE(checkSource(
        "fn f() -> Ptr<i64> {\n"
        "    var x: i64 = 42\n"
        "    &x\n"
        "}"));
}

TEST(SemaTest, AddrOfVarFromVar) {
    EXPECT_TRUE(checkSource(
        "fn f() -> Ptr<var i64> {\n"
        "    var x: i64 = 42\n"
        "    &var x\n"
        "}"));
}

TEST(SemaTest, ErrorAddrOfVarFromVal) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f() -> Ptr<var i64> {\n"
        "    val x: i64 = 42\n"
        "    &var x\n"
        "}", &errors));
    EXPECT_NE(errors.find("'&var' requires a 'var' binding"), std::string::npos);
}

TEST(SemaTest, DerefReadPtrOk) {
    EXPECT_TRUE(checkSource(
        "fn f(p: Ptr<i64>) -> i64 { *p }"));
}

TEST(SemaTest, ErrorDerefWritePtrReadonly) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(p: Ptr<i64>) -> Unit { *p = 42 }", &errors));
    EXPECT_NE(errors.find("cannot write through 'Ptr<T>'"), std::string::npos);
}

TEST(SemaTest, DerefWritePtrVarOk) {
    EXPECT_TRUE(checkSource(
        "fn f(p: Ptr<var i64>) -> Unit { *p = 42 }"));
}

TEST(SemaTest, PtrStructFieldAccess) {
    EXPECT_TRUE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn f(p: Ptr<Point>) -> i64 { (*p).x }"));
}

TEST(SemaTest, ErrorDerefFieldAssignReadonly) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn f(p: Ptr<Point>) -> Unit { (*p).x = 42 }", &errors));
    EXPECT_NE(errors.find("cannot write through 'Ptr<T>'"), std::string::npos);
}

TEST(SemaTest, DerefFieldAssignPtrVarOk) {
    EXPECT_TRUE(checkSource(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn f(p: Ptr<var Point>) -> Unit { (*p).x = 42 }"));
}

TEST(SemaTest, ErrorDerefFieldAssignImmutableField) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "struct Point { x: i64, y: i64 }\n"
        "fn f(p: Ptr<var Point>) -> Unit { (*p).x = 42 }", &errors));
    EXPECT_NE(errors.find("cannot assign to immutable field"), std::string::npos);
}

TEST(SemaTest, PtrParamPassing) {
    EXPECT_TRUE(checkSource(
        "fn read_val(p: Ptr<i64>) -> i64 { *p }\n"
        "fn main() -> i64 {\n"
        "    val x: i64 = 42\n"
        "    read_val(&x)\n"
        "}"));
}

TEST(SemaTest, PtrVarParamPassing) {
    EXPECT_TRUE(checkSource(
        "fn write_val(p: Ptr<var i64>) -> Unit { *p = 99 }\n"
        "fn main() -> i64 {\n"
        "    var x: i64 = 42\n"
        "    write_val(&var x)\n"
        "    x\n"
        "}"));
}

TEST(SemaTest, ErrorDerefNonPointer) {
    std::string errors;
    EXPECT_FALSE(checkSource(
        "fn f(x: i64) -> i64 { *x }", &errors));
    EXPECT_NE(errors.find("cannot dereference non-pointer type"), std::string::npos);
}
