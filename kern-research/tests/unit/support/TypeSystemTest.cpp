#include "kern/support/TypeSystem.h"
#include <gtest/gtest.h>

using namespace kern;

class TypeSystemTest : public ::testing::Test {
protected:
    Arena arena;
};

// --- Primitive registration ---

TEST_F(TypeSystemTest, PrimitivesPreRegistered) {
    TypeTable tt(arena);
    EXPECT_EQ(tt.size(), TypeTable::PRIMITIVE_COUNT);
}

TEST_F(TypeSystemTest, PrimitiveI64) {
    TypeTable tt(arena);
    const auto& ti = tt.get(TypeTable::I64);
    EXPECT_EQ(ti.kind, TypeKind::Primitive);
    EXPECT_EQ(ti.primitive.prim, PrimitiveKind::I64);
}

TEST_F(TypeSystemTest, PrimitiveBool) {
    TypeTable tt(arena);
    const auto& ti = tt.get(TypeTable::Bool);
    EXPECT_EQ(ti.kind, TypeKind::Primitive);
    EXPECT_EQ(ti.primitive.prim, PrimitiveKind::Bool);
}

TEST_F(TypeSystemTest, PrimitiveUnit) {
    TypeTable tt(arena);
    const auto& ti = tt.get(TypeTable::Unit);
    EXPECT_EQ(ti.kind, TypeKind::Primitive);
    EXPECT_EQ(ti.primitive.prim, PrimitiveKind::Unit);
}

// --- sizeOf / alignOf / bitWidth ---

TEST_F(TypeSystemTest, SizeOfPrimitives) {
    TypeTable tt(arena);
    EXPECT_EQ(tt.sizeOf(TypeTable::I8), 1);
    EXPECT_EQ(tt.sizeOf(TypeTable::I16), 2);
    EXPECT_EQ(tt.sizeOf(TypeTable::I32), 4);
    EXPECT_EQ(tt.sizeOf(TypeTable::I64), 8);
    EXPECT_EQ(tt.sizeOf(TypeTable::U8), 1);
    EXPECT_EQ(tt.sizeOf(TypeTable::U16), 2);
    EXPECT_EQ(tt.sizeOf(TypeTable::U32), 4);
    EXPECT_EQ(tt.sizeOf(TypeTable::U64), 8);
    EXPECT_EQ(tt.sizeOf(TypeTable::F32), 4);
    EXPECT_EQ(tt.sizeOf(TypeTable::F64), 8);
    EXPECT_EQ(tt.sizeOf(TypeTable::Bool), 1);
    EXPECT_EQ(tt.sizeOf(TypeTable::Unit), 0);
    EXPECT_EQ(tt.sizeOf(TypeTable::Error), 0);
}

TEST_F(TypeSystemTest, AlignOfPrimitives) {
    TypeTable tt(arena);
    EXPECT_EQ(tt.alignOf(TypeTable::I8), 1);
    EXPECT_EQ(tt.alignOf(TypeTable::I16), 2);
    EXPECT_EQ(tt.alignOf(TypeTable::I32), 4);
    EXPECT_EQ(tt.alignOf(TypeTable::I64), 8);
    EXPECT_EQ(tt.alignOf(TypeTable::F32), 4);
    EXPECT_EQ(tt.alignOf(TypeTable::F64), 8);
    EXPECT_EQ(tt.alignOf(TypeTable::Bool), 1);
}

TEST_F(TypeSystemTest, BitWidthPrimitives) {
    TypeTable tt(arena);
    EXPECT_EQ(tt.bitWidth(TypeTable::I8), 8);
    EXPECT_EQ(tt.bitWidth(TypeTable::I32), 32);
    EXPECT_EQ(tt.bitWidth(TypeTable::I64), 64);
}

// --- Type queries ---

TEST_F(TypeSystemTest, IsFloat) {
    TypeTable tt(arena);
    EXPECT_TRUE(tt.isFloat(TypeTable::F32));
    EXPECT_TRUE(tt.isFloat(TypeTable::F64));
    EXPECT_FALSE(tt.isFloat(TypeTable::I64));
    EXPECT_FALSE(tt.isFloat(TypeTable::Bool));
}

TEST_F(TypeSystemTest, IsSigned) {
    TypeTable tt(arena);
    EXPECT_TRUE(tt.isSigned(TypeTable::I8));
    EXPECT_TRUE(tt.isSigned(TypeTable::I64));
    EXPECT_TRUE(tt.isSigned(TypeTable::F64));
    EXPECT_FALSE(tt.isSigned(TypeTable::U8));
    EXPECT_FALSE(tt.isSigned(TypeTable::U64));
    EXPECT_FALSE(tt.isSigned(TypeTable::Bool));
}

TEST_F(TypeSystemTest, IsInteger) {
    TypeTable tt(arena);
    EXPECT_TRUE(tt.isInteger(TypeTable::I8));
    EXPECT_TRUE(tt.isInteger(TypeTable::U64));
    EXPECT_FALSE(tt.isInteger(TypeTable::F32));
    EXPECT_FALSE(tt.isInteger(TypeTable::Bool));
    EXPECT_FALSE(tt.isInteger(TypeTable::Unit));
}

TEST_F(TypeSystemTest, IsPrimitive) {
    TypeTable tt(arena);
    for (TypeId i = 0; i < TypeTable::PRIMITIVE_COUNT; ++i) {
        EXPECT_TRUE(tt.isPrimitive(i));
    }
}

TEST_F(TypeSystemTest, Name) {
    TypeTable tt(arena);
    EXPECT_STREQ(tt.name(TypeTable::I64), "i64");
    EXPECT_STREQ(tt.name(TypeTable::F32), "f32");
    EXPECT_STREQ(tt.name(TypeTable::Bool), "bool");
    EXPECT_STREQ(tt.name(TypeTable::Unit), "Unit");
    EXPECT_STREQ(tt.name(TypeTable::Error), "Error");
}

// --- Compound types ---

TEST_F(TypeSystemTest, MakePtr) {
    TypeTable tt(arena);
    auto ptr_id = tt.makePtr(TypeTable::I64, false);
    EXPECT_EQ(ptr_id, TypeTable::PRIMITIVE_COUNT);

    const auto& ti = tt.get(ptr_id);
    EXPECT_EQ(ti.kind, TypeKind::Ptr);
    EXPECT_EQ(ti.ptr.pointee, TypeTable::I64);
    EXPECT_FALSE(ti.ptr.is_mutable);
    EXPECT_EQ(tt.sizeOf(ptr_id), 8);
    EXPECT_EQ(tt.alignOf(ptr_id), 8);
}

TEST_F(TypeSystemTest, MakePtrMut) {
    TypeTable tt(arena);
    auto ptr_id = tt.makePtr(TypeTable::I32, true);
    const auto& ti = tt.get(ptr_id);
    EXPECT_EQ(ti.kind, TypeKind::PtrMut);
    EXPECT_TRUE(ti.ptr.is_mutable);
}

TEST_F(TypeSystemTest, MakeFn) {
    TypeTable tt(arena);
    TypeId params[] = {TypeTable::I64, TypeTable::I32};
    auto fn_id = tt.makeFn(params, TypeTable::Bool);

    const auto& ti = tt.get(fn_id);
    EXPECT_EQ(ti.kind, TypeKind::Fn);
    EXPECT_EQ(ti.fn.param_count, 2);
    EXPECT_EQ(ti.fn.params[0], TypeTable::I64);
    EXPECT_EQ(ti.fn.params[1], TypeTable::I32);
    EXPECT_EQ(ti.fn.return_type, TypeTable::Bool);
    EXPECT_EQ(tt.sizeOf(fn_id), 8);
}

TEST_F(TypeSystemTest, MakeFnNoParams) {
    TypeTable tt(arena);
    auto fn_id = tt.makeFn({}, TypeTable::Unit);
    const auto& ti = tt.get(fn_id);
    EXPECT_EQ(ti.fn.param_count, 0);
    EXPECT_EQ(ti.fn.return_type, TypeTable::Unit);
}

TEST_F(TypeSystemTest, MakeStruct) {
    TypeTable tt(arena);
    FieldInfo fields[] = {
        {"x", TypeTable::I64, false, -1},
        {"y", TypeTable::I64, false, -1},
    };
    auto s_id = tt.makeStruct("Point", fields);

    const auto& ti = tt.get(s_id);
    EXPECT_EQ(ti.kind, TypeKind::Struct);
    EXPECT_EQ(ti.struct_.name, "Point");
    EXPECT_EQ(ti.struct_.field_count, 2);
    EXPECT_EQ(ti.struct_.fields[0].name, "x");
    EXPECT_EQ(ti.struct_.fields[0].offset, 0);
    EXPECT_EQ(ti.struct_.fields[1].name, "y");
    EXPECT_EQ(ti.struct_.fields[1].offset, 8);
    EXPECT_EQ(tt.sizeOf(s_id), 16);
    EXPECT_EQ(tt.alignOf(s_id), 8);
}

TEST_F(TypeSystemTest, MakeStructMixedTypes) {
    TypeTable tt(arena);
    FieldInfo fields[] = {
        {"a", TypeTable::I8, false, -1},
        {"b", TypeTable::I64, false, -1},
        {"c", TypeTable::I8, false, -1},
    };
    auto s_id = tt.makeStruct("Mixed", fields);

    const auto& ti = tt.get(s_id);
    // a at 0, b at 8 (aligned to 8), c at 16
    EXPECT_EQ(ti.struct_.fields[0].offset, 0);
    EXPECT_EQ(ti.struct_.fields[1].offset, 8);
    EXPECT_EQ(ti.struct_.fields[2].offset, 16);
    EXPECT_EQ(tt.sizeOf(s_id), 24);  // padded to align 8
    EXPECT_EQ(tt.alignOf(s_id), 8);
}

TEST_F(TypeSystemTest, MakeEnum) {
    TypeTable tt(arena);
    std::string_view names[] = {"Red", "Green", "Blue"};
    int64_t values[] = {0, 1, 2};
    auto e_id = tt.makeEnum("Color", names, values);

    const auto& ti = tt.get(e_id);
    EXPECT_EQ(ti.kind, TypeKind::Enum);
    EXPECT_EQ(ti.enum_.name, "Color");
    EXPECT_EQ(ti.enum_.variant_count, 3);
    EXPECT_EQ(ti.enum_.names[0], "Red");
    EXPECT_EQ(ti.enum_.values[2], 2);
    EXPECT_EQ(tt.sizeOf(e_id), 8);
}

TEST_F(TypeSystemTest, MakeUnion) {
    TypeTable tt(arena);
    VariantInfo variants[] = {
        {"Circle", TypeTable::I64},
        {"Square", TypeTable::I64},
        {"Empty", INVALID_TYPE},
    };
    auto u_id = tt.makeUnion("Shape", variants);

    const auto& ti = tt.get(u_id);
    EXPECT_EQ(ti.kind, TypeKind::Union);
    EXPECT_EQ(ti.union_.name, "Shape");
    EXPECT_EQ(ti.union_.variant_count, 3);
    EXPECT_EQ(ti.union_.variants[0].name, "Circle");
    EXPECT_EQ(ti.union_.variants[2].payload_type, INVALID_TYPE);
    // tag(8) + payload(8) = 16
    EXPECT_EQ(tt.sizeOf(u_id), 16);
}

TEST_F(TypeSystemTest, NonPrimitiveNotPrimitive) {
    TypeTable tt(arena);
    auto ptr_id = tt.makePtr(TypeTable::I64, false);
    EXPECT_FALSE(tt.isPrimitive(ptr_id));
    EXPECT_FALSE(tt.isFloat(ptr_id));
    EXPECT_FALSE(tt.isSigned(ptr_id));
    EXPECT_FALSE(tt.isInteger(ptr_id));
}

TEST_F(TypeSystemTest, AddCustomType) {
    TypeTable tt(arena);
    TypeInfo ti{};
    ti.kind = TypeKind::Array;
    ti.array.element = TypeTable::I32;
    ti.array.count = 10;
    auto arr_id = tt.add(ti);

    EXPECT_EQ(tt.sizeOf(arr_id), 40);  // 10 * 4
    EXPECT_EQ(tt.alignOf(arr_id), 4);
}

TEST_F(TypeSystemTest, MultiplePtrTypes) {
    TypeTable tt(arena);
    auto p1 = tt.makePtr(TypeTable::I64, false);
    auto p2 = tt.makePtr(TypeTable::I64, true);
    auto p3 = tt.makePtr(TypeTable::I32, false);

    EXPECT_NE(p1, p2);
    EXPECT_NE(p1, p3);
    // All pointers are 8 bytes
    EXPECT_EQ(tt.sizeOf(p1), 8);
    EXPECT_EQ(tt.sizeOf(p2), 8);
    EXPECT_EQ(tt.sizeOf(p3), 8);
}

// ============================================================================
// EffectSet tests
// ============================================================================

TEST(EffectSetTest, EmptyIsNone) {
    EXPECT_EQ(EFFECT_NONE, 0);
}

TEST(EffectSetTest, SingleEffects) {
    EXPECT_EQ(EFFECT_MUT, 1);
    EXPECT_EQ(EFFECT_MEM, 2);
    EXPECT_EQ(EFFECT_IO, 4);
    EXPECT_EQ(EFFECT_ATOMIC, 8);
}

TEST(EffectSetTest, HasEffect) {
    EffectSet set = EFFECT_IO | EFFECT_ATOMIC;
    EXPECT_TRUE(hasEffect(set, Effect::IO));
    EXPECT_TRUE(hasEffect(set, Effect::Atomic));
    EXPECT_FALSE(hasEffect(set, Effect::Mut));
    EXPECT_FALSE(hasEffect(set, Effect::Mem));
}

TEST(EffectSetTest, AddEffect) {
    EffectSet set = EFFECT_NONE;
    set = addEffect(set, Effect::IO);
    EXPECT_TRUE(hasEffect(set, Effect::IO));
    EXPECT_FALSE(hasEffect(set, Effect::Mut));
    set = addEffect(set, Effect::Mut);
    EXPECT_TRUE(hasEffect(set, Effect::IO));
    EXPECT_TRUE(hasEffect(set, Effect::Mut));
}

TEST(EffectSetTest, UnionEffects) {
    EffectSet a = EFFECT_IO | EFFECT_MUT;
    EffectSet b = EFFECT_MEM | EFFECT_ATOMIC;
    EffectSet u = unionEffects(a, b);
    EXPECT_TRUE(hasEffect(u, Effect::IO));
    EXPECT_TRUE(hasEffect(u, Effect::Mut));
    EXPECT_TRUE(hasEffect(u, Effect::Mem));
    EXPECT_TRUE(hasEffect(u, Effect::Atomic));
}

TEST(EffectSetTest, Subset) {
    EffectSet sub = EFFECT_IO;
    EffectSet super = EFFECT_IO | EFFECT_MEM;
    EXPECT_TRUE(effectSubset(sub, super));
    EXPECT_FALSE(effectSubset(super, sub));
    EXPECT_TRUE(effectSubset(EFFECT_NONE, super));
    EXPECT_TRUE(effectSubset(sub, sub));
}

TEST(EffectSetTest, ParseEffectName) {
    Effect e;
    EXPECT_TRUE(parseEffectName("io", e));
    EXPECT_EQ(e, Effect::IO);
    EXPECT_TRUE(parseEffectName("mut", e));
    EXPECT_EQ(e, Effect::Mut);
    EXPECT_TRUE(parseEffectName("mem", e));
    EXPECT_EQ(e, Effect::Mem);
    EXPECT_TRUE(parseEffectName("atomic", e));
    EXPECT_EQ(e, Effect::Atomic);
    EXPECT_FALSE(parseEffectName("unknown", e));
}

TEST(EffectSetTest, EffectName) {
    EXPECT_STREQ(effectName(Effect::Mut), "mut");
    EXPECT_STREQ(effectName(Effect::Mem), "mem");
    EXPECT_STREQ(effectName(Effect::IO), "io");
    EXPECT_STREQ(effectName(Effect::Atomic), "atomic");
}

TEST(EffectSetTest, EffectSetString) {
    EXPECT_EQ(effectSetString(EFFECT_NONE), "pure");
    EXPECT_EQ(effectSetString(EFFECT_IO), "io");
    EXPECT_EQ(effectSetString(EFFECT_IO | EFFECT_ATOMIC), "io, atomic");
    EXPECT_EQ(effectSetString(EFFECT_MUT | EFFECT_MEM | EFFECT_IO | EFFECT_ATOMIC),
              "mut, mem, io, atomic");
}

TEST_F(TypeSystemTest, FnTypeWithEffects) {
    TypeTable tt(arena);
    TypeId params[] = {TypeTable::I64};
    TypeId fn1 = tt.makeFn(params, TypeTable::I64);
    TypeId fn2 = tt.makeFn(params, TypeTable::I64, EFFECT_IO);
    TypeId fn3 = tt.makeFn(params, TypeTable::I64, EFFECT_IO);
    // Different effects → different types
    EXPECT_NE(fn1, fn2);
    // Same effects → same type (dedup)
    EXPECT_EQ(fn2, fn3);
    // Verify effect is stored
    EXPECT_EQ(tt.get(fn1).fn.effects, EFFECT_NONE);
    EXPECT_EQ(tt.get(fn2).fn.effects, EFFECT_IO);
}
