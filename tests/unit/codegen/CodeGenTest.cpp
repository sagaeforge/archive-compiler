#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/ir/IRBuilder.h"
#include "kern/codegen/CodeGen.h"
#include "kern/sema/TypeChecker.h"
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

    TypeChecker tc(diag);
    tc.check(mod);

    IRBuilder irBuilder;
    IRModule irMod = irBuilder.build(mod, tc);

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

// --- Negation: uses neg instruction ---
TEST(CodeGenTest, UnaryNeg) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 { -42 }");
    EXPECT_NE(asm_code.find("neg"), std::string::npos);
}

// --- Not: uses xor 1 instruction ---
TEST(CodeGenTest, UnaryNot) {
    std::string asm_code = generateAsm(
        "fn main() -> bool { not true }");
    EXPECT_NE(asm_code.find("xor"), std::string::npos);
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

// ===== Coverage improvement tests =====

// --- Comparison != generates setne ---
TEST(CodeGenTest, ComparisonNe) {
    std::string asm_code = generateAsm("fn main() -> bool { 1 != 2 }");
    EXPECT_NE(asm_code.find("setne"), std::string::npos);
}

// --- Comparison < generates setl ---
TEST(CodeGenTest, ComparisonLt) {
    std::string asm_code = generateAsm("fn main() -> bool { 1 < 2 }");
    EXPECT_NE(asm_code.find("setl"), std::string::npos);
}

// --- Comparison > generates setg ---
TEST(CodeGenTest, ComparisonGt) {
    std::string asm_code = generateAsm("fn main() -> bool { 1 > 2 }");
    EXPECT_NE(asm_code.find("setg"), std::string::npos);
}

// --- Comparison >= generates setge ---
TEST(CodeGenTest, ComparisonGe) {
    std::string asm_code = generateAsm("fn main() -> bool { 1 >= 2 }");
    EXPECT_NE(asm_code.find("setge"), std::string::npos);
}

// --- Logical and uses short-circuit (conditional branch) ---
TEST(CodeGenTest, LogicalAndShortCircuit) {
    std::string asm_code = generateAsm("fn main() -> bool { true and false }");
    // Short-circuit: if lhs then rhs else false
    EXPECT_NE(asm_code.find("test"), std::string::npos);
    EXPECT_NE(asm_code.find("jnz"), std::string::npos);
}

// --- Logical or uses short-circuit (conditional branch) ---
TEST(CodeGenTest, LogicalOrShortCircuit) {
    std::string asm_code = generateAsm("fn main() -> bool { true or false }");
    // Short-circuit: if lhs then true else rhs
    EXPECT_NE(asm_code.find("test"), std::string::npos);
    EXPECT_NE(asm_code.find("jnz"), std::string::npos);
}

// --- Many locals force callee-saved register usage ---
TEST(CodeGenTest, ManyLocalsCalleeSaved) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 {\n"
        "    val a: i64 = 1\n"
        "    val b: i64 = 2\n"
        "    val c: i64 = 3\n"
        "    val d: i64 = 4\n"
        "    val e: i64 = 5\n"
        "    val f: i64 = 6\n"
        "    val g: i64 = 7\n"
        "    a + b + c + d + e + f + g\n"
        "}"
    );
    // Should use at least one callee-saved register
    bool has_callee_saved = (asm_code.find("r12") != std::string::npos) ||
                            (asm_code.find("r13") != std::string::npos) ||
                            (asm_code.find("r14") != std::string::npos) ||
                            (asm_code.find("r15") != std::string::npos);
    EXPECT_TRUE(has_callee_saved);
}

// --- Register spill to stack ---
TEST(CodeGenTest, RegisterSpillToStack) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 {\n"
        "    val a: i64 = 1\n"
        "    val b: i64 = 2\n"
        "    val c: i64 = 3\n"
        "    val d: i64 = 4\n"
        "    val e: i64 = 5\n"
        "    val f: i64 = 6\n"
        "    val g: i64 = 7\n"
        "    val h: i64 = 8\n"
        "    val i: i64 = 9\n"
        "    val j: i64 = 10\n"
        "    val k: i64 = 11\n"
        "    val l: i64 = 12\n"
        "    val m: i64 = 13\n"
        "    a + b + c + d + e + f + g + h + i + j + k + l + m\n"
        "}"
    );
    // Should spill to stack via rbp
    EXPECT_NE(asm_code.find("[rbp"), std::string::npos);
}

// --- If without else generates code ---
TEST(CodeGenTest, IfWithoutElseCodeGen) {
    std::string asm_code = generateAsm("fn main() -> i64 { if true { 42 } }");
    EXPECT_NE(asm_code.find("test"), std::string::npos);
    EXPECT_NE(asm_code.find("jnz"), std::string::npos);
}

// ===== M2.3: Type-aware CodeGen tests =====

// --- Unsigned div uses div (not idiv) ---
TEST(CodeGenTest, UnsignedDivUsesDiv) {
    std::string asm_code = generateAsm(
        "fn udiv(a: u64, b: u64) -> u64 { a / b }");
    EXPECT_NE(asm_code.find("div"), std::string::npos);
    EXPECT_NE(asm_code.find("xor  edx, edx"), std::string::npos);
    // Should NOT contain cqo for unsigned
    EXPECT_EQ(asm_code.find("cqo"), std::string::npos);
}

// --- Signed div uses idiv + cqo ---
TEST(CodeGenTest, SignedDivUsesIdiv) {
    std::string asm_code = generateAsm(
        "fn sdiv(a: i64, b: i64) -> i64 { a / b }");
    EXPECT_NE(asm_code.find("idiv"), std::string::npos);
    EXPECT_NE(asm_code.find("cqo"), std::string::npos);
}

// --- Unsigned comparison < uses setb ---
TEST(CodeGenTest, UnsignedCmpLtUsesSetb) {
    std::string asm_code = generateAsm(
        "fn ucmp(a: u64, b: u64) -> bool { a < b }");
    EXPECT_NE(asm_code.find("setb"), std::string::npos);
}

// --- Unsigned comparison <= uses setbe ---
TEST(CodeGenTest, UnsignedCmpLeUsesSetbe) {
    std::string asm_code = generateAsm(
        "fn ucmp(a: u64, b: u64) -> bool { a <= b }");
    EXPECT_NE(asm_code.find("setbe"), std::string::npos);
}

// --- Unsigned comparison > uses seta ---
TEST(CodeGenTest, UnsignedCmpGtUsesSeta) {
    std::string asm_code = generateAsm(
        "fn ucmp(a: u64, b: u64) -> bool { a > b }");
    EXPECT_NE(asm_code.find("seta"), std::string::npos);
}

// --- Unsigned comparison >= uses setae ---
TEST(CodeGenTest, UnsignedCmpGeUsesSetae) {
    std::string asm_code = generateAsm(
        "fn ucmp(a: u64, b: u64) -> bool { a >= b }");
    EXPECT_NE(asm_code.find("setae"), std::string::npos);
}

// --- Signed comparison < still uses setl ---
TEST(CodeGenTest, SignedCmpLtUsesSetl) {
    std::string asm_code = generateAsm(
        "fn scmp(a: i64, b: i64) -> bool { a < b }");
    EXPECT_NE(asm_code.find("setl"), std::string::npos);
}

// --- Eq/Ne unchanged by signedness ---
TEST(CodeGenTest, UnsignedEqStillSete) {
    std::string asm_code = generateAsm(
        "fn ucmp(a: u64, b: u64) -> bool { a == b }");
    EXPECT_NE(asm_code.find("sete"), std::string::npos);
}

// --- i32 arithmetic uses same reg (all ops in 64-bit regs for M2) ---
TEST(CodeGenTest, I32ArithCodeGen) {
    std::string asm_code = generateAsm(
        "fn add32(a: i32, b: i32) -> i32 { a + b }");
    EXPECT_NE(asm_code.find("add"), std::string::npos);
}

// --- regForWidth static helper verification (via div which uses rax/edx) ---
TEST(CodeGenTest, UnsignedDivXorEdx) {
    std::string asm_code = generateAsm(
        "fn f(a: u64, b: u64) -> u64 { a / b }");
    // xor edx, edx zeroes rdx for unsigned div
    EXPECT_NE(asm_code.find("xor  edx, edx"), std::string::npos);
}

// --- i32 div signed uses idiv + cdq ---
TEST(CodeGenTest, I32SignedDiv) {
    std::string asm_code = generateAsm(
        "fn div32(a: i32, b: i32) -> i32 { a / b }");
    EXPECT_NE(asm_code.find("idiv"), std::string::npos);
    EXPECT_NE(asm_code.find("cdq"), std::string::npos);
}

// --- u32 div unsigned ---
TEST(CodeGenTest, U32UnsignedDiv) {
    std::string asm_code = generateAsm(
        "fn udiv32(a: u32, b: u32) -> u32 { a / b }");
    // Should NOT have cqo
    EXPECT_EQ(asm_code.find("cqo"), std::string::npos);
    EXPECT_NE(asm_code.find("xor  edx, edx"), std::string::npos);
}

// ===== M3.4: Float CodeGen tests =====

// --- ConstFloat: movsd from .rodata ---
TEST(CodeGenTest, FloatConst) {
    std::string asm_code = generateAsm(
        "fn main() -> f64 { 3.14 }");
    EXPECT_NE(asm_code.find("movsd"), std::string::npos);
    EXPECT_NE(asm_code.find("[rel "), std::string::npos);
}

// --- FAdd: addsd ---
TEST(CodeGenTest, FloatAdd) {
    std::string asm_code = generateAsm(
        "fn add(a: f64, b: f64) -> f64 { a + b }");
    EXPECT_NE(asm_code.find("addsd"), std::string::npos);
}

// --- FSub: subsd ---
TEST(CodeGenTest, FloatSub) {
    std::string asm_code = generateAsm(
        "fn sub(a: f64, b: f64) -> f64 { a - b }");
    EXPECT_NE(asm_code.find("subsd"), std::string::npos);
}

// --- FMul: mulsd ---
TEST(CodeGenTest, FloatMul) {
    std::string asm_code = generateAsm(
        "fn mul(a: f64, b: f64) -> f64 { a * b }");
    EXPECT_NE(asm_code.find("mulsd"), std::string::npos);
}

// --- FDiv: divsd ---
TEST(CodeGenTest, FloatDiv) {
    std::string asm_code = generateAsm(
        "fn div(a: f64, b: f64) -> f64 { a / b }");
    EXPECT_NE(asm_code.find("divsd"), std::string::npos);
}

// --- FCmpLt: ucomisd + setb ---
TEST(CodeGenTest, FloatCmpLt) {
    std::string asm_code = generateAsm(
        "fn lt(a: f64, b: f64) -> bool { a < b }");
    EXPECT_NE(asm_code.find("ucomisd"), std::string::npos);
    EXPECT_NE(asm_code.find("setb"), std::string::npos);
}

// --- FNeg: mulsd by -1.0 ---
TEST(CodeGenTest, FloatNeg) {
    std::string asm_code = generateAsm(
        "fn neg(a: f64) -> f64 { -a }");
    EXPECT_NE(asm_code.find("mulsd"), std::string::npos);
}

// --- F32: addss ---
TEST(CodeGenTest, F32Add) {
    std::string asm_code = generateAsm(
        "fn add32(a: f32, b: f32) -> f32 { a + b }");
    EXPECT_NE(asm_code.find("addss"), std::string::npos);
}

// --- Float return uses xmm0 ---
TEST(CodeGenTest, FloatReturnXmm0) {
    std::string asm_code = generateAsm(
        "fn main() -> f64 { 1.5 }");
    // Return should move to xmm0 (movsd xmm0)
    EXPECT_NE(asm_code.find("xmm0"), std::string::npos);
}

// --- .rodata section emitted for float constants ---
TEST(CodeGenTest, SectionRodata) {
    std::string asm_code = generateAsm(
        "fn main() -> f64 { 3.14 }");
    EXPECT_NE(asm_code.find("section .rodata"), std::string::npos);
    EXPECT_NE(asm_code.find("dq 0x"), std::string::npos);
}

// --- Float params use XMM registers ---
TEST(CodeGenTest, FloatParamsXmm) {
    std::string asm_code = generateAsm(
        "fn add(a: f64, b: f64) -> f64 { a + b }");
    EXPECT_NE(asm_code.find("xmm0"), std::string::npos);
    EXPECT_NE(asm_code.find("xmm1"), std::string::npos);
}

// --- Float call passes args in XMM ---
TEST(CodeGenTest, FloatCallArgs) {
    std::string asm_code = generateAsm(
        "fn add(a: f64, b: f64) -> f64 { a + b }\n"
        "fn main() -> f64 { add(1.0, 2.0) }");
    EXPECT_NE(asm_code.find("call _add"), std::string::npos);
    EXPECT_NE(asm_code.find("xmm0"), std::string::npos);
}
