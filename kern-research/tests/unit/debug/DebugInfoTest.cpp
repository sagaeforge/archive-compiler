#include <gtest/gtest.h>
#include "kern/debug/DebugInfo.h"

using namespace kern;

// ============================================================================
// DebugInfo lookup tests
// ============================================================================

TEST(DebugInfoTest, FindMappingEmpty) {
    DebugInfo info;
    EXPECT_EQ(info.findMapping(0), nullptr);
    EXPECT_EQ(info.findMapping(100), nullptr);
}

TEST(DebugInfoTest, FindMappingSingle) {
    DebugInfo info;
    info.source_map.push_back({10, 1, 1, "test.kern"});

    EXPECT_EQ(info.findMapping(5), nullptr);  // before first mapping
    EXPECT_NE(info.findMapping(10), nullptr);
    EXPECT_EQ(info.findMapping(10)->line, 1u);
    EXPECT_NE(info.findMapping(100), nullptr);
    EXPECT_EQ(info.findMapping(100)->line, 1u);  // last matching
}

TEST(DebugInfoTest, FindMappingMultiple) {
    DebugInfo info;
    info.source_map.push_back({0, 1, 1, "test.kern"});
    info.source_map.push_back({10, 2, 1, "test.kern"});
    info.source_map.push_back({20, 3, 1, "test.kern"});

    auto* m = info.findMapping(0);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->line, 1u);

    m = info.findMapping(15);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->line, 2u);

    m = info.findMapping(25);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->line, 3u);
}

TEST(DebugInfoTest, FindFunctionEmpty) {
    DebugInfo info;
    EXPECT_EQ(info.findFunction(0), nullptr);
}

TEST(DebugInfoTest, FindFunctionSingle) {
    DebugInfo info;
    FunctionDebugInfo fn;
    fn.name = "main";
    fn.code_start = 0;
    fn.code_end = 100;
    info.functions.push_back(fn);

    auto* f = info.findFunction(50);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->name, "main");

    EXPECT_EQ(info.findFunction(100), nullptr);  // end is exclusive
    EXPECT_EQ(info.findFunction(200), nullptr);
}

TEST(DebugInfoTest, FindFunctionMultiple) {
    DebugInfo info;
    FunctionDebugInfo fn1;
    fn1.name = "add";
    fn1.code_start = 0;
    fn1.code_end = 50;
    info.functions.push_back(fn1);

    FunctionDebugInfo fn2;
    fn2.name = "main";
    fn2.code_start = 50;
    fn2.code_end = 150;
    info.functions.push_back(fn2);

    auto* f = info.findFunction(25);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->name, "add");

    f = info.findFunction(75);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->name, "main");
}

TEST(DebugInfoTest, FindLocalEmpty) {
    DebugInfo info;
    FunctionDebugInfo fn;
    fn.name = "test";
    EXPECT_EQ(info.findLocal(fn, "x"), nullptr);
}

TEST(DebugInfoTest, FindLocalExists) {
    DebugInfo info;
    FunctionDebugInfo fn;
    fn.name = "test";
    fn.locals.push_back({"x", TypeTable::I64, -8, 1, 5});
    fn.locals.push_back({"y", TypeTable::I32, -12, 2, 5});

    auto* v = info.findLocal(fn, "x");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->stack_offset, -8);

    v = info.findLocal(fn, "y");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->stack_offset, -12);

    EXPECT_EQ(info.findLocal(fn, "z"), nullptr);
}

// ============================================================================
// SourceMapBuilder tests
// ============================================================================

#include "kern/debug/SourceMap.h"

TEST(SourceMapBuilderTest, EmptyBuild) {
    SourceMapBuilder builder;
    auto result = builder.build();
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(builder.size(), 0u);
}

TEST(SourceMapBuilderTest, SingleMapping) {
    SourceMapBuilder builder;
    builder.addMapping(0, 1, 1, "test.kern");
    EXPECT_EQ(builder.size(), 1u);

    auto result = builder.build();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].addr, 0u);
    EXPECT_EQ(result[0].line, 1u);
    EXPECT_EQ(result[0].column, 1u);
    EXPECT_EQ(result[0].file, "test.kern");
}

TEST(SourceMapBuilderTest, SortedByAddress) {
    SourceMapBuilder builder;
    builder.addMapping(20, 3, 1, "test.kern");
    builder.addMapping(0, 1, 1, "test.kern");
    builder.addMapping(10, 2, 1, "test.kern");

    auto result = builder.build();
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].addr, 0u);
    EXPECT_EQ(result[1].addr, 10u);
    EXPECT_EQ(result[2].addr, 20u);
}

TEST(SourceMapBuilderTest, DeduplicateConsecutive) {
    SourceMapBuilder builder;
    builder.addMapping(0, 1, 1, "test.kern");
    builder.addMapping(5, 1, 1, "test.kern");  // same loc, different addr
    builder.addMapping(10, 2, 1, "test.kern");

    auto result = builder.build();
    ASSERT_EQ(result.size(), 2u);  // first two deduplicated
    EXPECT_EQ(result[0].line, 1u);
    EXPECT_EQ(result[1].line, 2u);
}

TEST(SourceMapBuilderTest, Clear) {
    SourceMapBuilder builder;
    builder.addMapping(0, 1, 1, "test.kern");
    EXPECT_EQ(builder.size(), 1u);
    builder.clear();
    EXPECT_EQ(builder.size(), 0u);
    EXPECT_TRUE(builder.build().empty());
}

// ============================================================================
// DebugInfoBuilder tests
// ============================================================================

#include "kern/debug/DebugInfoBuilder.h"
#include "kern/backend/MachIR.h"
#include "kern/support/CompilationContext.h"
#include <sstream>

TEST(DebugInfoBuilderTest, BuildEmptyModule) {
    CompilationContext ctx;
    DebugInfoBuilder builder(ctx);

    MachModule mod;
    mod.functions = nullptr;
    mod.fn_count = 0;

    DebugInfo info = builder.build(mod);
    EXPECT_TRUE(info.functions.empty());
}

TEST(DebugInfoBuilderTest, BuildSingleFunction) {
    CompilationContext ctx;
    DebugInfoBuilder builder(ctx);

    // Create a minimal MachFunction
    MachBlock block;
    block.label = ctx.strings.intern("entry");
    MachInstr nop(X86Op::Nop);
    nop.operand_count = 0;
    block.instrs = &nop;
    block.instr_count = 1;

    MachFunction fn;
    fn.name = ctx.strings.intern("main");
    fn.blocks = &block;
    fn.block_count = 1;
    fn.is_intrinsic = false;
    fn.stack_size = 0;

    MachModule mod;
    mod.functions = &fn;
    mod.fn_count = 1;

    DebugInfo info = builder.build(mod);
    ASSERT_EQ(info.functions.size(), 1u);
    EXPECT_EQ(info.functions[0].name, "main");
    EXPECT_EQ(info.functions[0].code_start, 0u);
    EXPECT_GT(info.functions[0].code_end, 0u);
}

TEST(DebugInfoBuilderTest, SkipIntrinsicFunctions) {
    CompilationContext ctx;
    DebugInfoBuilder builder(ctx);

    MachBlock block;
    block.label = ctx.strings.intern("entry");
    block.instrs = nullptr;
    block.instr_count = 0;

    MachFunction fn;
    fn.name = ctx.strings.intern("print");
    fn.blocks = &block;
    fn.block_count = 1;
    fn.is_intrinsic = true;

    MachModule mod;
    mod.functions = &fn;
    mod.fn_count = 1;

    DebugInfo info = builder.build(mod);
    EXPECT_TRUE(info.functions.empty());
}

TEST(DebugInfoBuilderTest, StackSlotLocalDetection) {
    CompilationContext ctx;
    DebugInfoBuilder builder(ctx);

    // Create function with stack operands
    MachInstr mov = makeMov(MachOperand::stack(-8), MachOperand::immediate(42));
    MachInstr mov2 = makeMov(MachOperand::stack(-16), MachOperand::immediate(10));

    MachInstr instrs[] = {mov, mov2};
    MachBlock block;
    block.label = ctx.strings.intern("entry");
    block.instrs = instrs;
    block.instr_count = 2;

    MachFunction fn;
    fn.name = ctx.strings.intern("test");
    fn.blocks = &block;
    fn.block_count = 1;
    fn.is_intrinsic = false;

    MachModule mod;
    mod.functions = &fn;
    mod.fn_count = 1;

    DebugInfo info = builder.build(mod);
    ASSERT_EQ(info.functions.size(), 1u);
    EXPECT_EQ(info.functions[0].locals.size(), 2u);
}

TEST(DebugInfoBuilderTest, SerializeDeserializeEmpty) {
    DebugInfo info;

    std::stringstream ss;
    DebugInfoBuilder::serialize(info, ss);

    ss.seekg(0);
    DebugInfo restored = DebugInfoBuilder::deserialize(ss);
    EXPECT_TRUE(restored.functions.empty());
    EXPECT_TRUE(restored.source_map.empty());
}

TEST(DebugInfoBuilderTest, SerializeDeserializeWithData) {
    DebugInfo info;
    FunctionDebugInfo fn;
    fn.name = "main";
    fn.code_start = 0;
    fn.code_end = 100;
    fn.source_loc = {1, 1, "test.kern"};
    fn.locals.push_back({"x", TypeTable::I64, -8, 1, 5});
    info.functions.push_back(fn);

    info.source_map.push_back({0, 1, 1, "test.kern"});
    info.source_map.push_back({50, 2, 5, "test.kern"});

    std::stringstream ss;
    DebugInfoBuilder::serialize(info, ss);

    ss.seekg(0);
    DebugInfo restored = DebugInfoBuilder::deserialize(ss);
    ASSERT_EQ(restored.functions.size(), 1u);
    EXPECT_EQ(restored.functions[0].code_start, 0u);
    EXPECT_EQ(restored.functions[0].code_end, 100u);
    ASSERT_EQ(restored.functions[0].locals.size(), 1u);
    EXPECT_EQ(restored.functions[0].locals[0].stack_offset, -8);

    ASSERT_EQ(restored.source_map.size(), 2u);
    EXPECT_EQ(restored.source_map[0].line, 1u);
    EXPECT_EQ(restored.source_map[1].line, 2u);
}

TEST(DebugInfoBuilderTest, DeserializeBadMagic) {
    std::stringstream ss;
    ss.write("XXXX", 4);

    ss.seekg(0);
    DebugInfo info = DebugInfoBuilder::deserialize(ss);
    EXPECT_TRUE(info.functions.empty());
}

// ============================================================================
// ValueInspector tests
// ============================================================================

#include "kern/debug/ValueInspector.h"

TEST(ValueInspectorTest, FormatI64) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    int64_t val = 42;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::I64, bytes, 8), "42");
}

TEST(ValueInspectorTest, FormatI32) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    int32_t val = -10;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::I32, bytes, 4), "-10");
}

TEST(ValueInspectorTest, FormatBool) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint8_t t = 1;
    EXPECT_EQ(inspector.format(TypeTable::Bool, &t, 1), "true");
    uint8_t f = 0;
    EXPECT_EQ(inspector.format(TypeTable::Bool, &f, 1), "false");
}

TEST(ValueInspectorTest, FormatU8) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint8_t val = 255;
    EXPECT_EQ(inspector.format(TypeTable::U8, &val, 1), "255");
}

TEST(ValueInspectorTest, FormatUnit) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    EXPECT_EQ(inspector.format(TypeTable::Unit, nullptr, 0), "()");
}

TEST(ValueInspectorTest, FormatUnknownType) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    EXPECT_EQ(inspector.format(9999, nullptr, 0), "<unknown type>");
}

TEST(ValueInspectorTest, FormatPointer) {
    Arena arena;
    TypeTable types(arena);
    TypeId ptr_type = types.makePtr(TypeTable::I64, false);
    ValueInspector inspector(types);

    uint64_t addr = 0x1234;
    auto bytes = reinterpret_cast<const uint8_t*>(&addr);
    auto result = inspector.format(ptr_type, bytes, 8);
    EXPECT_NE(result.find("Ptr<"), std::string::npos);
    EXPECT_NE(result.find("1234"), std::string::npos);
}

TEST(ValueInspectorTest, FormatMutablePointer) {
    Arena arena;
    TypeTable types(arena);
    TypeId ptr_type = types.makePtr(TypeTable::I64, true);
    ValueInspector inspector(types);

    uint64_t addr = 0x5678;
    auto bytes = reinterpret_cast<const uint8_t*>(&addr);
    auto result = inspector.format(ptr_type, bytes, 8);
    EXPECT_NE(result.find("Ptr<var"), std::string::npos);
}

TEST(ValueInspectorTest, FormatStruct) {
    Arena arena;
    TypeTable types(arena);
    FieldInfo fields[] = {
        {"x", TypeTable::I64, false, 0},
        {"y", TypeTable::I64, false, 8},
    };
    TypeId struct_type = types.makeStruct("Point", fields);
    ValueInspector inspector(types);

    uint8_t bytes[16] = {};
    auto result = inspector.format(struct_type, bytes, 16);
    EXPECT_NE(result.find("struct"), std::string::npos);
    EXPECT_NE(result.find("2 fields"), std::string::npos);
}

TEST(ValueInspectorTest, FormatEnum) {
    Arena arena;
    TypeTable types(arena);
    std::string_view names[] = {"Red", "Green", "Blue"};
    int64_t values[] = {0, 1, 2};
    TypeId enum_type = types.makeEnum("Color", names, values);
    ValueInspector inspector(types);

    int64_t tag = 1;
    auto bytes = reinterpret_cast<const uint8_t*>(&tag);
    auto result = inspector.format(enum_type, bytes, 8);
    EXPECT_NE(result.find("enum(1)"), std::string::npos);
}

TEST(ValueInspectorTest, SizeOfPrimitive) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    EXPECT_EQ(inspector.sizeOf(TypeTable::I64), 8u);
    EXPECT_EQ(inspector.sizeOf(TypeTable::I32), 4u);
    EXPECT_EQ(inspector.sizeOf(TypeTable::I16), 2u);
    EXPECT_EQ(inspector.sizeOf(TypeTable::I8), 1u);
    EXPECT_EQ(inspector.sizeOf(TypeTable::Bool), 1u);
}

TEST(ValueInspectorTest, SizeOfUnknown) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    EXPECT_EQ(inspector.sizeOf(9999), 0u);
}

TEST(ValueInspectorTest, FormatLocal) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    LocalVarInfo var;
    var.name = "x";
    var.type = TypeTable::I64;
    var.stack_offset = -8;

    // Simulate memory: rbp=1000, var at [1000-8]=992
    int64_t mem_val = 42;
    auto reader = [&](uint64_t addr, void* buf, size_t size) -> bool {
        if (addr == 992 && size == 8) {
            std::memcpy(buf, &mem_val, 8);
            return true;
        }
        return false;
    };

    EXPECT_EQ(inspector.formatLocal(var, 1000, reader), "42");
}

TEST(ValueInspectorTest, FormatLocalReadFailed) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    LocalVarInfo var;
    var.name = "x";
    var.type = TypeTable::I64;
    var.stack_offset = -8;

    auto reader = [](uint64_t, void*, size_t) -> bool { return false; };
    EXPECT_EQ(inspector.formatLocal(var, 1000, reader), "<read failed>");
}

TEST(ValueInspectorTest, FormatI8Negative) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    int8_t val = -1;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::I8, bytes, 1), "-1");
}

TEST(ValueInspectorTest, FormatI16) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    int16_t val = 1000;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::I16, bytes, 2), "1000");
}

TEST(ValueInspectorTest, FormatU16) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint16_t val = 65535;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::U16, bytes, 2), "65535");
}

TEST(ValueInspectorTest, FormatU32) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint32_t val = 1000000;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::U32, bytes, 4), "1000000");
}

TEST(ValueInspectorTest, FormatU64) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint64_t val = 18446744073709551615ULL;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    EXPECT_EQ(inspector.format(TypeTable::U64, bytes, 8), "18446744073709551615");
}

TEST(ValueInspectorTest, FormatF32) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    float val = 3.14f;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    auto result = inspector.format(TypeTable::F32, bytes, 4);
    EXPECT_NE(result.find("3.14"), std::string::npos);
    EXPECT_NE(result.find("f"), std::string::npos);
}

TEST(ValueInspectorTest, FormatF64) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    double val = 2.718;
    auto bytes = reinterpret_cast<const uint8_t*>(&val);
    auto result = inspector.format(TypeTable::F64, bytes, 8);
    EXPECT_NE(result.find("2.718"), std::string::npos);
}

TEST(ValueInspectorTest, InsufficientBytesI64) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint8_t bytes[4] = {};
    EXPECT_EQ(inspector.format(TypeTable::I64, bytes, 4), "<i64?>");
}

TEST(ValueInspectorTest, InsufficientBytesF32) {
    Arena arena;
    TypeTable types(arena);
    ValueInspector inspector(types);

    uint8_t bytes[2] = {};
    EXPECT_EQ(inspector.format(TypeTable::F32, bytes, 2), "<f32?>");
}
