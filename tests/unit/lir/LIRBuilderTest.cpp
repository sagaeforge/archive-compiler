#include "kern/lir/LIRBuilder.h"
#include "kern/lir/LIRDump.h"
#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRPasses.h"
#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class LIRBuilderTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    Module* parse(const char* source) {
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        return parser.parseModule();
    }

    LIRModule* buildLIR(const char* source) {
        auto* ast = parse(source);
        EXPECT_NE(ast, nullptr);
        if (!ast) return nullptr;
        EXPECT_FALSE(ctx.diag.hasErrors()) << "Parse errors";
        if (ctx.diag.hasErrors()) return nullptr;

        HIRBuilder hir_builder(ctx);
        HIRModule* hir = hir_builder.build(ast);
        EXPECT_FALSE(ctx.diag.hasErrors()) << "HIR errors";
        if (ctx.diag.hasErrors()) return nullptr;

        HIRPassManager pm;
        pm.add<PurityAnalysisPass>();
        pm.add<TailCallAnalysisPass>();
        pm.run(*hir, ctx);

        LIRBuilder lir_builder(ctx);
        return lir_builder.build(hir);
    }

    std::string dumpModule(LIRModule* mod) {
        std::ostringstream out;
        dumpLIR(mod, ctx.types, out);
        return out.str();
    }

    std::string dumpFn(LIRModule* mod, uint32_t idx = 0) {
        std::ostringstream out;
        dumpLIRFunction(&mod->functions[idx], ctx.types, out);
        return out.str();
    }
};

// ============================================================================
// Basic literals
// ============================================================================

TEST_F(LIRBuilderTest, ConstInt) {
    auto* mod = buildLIR("fn main() -> i64 { 42 }");
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->fn_count, 1u);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("const_int 42") != std::string::npos) << s;
    EXPECT_TRUE(s.find("ret") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, ConstBool) {
    auto* mod = buildLIR("fn main() -> i64 { if true { 1 } else { 0 } }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("const_bool true") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, ConstFloat) {
    auto* mod = buildLIR("fn main() -> f64 { 3.14 }");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->global_count, 1u);
    EXPECT_EQ(mod->globals[0].kind, GlobalData::FloatConst);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("global_ref") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, ConstString) {
    auto* mod = buildLIR("fn main() -> String { \"hello\" }");
    ASSERT_NE(mod, nullptr);
    EXPECT_GE(mod->global_count, 1u);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("const_string") != std::string::npos) << s;
}

// ============================================================================
// Parameters
// ============================================================================

TEST_F(LIRBuilderTest, SingleParam) {
    auto* mod = buildLIR("fn identity(x: i64) -> i64 { x }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_EQ(fn.param_count, 1u);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("block_arg $0") != std::string::npos) << s;
    EXPECT_TRUE(s.find("ret %v0") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, TwoParams) {
    auto* mod = buildLIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->functions[0].param_count, 2u);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("block_arg $0") != std::string::npos) << s;
    EXPECT_TRUE(s.find("block_arg $1") != std::string::npos) << s;
    EXPECT_TRUE(s.find("add %v0, %v1") != std::string::npos) << s;
}

// ============================================================================
// Arithmetic
// ============================================================================

TEST_F(LIRBuilderTest, IntArith) {
    auto* mod = buildLIR("fn calc(a: i64, b: i64) -> i64 { a + b * 2 }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("mul") != std::string::npos) << s;
    EXPECT_TRUE(s.find("add") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, FloatArith) {
    auto* mod = buildLIR("fn calc(a: f64, b: f64) -> f64 { a + b }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("fadd") != std::string::npos) << s;
}

// ============================================================================
// Comparison
// ============================================================================

TEST_F(LIRBuilderTest, IntComparison) {
    auto* mod = buildLIR(
        "fn cmp(a: i64, b: i64) -> i64 { if a < b { 1 } else { 0 } }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("icmp_lt") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, FloatComparison) {
    auto* mod = buildLIR(
        "fn cmp(a: f64, b: f64) -> i64 { if a == b { 1 } else { 0 } }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("fcmp_eq") != std::string::npos) << s;
}

// ============================================================================
// Unary
// ============================================================================

TEST_F(LIRBuilderTest, UnaryNeg) {
    auto* mod = buildLIR("fn neg(x: i64) -> i64 { -x }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("neg %v0") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, UnaryNot) {
    auto* mod = buildLIR(
        "fn inv(x: bool) -> i64 { if not x { 1 } else { 0 } }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("not %v0") != std::string::npos) << s;
}

// ============================================================================
// Calls
// ============================================================================

TEST_F(LIRBuilderTest, FunctionCall) {
    auto* mod = buildLIR(
        "fn double(x: i64) -> i64 { x + x }\n"
        "fn main() -> i64 { double(21) }"
    );
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->fn_count, 2u);
    auto s = dumpFn(mod, 1);
    // may be marked as tail call since it's in return position
    EXPECT_TRUE(s.find("call") != std::string::npos) << s;
    EXPECT_TRUE(s.find("@double") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, TailCall) {
    auto* mod = buildLIR(
        "fn loop(n: i64) -> i64 {\n"
        "    if n <= 0 { n }\n"
        "    else { loop(n - 1) }\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("[tail]") != std::string::npos) << s;
}

// ============================================================================
// If expression
// ============================================================================

TEST_F(LIRBuilderTest, IfExpr) {
    auto* mod = buildLIR(
        "fn abs(x: i64) -> i64 {\n"
        "    if x < 0 { -x } else { x }\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("condbr") != std::string::npos) << s;
    EXPECT_TRUE(s.find("if_then") != std::string::npos) << s;
    EXPECT_TRUE(s.find("if_else") != std::string::npos) << s;
    EXPECT_TRUE(s.find("if_merge") != std::string::npos) << s;
}

// ============================================================================
// Block + val/var
// ============================================================================

TEST_F(LIRBuilderTest, ValBinding) {
    auto* mod = buildLIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    val y: i64 = 20\n"
        "    x + y\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("const_int 10") != std::string::npos) << s;
    EXPECT_TRUE(s.find("const_int 20") != std::string::npos) << s;
    EXPECT_TRUE(s.find("add") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, VarBinding) {
    auto* mod = buildLIR(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x = 20\n"
        "    x\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("struct_alloc") != std::string::npos) << s;
    EXPECT_TRUE(s.find("store") != std::string::npos) << s;
    EXPECT_TRUE(s.find("load") != std::string::npos) << s;
}

// ============================================================================
// And/Or short-circuit
// ============================================================================

TEST_F(LIRBuilderTest, LogicalAnd) {
    auto* mod = buildLIR(
        "fn both(a: bool, b: bool) -> i64 { if a and b { 1 } else { 0 } }"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("and_eval") != std::string::npos) << s;
    EXPECT_TRUE(s.find("and_short") != std::string::npos) << s;
    EXPECT_TRUE(s.find("condbr") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, LogicalOr) {
    auto* mod = buildLIR(
        "fn either(a: bool, b: bool) -> i64 { if a or b { 1 } else { 0 } }"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("or_eval") != std::string::npos) << s;
    EXPECT_TRUE(s.find("or_short") != std::string::npos) << s;
}

// ============================================================================
// Match
// ============================================================================

TEST_F(LIRBuilderTest, MatchInt) {
    auto* mod = buildLIR(
        "fn classify(x: i64) -> i64 {\n"
        "    match x {\n"
        "        0 => 0\n"
        "        1 => 10\n"
        "        _ => 99\n"
        "    }\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("icmp_eq") != std::string::npos) << s;
    EXPECT_TRUE(s.find("match_arm") != std::string::npos) << s;
    EXPECT_TRUE(s.find("match_merge") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, MatchBool) {
    auto* mod = buildLIR(
        "fn to_int(b: bool) -> i64 {\n"
        "    match b {\n"
        "        true => 1\n"
        "        false => 0\n"
        "    }\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("match_arm") != std::string::npos) << s;
}

// ============================================================================
// Struct
// ============================================================================

TEST_F(LIRBuilderTest, StructLit) {
    auto* mod = buildLIR(
        "struct Point { val x: i64, val y: i64 }\n"
        "fn make() -> Point { Point { x: 1, y: 2 } }"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("struct_alloc") != std::string::npos) << s;
    EXPECT_TRUE(s.find("field_ptr") != std::string::npos) << s;
    EXPECT_TRUE(s.find("store") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, FieldAccess) {
    auto* mod = buildLIR(
        "struct Point { val x: i64, val y: i64 }\n"
        "fn get_x(p: Point) -> i64 { p.x }"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("field_ptr") != std::string::npos) << s;
    EXPECT_TRUE(s.find("load") != std::string::npos) << s;
}

// ============================================================================
// Enum
// ============================================================================

TEST_F(LIRBuilderTest, EnumAccess) {
    auto* mod = buildLIR(
        "enum Color { Red, Green, Blue }\n"
        "fn red() -> Color { Color.Red }"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("const_int 0") != std::string::npos) << s;
}

TEST_F(LIRBuilderTest, MatchEnum) {
    auto* mod = buildLIR(
        "enum Color { Red, Green, Blue }\n"
        "fn to_int(c: Color) -> i64 {\n"
        "    match c {\n"
        "        Red => 0\n"
        "        Green => 1\n"
        "        Blue => 2\n"
        "    }\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("icmp_eq") != std::string::npos) << s;
    EXPECT_TRUE(s.find("match_arm") != std::string::npos) << s;
}

// ============================================================================
// Union
// ============================================================================

TEST_F(LIRBuilderTest, UnionVariant) {
    auto* mod = buildLIR(
        "union Maybe { Some(i64), None }\n"
        "fn some_val() -> Maybe { Maybe::Some(42) }"
    );
    ASSERT_NE(mod, nullptr);
    auto s = dumpFn(mod);
    EXPECT_TRUE(s.find("struct_alloc") != std::string::npos) << s;
    EXPECT_TRUE(s.find("field_ptr") != std::string::npos) << s;
    EXPECT_TRUE(s.find("store") != std::string::npos) << s;
}

// ============================================================================
// Intrinsic
// ============================================================================

TEST_F(LIRBuilderTest, IntrinsicFn) {
    auto* mod = buildLIR(
        "fn print(x: i64) -> Unit = intrinsic\n"
        "fn main() -> i64 { 42 }"
    );
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->fn_count, 2u);
    EXPECT_EQ(mod->functions[0].block_count, 0u);
    EXPECT_TRUE(mod->functions[0].is_intrinsic);
    EXPECT_GT(mod->functions[1].block_count, 0u);
}

// ============================================================================
// Metadata preservation
// ============================================================================

TEST_F(LIRBuilderTest, PurityPreserved) {
    auto* mod = buildLIR(
        "fn pure_fn(x: i64) -> i64 { x + 1 }\n"
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    );
    ASSERT_NE(mod, nullptr);
    auto s0 = dumpFn(mod, 0);
    EXPECT_TRUE(s0.find("[pure]") != std::string::npos) << s0;
    auto s1 = dumpFn(mod, 1);
    EXPECT_TRUE(s1.find("[impure(mut)]") != std::string::npos) << s1;
}

// ============================================================================
// Multiple functions
// ============================================================================

TEST_F(LIRBuilderTest, MultipleFunctions) {
    auto* mod = buildLIR(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn mul(a: i64, b: i64) -> i64 { a * b }\n"
        "fn main() -> i64 { add(2, 3) + mul(4, 5) }"
    );
    ASSERT_NE(mod, nullptr);
    ASSERT_EQ(mod->fn_count, 3u);
    auto s = dumpFn(mod, 2);
    EXPECT_TRUE(s.find("call") != std::string::npos) << s;
    EXPECT_TRUE(s.find("@add") != std::string::npos) << s;
    EXPECT_TRUE(s.find("@mul") != std::string::npos) << s;
}

// ============================================================================
// Module dump
// ============================================================================

TEST_F(LIRBuilderTest, FullDump) {
    auto* mod = buildLIR("fn main() -> i64 { 42 }");
    ASSERT_NE(mod, nullptr);
    auto s = dumpModule(mod);
    EXPECT_TRUE(s.find("fn @main") != std::string::npos) << s;
    EXPECT_TRUE(s.find("entry:") != std::string::npos) << s;
}

} // namespace kern
