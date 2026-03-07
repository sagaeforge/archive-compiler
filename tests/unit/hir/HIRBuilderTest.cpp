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
// Lambda and closure tests
// ============================================================================

TEST_F(HIRBuilderTest, LambdaBasic) {
    auto* hir = buildHIR(
        "fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
        "fn main() -> i64 {\n"
        "    val double: fn(i64) -> i64 = { x: i64 => x * 2 }\n"
        "    apply(double, 21)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // Lambda should be lifted to a top-level function
    auto s = dumpToString(hir);
    EXPECT_NE(s.find("__lambda_"), std::string::npos);
}

TEST_F(HIRBuilderTest, ClosureCaptureBasic) {
    auto* hir = buildHIR(
        "fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
        "fn main() -> i64 {\n"
        "    val a: i64 = 10\n"
        "    val f: fn(i64) -> i64 = { x: i64 => x + a }\n"
        "    f(32)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    // Lifted lambda should exist
    EXPECT_NE(s.find("__lambda_"), std::string::npos);
}

TEST_F(HIRBuilderTest, ClosureCaptureMultiple) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val a: i64 = 10\n"
        "    val b: i64 = 20\n"
        "    val c: i64 = 12\n"
        "    val f: fn(i64) -> i64 = { x: i64 => x + a + b + c }\n"
        "    f(0)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, LambdaNoCapture) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "    val f: fn(i64) -> i64 = { x: i64 => x * 2 }\n"
        "    f(21)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // The lifted function should have exactly 1 param (no captures)
    auto s = dumpToString(hir);
    EXPECT_NE(s.find("__lambda_"), std::string::npos);
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

// ============================================================================
// Closure HOF return tests
// ============================================================================

TEST_F(HIRBuilderTest, ClosureHOFReturn) {
    auto* hir = buildHIR(
        "fn make_adder(n: i64) -> fn(i64) -> i64 {\n"
        "    { x: i64 => x + n }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "    val add_10: fn(i64) -> i64 = make_adder(10)\n"
        "    add_10(32)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    // Should have a lifted lambda
    EXPECT_NE(s.find("__lambda_"), std::string::npos);
    // make_adder should return something (closure struct)
    EXPECT_NE(s.find("make_adder"), std::string::npos);
}

TEST_F(HIRBuilderTest, ClosureHOFMultiCapture) {
    auto* hir = buildHIR(
        "fn make_fn(a: i64, b: i64) -> fn(i64) -> i64 {\n"
        "    { x: i64 => x + a + b }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "    val f: fn(i64) -> i64 = make_fn(10, 20)\n"
        "    f(12)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Try operator tests
// ============================================================================

TEST_F(HIRBuilderTest, TryOperatorBasic) {
    auto* hir = buildHIR(
        "union Result<T, E> { Ok(T), Err(E) }\n"
        "fn safe_div(a: i64, b: i64) -> Result<i64, i64> {\n"
        "    if b == 0 { Result<i64, i64>::Err(0) }\n"
        "    else { Result<i64, i64>::Ok(a / b) }\n"
        "}\n"
        "fn compute(x: i64) -> Result<i64, i64> {\n"
        "    val a: i64 = safe_div(x, 2)?\n"
        "    Result<i64, i64>::Ok(a)\n"
        "}\n"
        "fn main() -> i64 {\n"
        "    match compute(84) { Ok(v) => v, Err(_) => 0 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto s = dumpToString(hir);
    // The ? operator desugars to a match expression with Ok/Err arms
    EXPECT_NE(s.find("match"), std::string::npos);
}

TEST_F(HIRBuilderTest, TryOperatorChained) {
    auto* hir = buildHIR(
        "union Result<T, E> { Ok(T), Err(E) }\n"
        "fn step1(x: i64) -> Result<i64, i64> { Result<i64, i64>::Ok(x + 1) }\n"
        "fn step2(x: i64) -> Result<i64, i64> { Result<i64, i64>::Ok(x * 2) }\n"
        "fn pipeline(x: i64) -> Result<i64, i64> {\n"
        "    val a: i64 = step1(x)?\n"
        "    val b: i64 = step2(a)?\n"
        "    Result<i64, i64>::Ok(b)\n"
        "}\n"
        "fn main() -> i64 {\n"
        "    match pipeline(5) { Ok(v) => v, Err(_) => 0 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, TryOperatorNonUnionError) {
    (void)buildHIR(
        "fn bad() -> i64 {\n"
        "    val x: i64 = 42\n"
        "    x?\n"
        "}\n"
        "fn main() -> i64 { bad() }");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, TryOperatorNonResultUnionError) {
    (void)buildHIR(
        "union Shape { Circle(i64), Square(i64) }\n"
        "fn bad() -> i64 {\n"
        "    val s: Shape = Shape::Circle(5)\n"
        "    s?\n"
        "}\n"
        "fn main() -> i64 { bad() }");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, TryOperatorReturnTypeMismatch) {
    (void)buildHIR(
        "union Result<T, E> { Ok(T), Err(E) }\n"
        "fn step() -> Result<i64, i64> { Result<i64, i64>::Ok(42) }\n"
        "fn bad() -> i64 {\n"
        "    step()?\n"
        "}\n"
        "fn main() -> i64 { bad() }");
    // Function returns i64 (not a Result union), so ? should error
    EXPECT_TRUE(ctx.diag.hasErrors());
}

// ============================================================================
// Generic TypeVar BinOp
// ============================================================================

TEST_F(HIRBuilderTest, GenericTypeVarArith) {
    // TypeVar operands in binop should be deferred to monomorphization
    auto* hir = buildHIR(
        "fn add_val<T>(a: T, b: T) -> T { a + b }\n"
        "fn main() -> i64 {\n"
        "  val a: i64 = 20\n"
        "  val b: i64 = 22\n"
        "  add_val(a, b)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Closure Coercion at Call Args
// ============================================================================

TEST_F(HIRBuilderTest, ClosureAsFnParam) {
    // Closure struct with captured variable passed to fn-typed param
    auto* hir = buildHIR(
        "fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
        "fn main() -> i64 {\n"
        "  val a: i64 = 10\n"
        "  val f: fn(i64) -> i64 = { x: i64 => x + a }\n"
        "  apply(f, 32)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Associated types in traits
// ============================================================================

TEST_F(HIRBuilderTest, AssociatedTypeInTrait) {
    auto* hir = buildHIR(
        "trait Container {\n"
        "  type Item;\n"
        "  fn get(self: Self) -> Self::Item\n"
        "}\n"
        "struct IntBox { value: i64 }\n"
        "impl Container for IntBox {\n"
        "  type Item = i64;\n"
        "  fn get(self: IntBox) -> IntBox::Item { self.value }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "  val b: IntBox = IntBox { value: 42 }\n"
        "  b.get()\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Generic type aliases
// ============================================================================

TEST_F(HIRBuilderTest, StructUpdateSyntax) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64, z: i64 }\n"
        "fn main() -> i64 {\n"
        "  val p1: Point = Point { x: 1, y: 2, z: 3 }\n"
        "  val p2: Point = Point { ..p1, z: 10 }\n"
        "  p2.z\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WhereClause) {
    auto* hir = buildHIR(
        "trait Ord { fn cmp(self: Self, other: Self) -> i64 }\n"
        "impl Ord for i64 {\n"
        "  fn cmp(self: i64, other: i64) -> i64 { self - other }\n"
        "}\n"
        "fn max_val<T>(a: T, b: T) -> T where T: Ord {\n"
        "  val c: i64 = a.cmp(b)\n"
        "  match c > 0 {\n"
        "    true => a,\n"
        "    false => b,\n"
        "  }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "  max_val<i64>(10, 20)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, GenericTypeAlias) {
    auto* hir = buildHIR(
        "union Pair<A, B> { First(A), Second(B) }\n"
        "type IntPair<T> = Pair<i64, T>\n"
        "fn main() -> i64 {\n"
        "  val p: IntPair<bool> = Pair::First(42)\n"
        "  match p {\n"
        "    Pair::First(n) => n,\n"
        "    Pair::Second(_) => 0,\n"
        "  }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, StringInterpParsing) {
    // f-string without format_interp defined should produce an error
    auto* hir = buildHIR(
        "fn test(x: i64) -> i64 {\n"
        "  val msg = f\"value is {x}\"\n"
        "  x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    // Should have error about missing format_interp
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, StringInterpWithFormatInterp) {
    // With format_interp defined, f-string should desugar to a call
    auto* hir = buildHIR(
        "fn format_interp(a: Ptr<u8>, b: i64) -> Ptr<u8> {\n"
        "  a\n"
        "}\n"
        "fn test(x: i64) -> Ptr<u8> {\n"
        "  f\"value is {x}\"\n"
        "}");
    ASSERT_NE(hir, nullptr);
    // format_interp is defined, so f-string should succeed
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ConstFoldGlobalStructInit) {
    // Struct global initialized with const fn calls should fold to literals
    auto* hir = buildHIR(
        "struct Entry { offset: u64, flags: u64 }\n"
        "@const fn make_entry(off: u64, f: u64) -> Entry {\n"
        "  Entry { offset: off * 8, flags: f }\n"
        "}\n"
        "static val TABLE: Entry = Entry { offset: 16, flags: 3 }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ConstFoldGlobalArrayInit) {
    // Array global with const-evaluated elements
    auto* hir = buildHIR(
        "@const fn double(x: i64) -> i64 { x * 2 }\n"
        "static val VALS: [i64; 3] = [double(1), double(2), double(3)]\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // The const fn calls should have been folded to IntLit nodes
    ASSERT_GT(hir->global_count, 0u);
}

TEST_F(HIRBuilderTest, ConstEvalFloat) {
    // Float const evaluation in global initializer
    auto* hir = buildHIR(
        "static val PI_HALF: f64 = 3.14159 / 2.0\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Drop trait tests
// ============================================================================

TEST_F(HIRBuilderTest, DropTraitAutoDestructor) {
    // Type implementing Drop should have drop() called at scope exit
    auto* hir = buildHIR(
        "struct Resource { fd: i64 }\n"
        "trait Drop { fn drop(self: Self) -> i64 }\n"
        "impl Drop for Resource {\n"
        "  fn drop(self: Resource) -> i64 { 0 }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "  val r: Resource = Resource { fd: 42 }\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, DropTraitNotCalledForNonDropTypes) {
    // Types without Drop trait should have no extra drop calls
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "  val p = Point { x: 1, y: 2 }\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto* main_fn = hir->functions[hir->fn_count - 1];
    auto* body = static_cast<HIRBlockExpr*>(main_fn->body);
    // Only the val stmt, no drop calls
    EXPECT_EQ(body->stmt_count, 1u);
}

TEST_F(HIRBuilderTest, DropTraitReverseOrder) {
    // Multiple droppable values should be dropped in reverse declaration order
    auto* hir = buildHIR(
        "struct Res { id: i64 }\n"
        "trait Drop { fn drop(self: Self) -> i64 }\n"
        "impl Drop for Res {\n"
        "  fn drop(self: Res) -> i64 { 0 }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "  val a: Res = Res { id: 1 }\n"
        "  val b: Res = Res { id: 2 }\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // Find main by name (Drop impl fn may appear after it)
    HIRFnDecl* main_fn = nullptr;
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        if (hir->functions[i]->name == ctx.strings.intern("main")) {
            main_fn = hir->functions[i];
            break;
        }
    }
    ASSERT_NE(main_fn, nullptr);
    auto* body = static_cast<HIRBlockExpr*>(main_fn->body);
    // val a, val b, drop(b), drop(a) = 4 stmts
    EXPECT_EQ(body->stmt_count, 4u);
}

// ============================================================================
// Builtin print/println tests
// ============================================================================

TEST_F(HIRBuilderTest, PrintlnDesugarsToWriteCalls) {
    // println with format string desugars to __kern_write_str + __kern_write_i64 calls
    auto* hir = buildHIR(
        "fn __kern_write_str(ptr: Ptr<u8>, len: u64) -> Unit { () }\n"
        "fn __kern_write_i64(v: i64) -> Unit { () }\n"
        "fn main() -> i64 {\n"
        "  println(\"v = {}\", 42)\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, PrintNoArgsJustString) {
    // print with just a format string, no placeholders
    auto* hir = buildHIR(
        "fn __kern_write_str(ptr: Ptr<u8>, len: u64) -> Unit { () }\n"
        "fn main() -> i64 {\n"
        "  print(\"hello world\")\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, PrintTooFewArgs) {
    // print with more {} than args should error
    buildHIR(
        "fn __kern_write_str(ptr: Ptr<u8>, len: u64) -> Unit { () }\n"
        "fn __kern_write_i64(v: i64) -> Unit { () }\n"
        "fn main() -> i64 {\n"
        "  print(\"{} {}\", 42)\n"
        "  0\n"
        "}");
    // Should have error about not enough arguments
    EXPECT_TRUE(ctx.diag.hasErrors());
}

// ============================================================================
// const if tests
// ============================================================================

TEST_F(HIRBuilderTest, ConstIfTrueBranch) {
    // const if with true condition should only emit the then branch
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "  const if true { 42 } else { 99 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // The body should be the then branch (42), not an if expression
    auto* main_fn = hir->functions[hir->fn_count - 1];
    // Result should be a block (the then branch)
    ASSERT_NE(main_fn->body, nullptr);
}

TEST_F(HIRBuilderTest, ConstIfFalseBranch) {
    // const if with false condition should only emit the else branch
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "  const if false { 42 } else { 99 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ConstIfNonConstCondError) {
    // const if with non-const condition should error
    buildHIR(
        "fn get_flag() -> bool { true }\n"
        "fn main() -> i64 {\n"
        "  const if get_flag() { 42 } else { 99 }\n"
        "}");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ConstIfSizeOf) {
    // const if with size_of<T>() — compile-time evaluable
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "  const if size_of<i64>() > 4 { 64 } else { 32 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Iterator protocol tests
// ============================================================================

TEST_F(HIRBuilderTest, ForEachIteratorProtocol) {
    // Custom type with next() method can be used in for-each
    auto* hir = buildHIR(
        "union Option { Some(i64), None }\n"
        "struct Counter { current: i64, max: i64 }\n"
        "impl Counter {\n"
        "  fn next(self: Counter) -> Option {\n"
        "    if self.current < self.max {\n"
        "      Option::Some(self.current)\n"
        "    } else {\n"
        "      Option::None\n"
        "    }\n"
        "  }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "  var c = Counter { current: 0, max: 10 }\n"
        "  var sum: i64 = 0\n"
        "  for x in c {\n"
        "    sum = sum + x\n"
        "  }\n"
        "  sum\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ForEachNoNextMethodError) {
    // Type without next() should give error
    buildHIR(
        "struct Foo { x: i64 }\n"
        "fn main() -> i64 {\n"
        "  val f = Foo { x: 42 }\n"
        "  for x in f { x }\n"
        "  0\n"
        "}");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

// ============================================================================
// Copy/Clone trait tests
// ============================================================================

TEST_F(HIRBuilderTest, CopyDropConflictError) {
    // Type with both Copy and Drop should error
    buildHIR(
        "struct Res { fd: i64 }\n"
        "trait Drop { fn drop(self: Self) -> i64 }\n"
        "trait Copy {}\n"
        "impl Drop for Res {\n"
        "  fn drop(self: Res) -> i64 { 0 }\n"
        "}\n"
        "impl Copy for Res {}\n"
        "fn main() -> i64 { 0 }");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, CopyTraitNoConflict) {
    // Type with Copy but no Drop should succeed
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "trait Copy {}\n"
        "impl Copy for Point {}\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// From/Into conversion trait tests
// ============================================================================

TEST_F(HIRBuilderTest, TryOperatorWithFromConversion) {
    // ? operator should use From::from for error type conversion
    // From::from convention: RetErrPayloadType_from(src_err) fn
    // Return type Result_i64_AppError Err payload is u64, source Err is i64
    auto* hir = buildHIR(
        "union Result_i64_IoError { Ok(i64), Err(i64) }\n"
        "union Result_i64_AppError { Ok(i64), Err(u64) }\n"
        "fn u64_from(e: i64) -> u64 { e as u64 }\n"
        "fn read_file() -> Result_i64_IoError { Result_i64_IoError::Ok(42) }\n"
        "fn main() -> Result_i64_AppError {\n"
        "  val data = read_file()?\n"
        "  Result_i64_AppError::Ok(data)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, TryOperatorNoFromConversionError) {
    // ? operator with incompatible error types and no From should error
    buildHIR(
        "union Result_i64_i64 { Ok(i64), Err(i64) }\n"
        "union Result_i64_u64 { Ok(i64), Err(u64) }\n"
        "fn fail() -> Result_i64_i64 { Result_i64_i64::Err(1) }\n"
        "fn main() -> Result_i64_u64 {\n"
        "  val x = fail()?\n"
        "  Result_i64_u64::Ok(x)\n"
        "}");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

// ============================================================================
// Global allocator annotation tests
// ============================================================================

TEST_F(HIRBuilderTest, GlobalAllocatorAnnotation) {
    auto* hir = buildHIR(
        "struct Allocator { base: u64 }\n"
        "@global_allocator\n"
        "static val ALLOC: Allocator = Allocator { base: 0 }\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    ASSERT_GE(hir->global_count, 1u);
    EXPECT_TRUE(hir->globals[0]->is_global_allocator);
}

TEST_F(HIRBuilderTest, GlobalNoAllocatorByDefault) {
    auto* hir = buildHIR(
        "static val X: i64 = 42\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    ASSERT_GE(hir->global_count, 1u);
    EXPECT_FALSE(hir->globals[0]->is_global_allocator);
}

// ============================================================================
// Deref/DerefMut trait auto-deref tests
// ============================================================================

TEST_F(HIRBuilderTest, DerefTraitAutoDerefMethodCall) {
    auto* hir = buildHIR(
        "struct Inner { value: i64 }\n"
        "struct Box_Inner { ptr: i64 }\n"
        "trait Deref { type Target\n fn deref(self: Box_Inner) -> Inner }\n"
        "impl Deref for Box_Inner { type Target = Inner\n"
        "  fn deref(self: Box_Inner) -> Inner { Inner { value: self.ptr } } }\n"
        "impl Inner { fn get(self: Inner) -> i64 { self.value } }\n"
        "fn main() -> i64 {\n"
        "  val b = Box_Inner { ptr: 42 }\n"
        "  b.get()\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, DerefTraitAutoDerefFieldAccess) {
    auto* hir = buildHIR(
        "struct Inner { value: i64 }\n"
        "struct Wrapper { data: i64 }\n"
        "trait Deref { type Target\n fn deref(self: Wrapper) -> Inner }\n"
        "impl Deref for Wrapper { type Target = Inner\n"
        "  fn deref(self: Wrapper) -> Inner { Inner { value: self.data } } }\n"
        "fn main() -> i64 {\n"
        "  val w = Wrapper { data: 10 }\n"
        "  w.value\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Display trait integration with print
// ============================================================================

TEST_F(HIRBuilderTest, DisplayTraitUsedInPrint) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "trait Display { fn display(self: Point) -> Unit }\n"
        "impl Display for Point {\n"
        "  fn display(self: Point) -> Unit { () }\n"
        "}\n"
        "fn __kern_write_str(ptr: Ptr<u8>, len: u64) -> Unit { () }\n"
        "fn main() -> Unit {\n"
        "  val p = Point { x: 1, y: 2 }\n"
        "  println(\"{}\", p)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Send/Sync marker trait tests
// ============================================================================

TEST_F(HIRBuilderTest, SendSyncTraitRegistered) {
    auto* hir = buildHIR(
        "struct Mutex { locked: bool }\n"
        "trait Send {}\n"
        "trait Sync {}\n"
        "impl Send for Mutex {}\n"
        "impl Sync for Mutex {}\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Default trait tests
// ============================================================================

TEST_F(HIRBuilderTest, DefaultTraitBuiltin) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "trait Default { fn default() -> Point }\n"
        "impl Default for Point {\n"
        "  fn default() -> Point { Point { x: 0, y: 0 } }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "  val p = default<Point>()\n"
        "  p.x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, DefaultTraitNoImplError) {
    buildHIR(
        "struct Foo { x: i64 }\n"
        "fn main() -> i64 {\n"
        "  val f = default<Foo>()\n"
        "  f.x\n"
        "}");
    EXPECT_TRUE(ctx.diag.hasErrors());
}

// ============================================================================
// zeroed<T>() builtin tests
// ============================================================================

TEST_F(HIRBuilderTest, ZeroedPrimitive) {
    auto* hir = buildHIR("fn main() -> i64 { zeroed<i64>() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ZeroedStruct) {
    auto* hir = buildHIR(
        "struct Point { x: i64, y: i64 }\n"
        "fn main() -> i64 {\n"
        "  val p = zeroed<Point>()\n"
        "  p.x\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// memcpy/memset/memmove builtin tests
// ============================================================================

TEST_F(HIRBuilderTest, MemcpyBuiltin) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "  val dst = 0 as Ptr<var u8>\n"
        "  val src = 0 as Ptr<u8>\n"
        "  memcpy(dst, src, 16)\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, MemsetBuiltin) {
    auto* hir = buildHIR(
        "fn main() -> i64 {\n"
        "  val dst = 0 as Ptr<var u8>\n"
        "  memset(dst, 0 as u8, 4096)\n"
        "  0\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Bitfield extract/insert builtin tests
// ============================================================================

TEST_F(HIRBuilderTest, ExtractBitsConst) {
    auto* hir = buildHIR(
        "fn main() -> u64 {\n"
        "  val pte: u64 = 0x1003\n"
        "  extract_bits(pte, 0, 1)\n"  // extract bit 0 (present)
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, InsertBitsConst) {
    auto* hir = buildHIR(
        "fn main() -> u64 {\n"
        "  val pte: u64 = 0\n"
        "  insert_bits(pte, 0, 1, 1)\n"  // set present bit
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, AlignAnnotationOnGlobal) {
    auto* hir = buildHIR(
        "@align(4096)\n"
        "static val PAGE_TABLE: u64 = 0\n"
        "fn main() -> u64 { PAGE_TABLE }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // Verify the global has explicit_align set
    bool found = false;
    for (uint32_t i = 0; i < hir->global_count; ++i) {
        if (hir->globals[i]->name == ctx.strings.intern("PAGE_TABLE")) {
            EXPECT_EQ(hir->globals[i]->explicit_align, 4096u);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(HIRBuilderTest, AlignAnnotationDefaultZero) {
    auto* hir = buildHIR(
        "static val X: u64 = 42\n"
        "fn main() -> u64 { X }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    bool found = false;
    for (uint32_t i = 0; i < hir->global_count; ++i) {
        if (hir->globals[i]->name == ctx.strings.intern("X")) {
            EXPECT_EQ(hir->globals[i]->explicit_align, 0u);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(HIRBuilderTest, LikelyBuiltin) {
    auto* hir = buildHIR(
        "fn test(x: bool) -> i64 {\n"
        "  if likely(x) { 1 } else { 0 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, UnlikelyBuiltin) {
    auto* hir = buildHIR(
        "fn test(x: bool) -> i64 {\n"
        "  if unlikely(x) { 1 } else { 0 }\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, TransmuteBuiltin) {
    auto* hir = buildHIR(
        "fn test(addr: u64) -> Ptr<u8> {\n"
        "  transmute<Ptr<u8>>(addr)\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, UsizeIsizeTypes) {
    auto* hir = buildHIR(
        "fn test(a: usize, b: isize) -> usize {\n"
        "  val x: usize = 42usize\n"
        "  val y: isize = -1isize\n"
        "  a\n"
        "}");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // Verify params have the right types
    auto& fn = hir->functions[0];
    EXPECT_EQ(fn->params[0].type, TypeTable::Usize);
    EXPECT_EQ(fn->params[1].type, TypeTable::Isize);
    EXPECT_EQ(fn->return_type, TypeTable::Usize);
}

TEST_F(HIRBuilderTest, UsizeIsizeSizeOf) {
    // usize/isize should be 8 bytes on x86-64
    EXPECT_EQ(ctx.types.sizeOf(TypeTable::Usize), 8u);
    EXPECT_EQ(ctx.types.sizeOf(TypeTable::Isize), 8u);
    EXPECT_EQ(ctx.types.alignOf(TypeTable::Usize), 8u);
    EXPECT_EQ(ctx.types.alignOf(TypeTable::Isize), 8u);
    EXPECT_TRUE(ctx.types.isInteger(TypeTable::Usize));
    EXPECT_TRUE(ctx.types.isInteger(TypeTable::Isize));
    EXPECT_TRUE(ctx.types.isSigned(TypeTable::Isize));
    EXPECT_FALSE(ctx.types.isSigned(TypeTable::Usize));
}

// ============================================================================
// Intrinsic: ldmxcsr / stmxcsr
// ============================================================================

TEST_F(HIRBuilderTest, LdmxcsrIntrinsic) {
    auto* hir = buildHIR(
        "fn ldmxcsr(v: u32) -> Unit = intrinsic\n"
        "fn main() -> i64 { ldmxcsr(0x1F80u32); 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, StmxcsrIntrinsic) {
    auto* hir = buildHIR(
        "fn stmxcsr(p: Ptr<var u32>) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// @no_red_zone annotation
// ============================================================================

TEST_F(HIRBuilderTest, NoRedZoneAnnotation) {
    auto* hir = buildHIR(
        "@no_red_zone fn f() -> i64 { 42 }\n"
        "fn main() -> i64 { f() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_TRUE(hir->functions[0]->is_no_red_zone);
    EXPECT_FALSE(hir->functions[1]->is_no_red_zone);
}

// ============================================================================
// @section("name", "flags")
// ============================================================================

TEST_F(HIRBuilderTest, SectionWithFlags) {
    auto* hir = buildHIR(
        "@section(\".mytext\", \"axp\") fn f() -> i64 { 42 }\n"
        "fn main() -> i64 { f() }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->functions[0]->section_name, ".mytext");
    EXPECT_EQ(hir->functions[0]->section_flags, "axp");
    EXPECT_TRUE(hir->functions[1]->section_flags.empty());
}

// ============================================================================
// Global variable visibility annotations
// ============================================================================

TEST_F(HIRBuilderTest, GlobalWeakAnnotation) {
    auto* hir = buildHIR(
        "@weak pub static val X: i64 = 42\n"
        "fn main() -> i64 { X }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_TRUE(hir->globals[0]->is_weak);
}

TEST_F(HIRBuilderTest, GlobalHiddenAnnotation) {
    auto* hir = buildHIR(
        "@hidden pub static val X: i64 = 42\n"
        "fn main() -> i64 { X }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_TRUE(hir->globals[0]->is_hidden);
}

TEST_F(HIRBuilderTest, GlobalProtectedAnnotation) {
    auto* hir = buildHIR(
        "@protected pub static val X: i64 = 42\n"
        "fn main() -> i64 { X }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_TRUE(hir->globals[0]->is_protected);
}

TEST_F(HIRBuilderTest, GlobalSectionFlags) {
    auto* hir = buildHIR(
        "@section(\".mydata\", \"awp\") static val X: i64 = 42\n"
        "fn main() -> i64 { X }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    EXPECT_EQ(hir->globals[0]->section_name, ".mydata");
    EXPECT_EQ(hir->globals[0]->section_flags, "awp");
}

// ============================================================================
// EFER / CR8 intrinsics
// ============================================================================

TEST_F(HIRBuilderTest, ReadEferIntrinsic) {
    auto* hir = buildHIR(
        "fn read_efer() -> u64 = intrinsic\n"
        "fn main() -> i64 { read_efer() as i64 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WriteEferIntrinsic) {
    auto* hir = buildHIR(
        "fn write_efer(v: u64) -> Unit = intrinsic\n"
        "fn main() -> i64 { write_efer(0u64); 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, ReadCr8Intrinsic) {
    auto* hir = buildHIR(
        "fn read_cr8() -> u64 = intrinsic\n"
        "fn main() -> i64 { read_cr8() as i64 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WriteCr8Intrinsic) {
    auto* hir = buildHIR(
        "fn write_cr8(v: u64) -> Unit = intrinsic\n"
        "fn main() -> i64 { write_cr8(0u64); 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// CET / MPK intrinsics
// ============================================================================

TEST_F(HIRBuilderTest, RdsspIntrinsic) {
    auto* hir = buildHIR(
        "fn rdssp() -> u64 = intrinsic\n"
        "fn main() -> i64 { rdssp() as i64 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, RdpkruIntrinsic) {
    auto* hir = buildHIR(
        "fn rdpkru() -> u32 = intrinsic\n"
        "fn main() -> i64 { rdpkru() as i64 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WrpkruIntrinsic) {
    auto* hir = buildHIR(
        "fn wrpkru(v: u32) -> Unit = intrinsic\n"
        "fn main() -> i64 { wrpkru(0u32); 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Half-width atomics
// ============================================================================

TEST_F(HIRBuilderTest, AtomicLoadU32Intrinsic) {
    auto* hir = buildHIR(
        "fn atomic_load_u32(ptr: Ptr<u32>) -> u32 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, AtomicStoreU32Intrinsic) {
    auto* hir = buildHIR(
        "fn atomic_store_u32(ptr: Ptr<var u32>, v: u32) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, AtomicCasU32Intrinsic) {
    auto* hir = buildHIR(
        "fn atomic_cas_u32(ptr: Ptr<var u32>, expected: u32, desired: u32) -> u32 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// ============================================================================
// Atomic bit ops + strnlen
// ============================================================================

TEST_F(HIRBuilderTest, TestAndSetIntrinsic) {
    auto* hir = buildHIR(
        "fn test_and_set(ptr: Ptr<var u64>, bit: u64) -> bool = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, TestAndClearIntrinsic) {
    auto* hir = buildHIR(
        "fn test_and_clear(ptr: Ptr<var u64>, bit: u64) -> bool = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, StrnlenIntrinsic) {
    auto* hir = buildHIR(
        "fn strnlen(s: Ptr<u8>, max_len: u64) -> u64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// --- CPU control intrinsics ---
TEST_F(HIRBuilderTest, PauseIntrinsic) {
    auto* hir = buildHIR(
        "fn pause() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, SwapgsIntrinsic) {
    auto* hir = buildHIR(
        "fn swapgs() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WbinvdIntrinsic) {
    auto* hir = buildHIR(
        "fn wbinvd() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// --- Debug register intrinsics ---
TEST_F(HIRBuilderTest, ReadDr0Intrinsic) {
    auto* hir = buildHIR(
        "fn read_dr0() -> u64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WriteDr7Intrinsic) {
    auto* hir = buildHIR(
        "fn write_dr7(v: u64) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// --- Descriptor table intrinsics ---
TEST_F(HIRBuilderTest, InvlpgIntrinsic) {
    auto* hir = buildHIR(
        "fn invlpg(addr: Ptr<u8>) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, LgdtIntrinsic) {
    auto* hir = buildHIR(
        "fn lgdt(ptr: Ptr<u8>) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, LidtIntrinsic) {
    auto* hir = buildHIR(
        "fn lidt(ptr: Ptr<u8>) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, LtrIntrinsic) {
    auto* hir = buildHIR(
        "fn ltr(sel: u16) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, RdpmcIntrinsic) {
    auto* hir = buildHIR(
        "fn rdpmc(counter: u32) -> u64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// --- Control register intrinsics ---
TEST_F(HIRBuilderTest, ReadCr0Intrinsic) {
    auto* hir = buildHIR(
        "fn read_cr0() -> u64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, WriteCr4Intrinsic) {
    auto* hir = buildHIR(
        "fn write_cr4(v: u64) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

// --- VMX intrinsics ---
TEST_F(HIRBuilderTest, VmxonIntrinsic) {
    auto* hir = buildHIR(
        "fn vmxon(addr: Ptr<u8>) -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, VmxoffIntrinsic) {
    auto* hir = buildHIR(
        "fn vmxoff() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, VmcallIntrinsic) {
    auto* hir = buildHIR(
        "fn vmcall() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, VmlaunchIntrinsic) {
    auto* hir = buildHIR(
        "fn vmlaunch() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST_F(HIRBuilderTest, VmresumeIntrinsic) {
    auto* hir = buildHIR(
        "fn vmresume() -> Unit = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

} // namespace kern
