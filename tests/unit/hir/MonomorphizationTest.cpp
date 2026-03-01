#include "kern/hir/HIRBuilder.h"
#include "kern/hir/MonomorphizationPass.h"
#include "kern/hir/HIRDump.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class MonomorphizationTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    Module* parse(const char* source) {
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        return parser.parseModule();
    }

    HIRModule* buildAndMono(const char* source) {
        auto* ast = parse(source);
        EXPECT_NE(ast, nullptr);
        HIRBuilder builder(ctx);
        auto* hir = builder.build(ast);
        if (!hir || ctx.diag.hasErrors()) return nullptr;
        MonomorphizationPass mono(ctx);
        return mono.run(hir);
    }

    std::string dumpToString(const HIRModule* mod) {
        std::ostringstream out;
        dumpHIR(mod, ctx.types, out);
        return out.str();
    }
};

TEST_F(MonomorphizationTest, SingleTypeParam) {
    auto* hir = buildAndMono(R"(
        fn identity<T>(x: T) -> T { x }
        fn main() -> i64 { identity(42) }
    )");
    ASSERT_NE(hir, nullptr);
    auto dump = dumpToString(hir);
    // Should have identity_i64, not generic identity
    EXPECT_NE(dump.find("identity_i64"), std::string::npos);
    EXPECT_EQ(dump.find("fn identity "), std::string::npos);
}

TEST_F(MonomorphizationTest, MultipleInstantiations) {
    auto* hir = buildAndMono(R"(
        fn first<T>(x: T, y: T) -> T { x }
        fn main() -> i64 {
            val a: i32 = first(1 as i32, 2 as i32)
            first(40, 2) + (a as i64)
        }
    )");
    ASSERT_NE(hir, nullptr);
    auto dump = dumpToString(hir);
    EXPECT_NE(dump.find("first_i64"), std::string::npos);
    EXPECT_NE(dump.find("first_i32"), std::string::npos);
}

TEST_F(MonomorphizationTest, GenericWithUnion) {
    auto* hir = buildAndMono(R"(
        union Option<T> { None, Some(T) }
        fn unwrap_or<T>(opt: Option<T>, fallback: T) -> T {
            match opt { None => fallback, Some(v) => v }
        }
        fn main() -> i64 {
            val a: Option<i64> = Option<i64>::Some(42)
            unwrap_or(a, 0)
        }
    )");
    ASSERT_NE(hir, nullptr);
    auto dump = dumpToString(hir);
    EXPECT_NE(dump.find("unwrap_or_i64"), std::string::npos);
}

TEST_F(MonomorphizationTest, TwoTypeParams) {
    auto* hir = buildAndMono(R"(
        union Result<T, E> { Ok(T), Err(E) }
        fn unwrap_ok<T, E>(r: Result<T, E>, fallback: T) -> T {
            match r { Ok(v) => v, Err(_) => fallback }
        }
        fn main() -> i64 {
            val ok: Result<i64, i64> = Result<i64, i64>::Ok(42)
            unwrap_ok(ok, 0)
        }
    )");
    ASSERT_NE(hir, nullptr);
    auto dump = dumpToString(hir);
    EXPECT_NE(dump.find("unwrap_ok_i64_i64"), std::string::npos);
}

TEST_F(MonomorphizationTest, GenericOnlyContainingTypeVar) {
    // Test case where TypeVar is only inside a parametric type (not a direct param)
    auto* hir = buildAndMono(R"(
        union Option<T> { None, Some(T) }
        fn is_some<T>(opt: Option<T>) -> bool {
            match opt { None => false, Some(_) => true }
        }
        fn main() -> i64 {
            val a: Option<i64> = Option<i64>::Some(42)
            if is_some(a) { 42 } else { 0 }
        }
    )");
    ASSERT_NE(hir, nullptr);
    auto dump = dumpToString(hir);
    EXPECT_NE(dump.find("is_some_i64"), std::string::npos);
}

TEST_F(MonomorphizationTest, GenericFunctionPreservedCorrectly) {
    // After monomorphization, generic functions should be removed
    auto* hir = buildAndMono(R"(
        fn id<T>(x: T) -> T { x }
        fn main() -> i64 { id(42) }
    )");
    ASSERT_NE(hir, nullptr);
    // Only main and id_i64 should remain, not generic id
    uint32_t fn_count = 0;
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        if (hir->functions[i]->type_param_count == 0) fn_count++;
    }
    EXPECT_EQ(fn_count, hir->fn_count); // All remaining fns should be non-generic
}

TEST_F(MonomorphizationTest, NoGenericNoChange) {
    auto* hir = buildAndMono(R"(
        fn add(a: i64, b: i64) -> i64 { a + b }
        fn main() -> i64 { add(40, 2) }
    )");
    ASSERT_NE(hir, nullptr);
    EXPECT_EQ(hir->fn_count, 2u); // add + main
}

TEST_F(MonomorphizationTest, GenericWithBoolInstantiation) {
    auto* hir = buildAndMono(R"(
        union Option<T> { None, Some(T) }
        fn is_some<T>(opt: Option<T>) -> bool {
            match opt { None => false, Some(_) => true }
        }
        fn main() -> i64 {
            val a: Option<bool> = Option<bool>::Some(true)
            if is_some(a) { 42 } else { 0 }
        }
    )");
    ASSERT_NE(hir, nullptr);
    auto dump = dumpToString(hir);
    EXPECT_NE(dump.find("is_some_bool"), std::string::npos);
}

TEST_F(MonomorphizationTest, ConstGenericStruct) {
    auto* hir = buildAndMono(R"(
        struct Buffer<T, const N: u64> { data: [T; N], len: i64 }
        fn main() -> i64 {
            val b: Buffer<i64, 4> = Buffer<i64, 4> { data: [1, 2, 3, 4], len: 4 }
            b.len
        }
    )");
    ASSERT_NE(hir, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
}

} // namespace kern
