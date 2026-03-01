#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/ir/IRBuilder.h"
#include "kern/ir/KernIR.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

struct IRResult {
    std::string source;
    Arena arena;
    DiagnosticEngine diag;
    IRModule ir;
};

static IRResult buildIR(std::string src) {
    IRResult r;
    r.source = std::move(src);
    Lexer lexer(r.source, "test.kern", r.diag);
    Parser parser(lexer, r.arena, r.diag);
    Module* mod = parser.parseModule();
    EXPECT_FALSE(r.diag.hasErrors());

    IRBuilder builder;
    r.ir = builder.build(mod);
    return r;
}

static bool hasOpcode(const IRFunction& fn, IROpcode op) {
    for (const auto& block : fn.blocks) {
        for (const auto& instr : block.instrs) {
            if (instr.op == op) return true;
        }
    }
    return false;
}

// ===== Existing tests =====

TEST(IRTest, SimpleConstant) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_EQ(r.ir.functions[0].name, "main");
    EXPECT_FALSE(r.ir.functions[0].blocks.empty());
}

TEST(IRTest, FibonacciIR) {
    auto r = buildIR(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n }\n"
        "    else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_EQ(r.ir.functions[0].name, "fib");
    EXPECT_GE(r.ir.functions[0].blocks.size(), 3u);
}

TEST(IRTest, IRDump) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("fn main"), std::string::npos);
    EXPECT_NE(dump.find("const_int"), std::string::npos);
    EXPECT_NE(dump.find("ret"), std::string::npos);
}

TEST(IRTest, FunctionCall) {
    auto r = buildIR(
        "fn double_val(x: i64) -> i64 { x + x }\n"
        "fn main() -> i64 { double_val(21) }"
    );
    ASSERT_EQ(r.ir.functions.size(), 2u);

    bool found_call = false;
    for (const auto& block : r.ir.functions[1].blocks) {
        for (const auto& instr : block.instrs) {
            if (instr.op == IROpcode::Call) {
                found_call = true;
                EXPECT_EQ(instr.callee_name, "double_val");
            }
        }
    }
    EXPECT_TRUE(found_call);
}

// ===== New TDD tests =====

// --- Val binding generates ConstInt + reference ---
TEST(IRTest, ValBinding) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    x\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ConstInt));
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Ret));
}

// --- Arithmetic generates Add ---
TEST(IRTest, ArithmeticAdd) {
    auto r = buildIR("fn main() -> i64 { 1 + 2 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Add));
}

// --- Subtraction ---
TEST(IRTest, ArithmeticSub) {
    auto r = buildIR("fn main() -> i64 { 10 - 3 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Sub));
}

// --- Multiplication ---
TEST(IRTest, ArithmeticMul) {
    auto r = buildIR("fn main() -> i64 { 6 * 7 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Mul));
}

// --- Division ---
TEST(IRTest, ArithmeticDiv) {
    auto r = buildIR("fn main() -> i64 { 84 / 2 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Div));
}

// --- Comparison generates ICmpLe ---
TEST(IRTest, Comparison) {
    auto r = buildIR("fn main() -> bool { 1 <= 2 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpLe));
}

// --- Negation is lowered to Sub(0, x) ---
TEST(IRTest, UnaryNeg) {
    auto r = buildIR("fn main() -> i64 { -42 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    // -x is lowered to 0 - x
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Sub));
}

// --- Not is lowered to Sub(1, x) ---
TEST(IRTest, UnaryNot) {
    auto r = buildIR("fn main() -> bool { not true }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    // not x is lowered to 1 - x
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Sub));
}

// --- If/else generates CondBranch ---
TEST(IRTest, IfElseGeneratesCondBranch) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    if true { 1 } else { 0 }\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::CondBranch));
}

// --- Nested if generates multiple CondBranch ---
TEST(IRTest, NestedIf) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    if true { if false { 1 } else { 2 } } else { 3 }\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    int cond_count = 0;
    for (const auto& block : r.ir.functions[0].blocks) {
        for (const auto& instr : block.instrs) {
            if (instr.op == IROpcode::CondBranch) cond_count++;
        }
    }
    EXPECT_GE(cond_count, 2);
}

// --- Multiple functions in IR ---
TEST(IRTest, MultipleFunctions) {
    auto r = buildIR(
        "fn a() -> i64 { 1 }\n"
        "fn b() -> i64 { 2 }\n"
        "fn main() -> i64 { a() + b() }"
    );
    ASSERT_EQ(r.ir.functions.size(), 3u);
    EXPECT_EQ(r.ir.functions[0].name, "a");
    EXPECT_EQ(r.ir.functions[1].name, "b");
    EXPECT_EQ(r.ir.functions[2].name, "main");
}

// --- Parameters become values ---
TEST(IRTest, Parameters) {
    auto r = buildIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_EQ(r.ir.functions[0].param_values.size(), 2u);
    EXPECT_EQ(r.ir.functions[0].param_names.size(), 2u);
    EXPECT_EQ(r.ir.functions[0].param_names[0], "a");
    EXPECT_EQ(r.ir.functions[0].param_names[1], "b");
}

// --- IR dump contains all expected elements for fib ---
TEST(IRTest, FibDumpComprehensive) {
    auto r = buildIR(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("fn fib"), std::string::npos);
    EXPECT_NE(dump.find("icmp_le"), std::string::npos);
    EXPECT_NE(dump.find("condbr"), std::string::npos);
    EXPECT_NE(dump.find("call"), std::string::npos);
    EXPECT_NE(dump.find("add"), std::string::npos);
    EXPECT_NE(dump.find("sub"), std::string::npos);
}
