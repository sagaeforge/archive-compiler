#include "kern/support/CompilationContext.h"
#include <gtest/gtest.h>

using namespace kern;

TEST(CompilationContextTest, DefaultConstruction) {
    CompilationContext ctx;
    // Primitives should be pre-registered
    EXPECT_EQ(ctx.types.size(), TypeTable::PRIMITIVE_COUNT);
    // StringPool starts empty
    EXPECT_EQ(ctx.strings.size(), 0);
    // No errors
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST(CompilationContextTest, StringPoolUsesSharedArena) {
    CompilationContext ctx;
    auto s = ctx.strings.intern("hello");
    EXPECT_EQ(s, "hello");
    // Interning again returns same pointer (Arena is shared)
    auto s2 = ctx.strings.intern("hello");
    EXPECT_EQ(s.data(), s2.data());
}

TEST(CompilationContextTest, TypeTableUsesSharedArena) {
    CompilationContext ctx;
    auto ptr_id = ctx.types.makePtr(TypeTable::I64, false);
    EXPECT_EQ(ctx.types.get(ptr_id).kind, TypeKind::Ptr);
}

TEST(CompilationContextTest, DiagnosticReportsErrors) {
    CompilationContext ctx;
    EXPECT_FALSE(ctx.diag.hasErrors());
    ctx.diag.error({0, 0, {}}, "test error");
    EXPECT_TRUE(ctx.diag.hasErrors());
    EXPECT_EQ(ctx.diag.diagnostics().size(), 1);
}

TEST(CompilationContextTest, AllComponentsWorkTogether) {
    CompilationContext ctx;

    // Intern a struct name
    auto name = ctx.strings.intern("Point");

    // Create a struct type
    FieldInfo fields[] = {
        {ctx.strings.intern("x"), TypeTable::F64, false, -1},
        {ctx.strings.intern("y"), TypeTable::F64, false, -1},
    };
    auto struct_id = ctx.types.makeStruct(name, fields);

    // Verify
    const auto& ti = ctx.types.get(struct_id);
    EXPECT_EQ(ti.struct_.name, "Point");
    EXPECT_EQ(ti.struct_.field_count, 2);
    EXPECT_EQ(ctx.types.sizeOf(struct_id), 16);

    // Arena allocated everything — no leaks to worry about
    EXPECT_FALSE(ctx.diag.hasErrors());
}

TEST(CompilationContextTest, ArenaAllocatesNodes) {
    CompilationContext ctx;
    // Allocate something via the arena directly
    auto* val = ctx.arena.make<int>(42);
    EXPECT_EQ(*val, 42);
}
