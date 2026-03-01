#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/ir/IRBuilder.h"
#include "kern/codegen/CodeGen.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

static std::string generateAsm(std::string src) {
    DiagnosticEngine diag;
    Arena arena;
    Lexer lexer(src, "test.kern", diag);
    Parser parser(lexer, arena, diag);
    Module* mod = parser.parseModule();
    EXPECT_FALSE(diag.hasErrors());

    IRBuilder irBuilder;
    IRModule irMod = irBuilder.build(mod);

    std::ostringstream out;
    CodeGen codegen(out);
    codegen.emitModule(irMod);
    return out.str();
}

// ===== Existing tests =====

TEST(CodeGenTest, SimpleReturn) {
    std::string asm_code = generateAsm("fn main() -> i64 { 42 }");
    EXPECT_NE(asm_code.find("_main"), std::string::npos);
    EXPECT_NE(asm_code.find("mov"), std::string::npos);
    EXPECT_NE(asm_code.find("42"), std::string::npos);
    EXPECT_NE(asm_code.find("ret"), std::string::npos);
}

TEST(CodeGenTest, FibonacciAsm) {
    std::string asm_code = generateAsm(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n }\n"
        "    else { fib(n - 1) + fib(n - 2) }\n"
        "}\n"
        "fn main() -> i64 { fib(10) }"
    );
    EXPECT_NE(asm_code.find("_fib"), std::string::npos);
    EXPECT_NE(asm_code.find("_main"), std::string::npos);
    EXPECT_NE(asm_code.find("call _fib"), std::string::npos);
}

TEST(CodeGenTest, HasSectionText) {
    std::string asm_code = generateAsm("fn main() -> i64 { 0 }");
    EXPECT_NE(asm_code.find("section .text"), std::string::npos);
    EXPECT_NE(asm_code.find("global _main"), std::string::npos);
}

// ===== New TDD tests =====

// --- Arithmetic operations produce add/sub/imul/idiv ---
TEST(CodeGenTest, ArithmeticOps) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 { 1 + 2 * 3 - 4 }");
    EXPECT_NE(asm_code.find("add"), std::string::npos);
    EXPECT_NE(asm_code.find("sub"), std::string::npos);
    EXPECT_NE(asm_code.find("imul"), std::string::npos);
}

TEST(CodeGenTest, Division) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 { 84 / 2 }");
    EXPECT_NE(asm_code.find("idiv"), std::string::npos);
    // idiv requires cqo for sign extension
    EXPECT_NE(asm_code.find("cqo"), std::string::npos);
}

// --- Comparison generates cmp + set* ---
TEST(CodeGenTest, ComparisonLe) {
    std::string asm_code = generateAsm(
        "fn main() -> bool { 1 <= 2 }");
    EXPECT_NE(asm_code.find("cmp"), std::string::npos);
    EXPECT_NE(asm_code.find("setle"), std::string::npos);
}

TEST(CodeGenTest, ComparisonEq) {
    std::string asm_code = generateAsm(
        "fn main() -> bool { 1 == 2 }");
    EXPECT_NE(asm_code.find("cmp"), std::string::npos);
    EXPECT_NE(asm_code.find("sete"), std::string::npos);
}

// --- Negation: lowered to sub(0, x) ---
TEST(CodeGenTest, UnaryNeg) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 { -42 }");
    // -x is 0 - x, so we should see sub
    EXPECT_NE(asm_code.find("sub"), std::string::npos);
}

// --- Not: lowered to sub(1, x) ---
TEST(CodeGenTest, UnaryNot) {
    std::string asm_code = generateAsm(
        "fn main() -> bool { not true }");
    // not x is 1 - x, so we should see sub
    EXPECT_NE(asm_code.find("sub"), std::string::npos);
}

// --- Function with parameters uses System V ABI ---
TEST(CodeGenTest, ParameterPassing) {
    std::string asm_code = generateAsm(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(20, 22) }");
    // Parameters passed in rdi, rsi
    // Caller sets up args before call
    EXPECT_NE(asm_code.find("call _add"), std::string::npos);
    EXPECT_NE(asm_code.find("rdi"), std::string::npos);
}

// --- If/else generates conditional jumps ---
TEST(CodeGenTest, ConditionalBranch) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 { if true { 1 } else { 0 } }");
    // Should have test + jnz pattern
    EXPECT_NE(asm_code.find("test"), std::string::npos);
    EXPECT_NE(asm_code.find("jnz"), std::string::npos);
}

// --- Multiple functions each get global label ---
TEST(CodeGenTest, MultipleFunctionLabels) {
    std::string asm_code = generateAsm(
        "fn foo() -> i64 { 1 }\n"
        "fn bar() -> i64 { 2 }\n"
        "fn main() -> i64 { foo() + bar() }");
    EXPECT_NE(asm_code.find("global _foo"), std::string::npos);
    EXPECT_NE(asm_code.find("global _bar"), std::string::npos);
    EXPECT_NE(asm_code.find("global _main"), std::string::npos);
    EXPECT_NE(asm_code.find("_foo:"), std::string::npos);
    EXPECT_NE(asm_code.find("_bar:"), std::string::npos);
    EXPECT_NE(asm_code.find("_main:"), std::string::npos);
}

// --- Val binding generates mov ---
TEST(CodeGenTest, ValBinding) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 {\n"
        "    val x: i64 = 42\n"
        "    x\n"
        "}");
    EXPECT_NE(asm_code.find("42"), std::string::npos);
    EXPECT_NE(asm_code.find("ret"), std::string::npos);
}

// --- Nested calls with callee-save ---
TEST(CodeGenTest, NestedCalls) {
    std::string asm_code = generateAsm(
        "fn double_val(x: i64) -> i64 { x + x }\n"
        "fn quad(x: i64) -> i64 { double_val(double_val(x)) }\n"
        "fn main() -> i64 { quad(5) }");
    // quad needs to save intermediate result across call
    // Should use callee-saved register push/pop
    int call_count = 0;
    size_t pos = 0;
    while ((pos = asm_code.find("call _double_val", pos)) != std::string::npos) {
        call_count++;
        pos += 16;
    }
    EXPECT_EQ(call_count, 2); // quad calls double_val twice
}
