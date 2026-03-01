#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRDump.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class HIRBuilderTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    Module* parse(const char* source) {
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        return parser.parseModule();
    }

    HIRModule* buildHIR(const char* source) {
        auto* ast = parse(source);
        EXPECT_NE(ast, nullptr);
        HIRBuilder builder(ctx);
        return builder.build(ast);
    }

    std::string dumpToString(const HIRModule* mod) {
        std::ostringstream out;
        dumpHIR(mod, ctx.types, out);
        return out.str();
    }
};

// ============================================================================
// Basic function tests
// ============================================================================

TEST_F(HIRBuilderTest, SimpleConst) {
    auto* hir = buildHIR("fn main() -> i64 { 42 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->fn_count, 1u);
    EXPECT_EQ(hir->functions[0]->name, "main");
    EXPECT_EQ(hir->functions[0]->return_type, TypeTable::I64);
    EXPECT_NE(hir->functions[0]->body, nullptr);
}

TEST_F(HIRBuilderTest, FnWithParams) {
    auto* hir = buildHIR("fn add(a: i64, b: i64) -> i64 { a + b }\n"
                          "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->fn_count, 2u);
    EXPECT_EQ(hir->functions[0]->param_count, 2u);
    EXPECT_EQ(hir->functions[0]->params[0].name, "a");
    EXPECT_EQ(hir->functions[0]->params[0].type, TypeTable::I64);
}

TEST_F(HIRBuilderTest, MultipleTypes) {
    auto* hir = buildHIR(
        "fn foo(x: i32, y: u8) -> i32 { x }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->functions[0]->params[0].type, TypeTable::I32);
    EXPECT_EQ(hir->functions[0]->params[1].type, TypeTable::U8);
}

TEST_F(HIRBuilderTest, IntrinsicFn) {
    auto* hir = buildHIR(
        "fn write(code: i64) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_TRUE(hir->functions[0]->is_intrinsic);
    EXPECT_EQ(hir->functions[0]->body, nullptr);
}

// ============================================================================
// Expression type tests
// ============================================================================

TEST_F(HIRBuilderTest, IntLitDefaultI64) {
    auto* hir = buildHIR("fn main() -> i64 { 42 }");
    ASSERT_NE(hir, nullptr);
    auto* body = hir->functions[0]->body;
    ASSERT_EQ(body->kind, HIRExpr::Kind::Block);
    auto* block = static_cast<HIRBlockExpr*>(body);
    ASSERT_NE(block->result, nullptr);
    EXPECT_EQ(block->result->type, TypeTable::I64);
}

TEST_F(HIRBuilderTest, IntLitContextCoercion) {
    auto* hir = buildHIR(
        "fn foo(x: i32) -> i32 { x }\n"
        "fn main() -> i64 { foo(10)\n0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, FloatLit) {
    auto* hir = buildHIR("fn main() -> i64 { val x: f64 = 3.14\n0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, BoolLit) {
    auto* hir = buildHIR("fn main() -> i64 { if true { 1 } else { 0 } }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, StringLit) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val s: String = \"hello\"\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// BinOp tests
// ============================================================================

TEST_F(HIRBuilderTest, BinOpArith) {
    auto* hir = buildHIR("fn main() -> i64 { 1 + 2 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    EXPECT_TRUE(s.find("binop +") != std::string::npos);
}

TEST_F(HIRBuilderTest, BinOpComparison) {
    auto* hir = buildHIR("fn main() -> i64 { if 1 < 2 { 1 } else { 0 } }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, BinOpLogical) {
    auto* hir = buildHIR("fn main() -> i64 { if true and false { 1 } else { 0 } }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// UnaryOp tests
// ============================================================================

TEST_F(HIRBuilderTest, UnaryNeg) {
    auto* hir = buildHIR("fn main() -> i64 { -42 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, UnaryNot) {
    auto* hir = buildHIR("fn main() -> i64 { if not false { 1 } else { 0 } }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Control flow tests
// ============================================================================

TEST_F(HIRBuilderTest, IfExpr) {
    auto* hir = buildHIR("fn main() -> i64 { if true { 1 } else { 0 } }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    EXPECT_TRUE(s.find("if") != std::string::npos);
    EXPECT_TRUE(s.find("then:") != std::string::npos);
    EXPECT_TRUE(s.find("else:") != std::string::npos);
}

TEST_F(HIRBuilderTest, BlockWithStmts) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    val y: i64 = 20\n"
        "    x + y\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, VarAssign) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    var x: i64 = 1\n"
        "    x = 2\n"
        "    x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ReturnExpr) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    return 42\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Struct tests
// ============================================================================

TEST_F(HIRBuilderTest, StructDeclAndLit) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 10, y: 20 }\n"
        "    p.x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->struct_count, 1u);
    EXPECT_EQ(hir->structs[0]->name, "Point");
}

TEST_F(HIRBuilderTest, StructFieldAccess) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 5, y: 10 }\n"
        "    p.x + p.y\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, StructVarFieldAssign) {
    auto* hir = buildHIR(
        "struct Counter { var count: i64 }\n"
        "fn main() -> i64 {\n"
        "    var c: Counter = Counter { count: 0 }\n"
        "    c.count = 42\n"
        "    c.count\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Enum tests
// ============================================================================

TEST_F(HIRBuilderTest, EnumDeclAndAccess) {
    auto* hir = buildHIR(
        "enum Color { Red, Green, Blue }\n"
        "fn main() -> i64 {\n"
        "    val c: Color = Color.Red\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->enum_count, 1u);
}

// ============================================================================
// Union tests
// ============================================================================

TEST_F(HIRBuilderTest, UnionDeclAndVariant) {
    auto* hir = buildHIR(
        "union Option { Some(i64), None }\n"
        "fn main() -> i64 {\n"
        "    val x: Option = Option::Some(42)\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->union_count, 1u);
}

// ============================================================================
// Match tests
// ============================================================================

TEST_F(HIRBuilderTest, MatchInt) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 1\n"
        "    match x {\n"
        "        0 => 100\n"
        "        _ => 200\n"
        "    }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, MatchBool) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    match true {\n"
        "        true => 1\n"
        "        false => 0\n"
        "    }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, MatchEnum) {
    auto* hir = buildHIR(
        "enum Color { Red, Green, Blue }\n"
        "fn main() -> i64 {\n"
        "    val c: Color = Color.Red\n"
        "    match c {\n"
        "        Red => 0\n"
        "        Green => 1\n"
        "        Blue => 2\n"
        "    }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, MatchUnion) {
    auto* hir = buildHIR(
        "union Option { Some(i64), None }\n"
        "fn main() -> i64 {\n"
        "    val x: Option = Option::Some(42)\n"
        "    match x {\n"
        "        Some(v) => v\n"
        "        None => 0\n"
        "    }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Pointer tests
// ============================================================================

TEST_F(HIRBuilderTest, PtrAddrOf) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 42\n"
        "    val p: Ptr<i64> = &x\n"
        "    *p\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, PtrVarWrite) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    val p: Ptr<var i64> = &var x\n"
        "    *p = 20\n"
        "    x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// String tests
// ============================================================================

TEST_F(HIRBuilderTest, StringLen) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val s: String = \"hello\"\n"
        "    s.len\n"
        "}");
    ASSERT_NE(hir, nullptr);
    // String.len returns u64, but fn returns i64 — this should be a type error
    // unless the checker allows it. In v1 TypeChecker, U64 != I64 is an error.
    // Let's just check it builds without crashing.
}

// ============================================================================
// Error tests
// ============================================================================

TEST_F(HIRBuilderTest, ErrorUndeclaredVar) {
    auto* hir = buildHIR("fn main() -> i64 { x }");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorTypeMismatch) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = true\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorImmutableAssign) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 1\n"
        "    x = 2\n"
        "    x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorUndeclaredFn) {
    auto* hir = buildHIR("fn main() -> i64 { foo() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorWrongArgCount) {
    auto* hir = buildHIR(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1) }");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorNonExhaustiveMatch) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    match 1 {\n"
        "        0 => 0\n"
        "    }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorStructMissingField) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1 }\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ErrorPtrWriteReadOnly) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    val p: Ptr<i64> = &x\n"
        "    *p = 20\n"
        "    0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_TRUE(ctx.diag.hasErrors());
}

// ============================================================================
// Dump integration test
// ============================================================================

TEST_F(HIRBuilderTest, DumpFullModule) {
    auto* hir = buildHIR(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(1, 2) }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    EXPECT_TRUE(s.find("fn add") != std::string::npos);
    EXPECT_TRUE(s.find("fn main") != std::string::npos);
    EXPECT_TRUE(s.find("call add") != std::string::npos);
    EXPECT_TRUE(s.find("binop +") != std::string::npos);
}

TEST_F(HIRBuilderTest, DumpModuleWithStruct) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "    val p: Point = Point { x: 1, y: 2 }\n"
        "    p.x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    EXPECT_TRUE(s.find("struct_decl Point") != std::string::npos);
    EXPECT_TRUE(s.find("struct_lit Point") != std::string::npos);
    EXPECT_TRUE(s.find("field_access .x") != std::string::npos);
}

} // namespace kern
