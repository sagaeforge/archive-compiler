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

    TypeChecker tc(diag, &arena);
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
    // Caller sets up args before call (or jmp for tail call)
    bool has_call_or_jmp = (asm_code.find("call _add") != std::string::npos) ||
                           (asm_code.find("jmp  _add") != std::string::npos);
    EXPECT_TRUE(has_call_or_jmp);
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
    // quad calls double_val twice: inner is normal call, outer is tail call (jmp)
    int call_count = 0;
    int jmp_count = 0;
    size_t pos = 0;
    while ((pos = asm_code.find("call _double_val", pos)) != std::string::npos) {
        call_count++;
        pos += 16;
    }
    pos = 0;
    while ((pos = asm_code.find("jmp  _double_val", pos)) != std::string::npos) {
        jmp_count++;
        pos += 16;
    }
    EXPECT_EQ(call_count + jmp_count, 2); // quad invokes double_val twice total
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
    bool has_call_or_jmp_add = (asm_code.find("call _add") != std::string::npos) ||
                               (asm_code.find("jmp  _add") != std::string::npos);
    EXPECT_TRUE(has_call_or_jmp_add);
    EXPECT_NE(asm_code.find("xmm0"), std::string::npos);
}

// ===== TCE: Tail call CodeGen tests =====

// --- Tail call emits jmp, not call ---
TEST(CodeGenTest, TailCallEmitsJmp) {
    std::string asm_code = generateAsm(
        "fn g(x: i64) -> i64 { x }\n"
        "fn f(x: i64) -> i64 { g(x) }");
    // f tail-calls g: should have jmp _g, not call _g
    EXPECT_NE(asm_code.find("jmp  _g"), std::string::npos);
    // Find the _f function section and check no "call _g" there
    auto f_pos = asm_code.find("_f:");
    auto g_pos = asm_code.find("_g:");
    ASSERT_NE(f_pos, std::string::npos);
    std::string f_section = asm_code.substr(f_pos, g_pos > f_pos ? g_pos - f_pos : std::string::npos);
    EXPECT_EQ(f_section.find("call _g"), std::string::npos);
}

// --- Tail call restores frame (pop rbp before jmp) ---
TEST(CodeGenTest, TailCallRestoresFrame) {
    std::string asm_code = generateAsm(
        "fn g(x: i64) -> i64 { x }\n"
        "fn f(x: i64) -> i64 { g(x) }");
    // Deferred tail epilogue: "pop rbp" immediately before "jmp _g"
    auto jmp_pos = asm_code.find("jmp  _g");
    ASSERT_NE(jmp_pos, std::string::npos);
    // Find the "pop  rbp" that comes right before this jmp
    auto pop_pos = asm_code.rfind("pop  rbp", jmp_pos);
    ASSERT_NE(pop_pos, std::string::npos);
    EXPECT_LT(pop_pos, jmp_pos);
}

// --- Self tail call emits jmp ---
TEST(CodeGenTest, SelfTailCallEmitsJmp) {
    std::string asm_code = generateAsm(
        "fn countdown(n: i64) -> i64 {\n"
        "    if n <= 0 { 0 } else { countdown(n - 1) }\n"
        "}");
    EXPECT_NE(asm_code.find("jmp  _countdown"), std::string::npos);
}

// --- Non-tail call still uses call ---
TEST(CodeGenTest, NonTailCallStillUsesCall) {
    std::string asm_code = generateAsm(
        "fn g(x: i64) -> i64 { x }\n"
        "fn f(x: i64) -> i64 { g(x) + 1 }");
    EXPECT_NE(asm_code.find("call _g"), std::string::npos);
}

// --- Tail call with different param count ---
TEST(CodeGenTest, TailCallDifferentParamCount) {
    std::string asm_code = generateAsm(
        "fn g(a: i64, b: i64, c: i64) -> i64 { a + b + c }\n"
        "fn f(x: i64) -> i64 { g(x, x, x) }");
    EXPECT_NE(asm_code.find("jmp  _g"), std::string::npos);
}

// --- Tail call with float args ---
TEST(CodeGenTest, TailCallFloatArgs) {
    std::string asm_code = generateAsm(
        "fn g(a: f64) -> f64 { a }\n"
        "fn f(a: f64) -> f64 { g(a) }");
    EXPECT_NE(asm_code.find("jmp  _g"), std::string::npos);
}

// ===== M5a: Struct CodeGen tests =====

TEST(CodeGenTest, StructAllocReservesStack) {
    std::string asm_code = generateAsm(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, y: 2 }\n"
        "    p.x\n"
        "}"
    );
    // Should have sub rsp for stack frame (struct needs stack space)
    EXPECT_NE(asm_code.find("sub  rsp"), std::string::npos);
    // Should store field values to stack via mov
    EXPECT_NE(asm_code.find("[rbp"), std::string::npos);
}

TEST(CodeGenTest, StructFieldStoreAndLoad) {
    std::string asm_code = generateAsm(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 42, y: 99 }\n"
        "    p.x\n"
        "}"
    );
    // The value 42 and 99 should appear in the asm
    EXPECT_NE(asm_code.find("42"), std::string::npos);
    EXPECT_NE(asm_code.find("99"), std::string::npos);
}

TEST(CodeGenTest, StructFieldAssignGenMov) {
    std::string asm_code = generateAsm(
        "struct Point { var x: i64, var y: i64 }\n"
        "fn main() -> i64 {\n"
        "    var p: Point = Point { x: 1, y: 2 }\n"
        "    p.x = 42\n"
        "    p.x\n"
        "}"
    );
    // Should have mov instructions writing 42 to the struct field
    EXPECT_NE(asm_code.find("42"), std::string::npos);
}

TEST(CodeGenTest, StructParamUnpack) {
    std::string asm_code = generateAsm(
        "struct Point { x: i64, y: i64 }\n"
        "fn get_x(p: Point) -> i64 { p.x }"
    );
    // Struct param should be stored from rdi to stack
    EXPECT_NE(asm_code.find("rdi"), std::string::npos);
    EXPECT_NE(asm_code.find("[rbp"), std::string::npos);
}

// ===== M5b: Enum/Union CodeGen tests =====

TEST(CodeGenTest, EnumAccessEmitsConstant) {
    std::string asm_code = generateAsm(
        "enum Color { Red, Green, Blue }\n"
        "fn main() -> Color { Color.Blue }"
    );
    // Blue is tag 2
    EXPECT_NE(asm_code.find("2"), std::string::npos);
    EXPECT_NE(asm_code.find("ret"), std::string::npos);
}

TEST(CodeGenTest, UnionVariantAllocAndStore) {
    std::string asm_code = generateAsm(
        "union Shape { Circle(i64), Square(i64) }\n"
        "fn main() -> i64 {\n"
        "    val s: Shape = Shape::Circle(42)\n"
        "    0\n"
        "}"
    );
    // Should have stack writes for tag and payload
    EXPECT_NE(asm_code.find("[rbp"), std::string::npos);
    EXPECT_NE(asm_code.find("42"), std::string::npos);
}

TEST(CodeGenTest, EnumMatchGenCmp) {
    std::string asm_code = generateAsm(
        "enum Color { Red, Green, Blue }\n"
        "fn f(c: Color) -> i64 {\n"
        "    match c {\n"
        "        Red => 1,\n"
        "        Green => 2,\n"
        "        Blue => 3\n"
        "    }\n"
        "}"
    );
    EXPECT_NE(asm_code.find("cmp"), std::string::npos);
}

// ===== M5c: Pointer CodeGen tests =====

TEST(CodeGenTest, AddrOfEmitsLea) {
    std::string asm_code = generateAsm(
        "fn f(x: i64) -> Ptr<i64> { &x }"
    );
    EXPECT_NE(asm_code.find("lea"), std::string::npos);
}

TEST(CodeGenTest, DerefEmitsMovFromPtr) {
    std::string asm_code = generateAsm(
        "fn f(p: Ptr<i64>) -> i64 { (*p) }"
    );
    // Should have a mov that dereferences [reg]
    EXPECT_NE(asm_code.find("mov"), std::string::npos);
}

TEST(CodeGenTest, DerefAssignEmitsMovToPtr) {
    std::string asm_code = generateAsm(
        "fn f(p: Ptr<var i64>) -> i64 {\n"
        "    *p = 42\n"
        "    val r: i64 = (*p)\n"
        "    r\n"
        "}"
    );
    // Should have mov [reg], ... for the store
    EXPECT_NE(asm_code.find("42"), std::string::npos);
}

// ===== String CodeGen tests =====

TEST(CodeGenTest, StringLitEmitsRodataAndLea) {
    std::string asm_code = generateAsm(
        "fn main() -> i64 {\n"
        "    val s: String = \"hello\"\n"
        "    42\n"
        "}"
    );
    // Should have lea for string data address from .rodata
    EXPECT_NE(asm_code.find("lea"), std::string::npos);
    EXPECT_NE(asm_code.find("._str_"), std::string::npos);
    // Should have .rodata section with string data
    EXPECT_NE(asm_code.find(".rodata"), std::string::npos);
    // String length (5) should be stored
    EXPECT_NE(asm_code.find("5"), std::string::npos);
}

TEST(CodeGenTest, StringLenFieldAccess) {
    std::string asm_code = generateAsm(
        "fn main() -> u64 {\n"
        "    val s: String = \"hi\"\n"
        "    s.len\n"
        "}"
    );
    // Should have mov for field load at offset 8 (len field)
    EXPECT_NE(asm_code.find("rbp"), std::string::npos);
    // String "hi" has length 2
    EXPECT_NE(asm_code.find("2"), std::string::npos);
}
