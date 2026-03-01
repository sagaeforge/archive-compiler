#include "kern/lir/LIR.h"
#include "kern/lir/LIRDump.h"
#include "kern/lir/LIRPass.h"
#include "kern/support/CompilationContext.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class LIRTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    VReg nextVReg(uint32_t& counter) { return counter++; }

    LIRInstr makeConstInt(VReg result, int64_t value, TypeId type) {
        LIRInstr i{};
        i.op = LIROp::ConstInt;
        i.result = result;
        i.type = type;
        i.const_int.value = value;
        return i;
    }

    LIRInstr makeConstFloat(VReg result, double value, TypeId type) {
        LIRInstr i{};
        i.op = LIROp::ConstFloat;
        i.result = result;
        i.type = type;
        i.const_float.value = value;
        return i;
    }

    LIRInstr makeConstBool(VReg result, bool value) {
        LIRInstr i{};
        i.op = LIROp::ConstBool;
        i.result = result;
        i.type = TypeTable::Bool;
        i.const_bool.value = value;
        return i;
    }

    LIRInstr makeBinOp(LIROp op, VReg result, VReg lhs, VReg rhs, TypeId type) {
        LIRInstr i{};
        i.op = op;
        i.result = result;
        i.type = type;
        i.bin.lhs = lhs;
        i.bin.rhs = rhs;
        return i;
    }

    LIRInstr makeRet(VReg value, TypeId type) {
        LIRInstr i{};
        i.op = LIROp::Ret;
        i.result = INVALID_VREG;
        i.type = type;
        i.ret.value = value;
        return i;
    }
};

// ============================================================================
// Opcode name tests
// ============================================================================

TEST_F(LIRTest, OpNames) {
    EXPECT_STREQ(lirOpName(LIROp::ConstInt), "const_int");
    EXPECT_STREQ(lirOpName(LIROp::Add), "add");
    EXPECT_STREQ(lirOpName(LIROp::ICmpEq), "icmp_eq");
    EXPECT_STREQ(lirOpName(LIROp::Call), "call");
    EXPECT_STREQ(lirOpName(LIROp::Ret), "ret");
    EXPECT_STREQ(lirOpName(LIROp::Branch), "br");
    EXPECT_STREQ(lirOpName(LIROp::CondBranch), "condbr");
    EXPECT_STREQ(lirOpName(LIROp::Load), "load");
    EXPECT_STREQ(lirOpName(LIROp::Store), "store");
    EXPECT_STREQ(lirOpName(LIROp::BlockArg), "block_arg");
}

// ============================================================================
// Instruction construction tests
// ============================================================================

TEST_F(LIRTest, ConstIntInstr) {
    auto i = makeConstInt(0, 42, TypeTable::I64);
    EXPECT_EQ(i.op, LIROp::ConstInt);
    EXPECT_EQ(i.result, 0u);
    EXPECT_EQ(i.type, TypeTable::I64);
    EXPECT_EQ(i.const_int.value, 42);
}

TEST_F(LIRTest, ConstFloatInstr) {
    auto i = makeConstFloat(1, 3.14, TypeTable::F64);
    EXPECT_EQ(i.op, LIROp::ConstFloat);
    EXPECT_EQ(i.const_float.value, 3.14);
}

TEST_F(LIRTest, ConstBoolInstr) {
    auto i = makeConstBool(2, true);
    EXPECT_EQ(i.op, LIROp::ConstBool);
    EXPECT_TRUE(i.const_bool.value);
}

TEST_F(LIRTest, BinOpInstr) {
    auto i = makeBinOp(LIROp::Add, 3, 1, 2, TypeTable::I64);
    EXPECT_EQ(i.op, LIROp::Add);
    EXPECT_EQ(i.bin.lhs, 1u);
    EXPECT_EQ(i.bin.rhs, 2u);
}

TEST_F(LIRTest, RetInstr) {
    auto i = makeRet(5, TypeTable::I64);
    EXPECT_EQ(i.op, LIROp::Ret);
    EXPECT_EQ(i.result, INVALID_VREG);
    EXPECT_EQ(i.ret.value, 5u);
}

TEST_F(LIRTest, UnaryInstr) {
    LIRInstr i{};
    i.op = LIROp::Neg;
    i.result = 4;
    i.type = TypeTable::I64;
    i.unary.operand = 3;
    EXPECT_EQ(i.op, LIROp::Neg);
    EXPECT_EQ(i.unary.operand, 3u);
}

TEST_F(LIRTest, BranchInstr) {
    LIRInstr i{};
    i.op = LIROp::Branch;
    i.result = INVALID_VREG;
    i.type = TypeTable::Unit;
    i.branch.target = 2;
    EXPECT_EQ(i.branch.target, 2u);
}

TEST_F(LIRTest, CondBranchInstr) {
    LIRInstr i{};
    i.op = LIROp::CondBranch;
    i.result = INVALID_VREG;
    i.type = TypeTable::Unit;
    i.cond_branch.cond = 0;
    i.cond_branch.true_target = 1;
    i.cond_branch.false_target = 2;
    EXPECT_EQ(i.cond_branch.cond, 0u);
    EXPECT_EQ(i.cond_branch.true_target, 1u);
    EXPECT_EQ(i.cond_branch.false_target, 2u);
}

TEST_F(LIRTest, CallInstr) {
    LIRInstr i{};
    i.op = LIROp::Call;
    i.result = 10;
    i.type = TypeTable::I64;
    i.call.callee = "foo";
    i.call.args = nullptr;
    i.call.arg_count = 0;
    i.call.is_tail = true;
    EXPECT_EQ(i.call.callee, "foo");
    EXPECT_TRUE(i.call.is_tail);
}

TEST_F(LIRTest, MemoryInstrs) {
    // AddrOf
    LIRInstr addr{};
    addr.op = LIROp::AddrOf;
    addr.result = 5;
    addr.addr_of.source = 3;
    EXPECT_EQ(addr.addr_of.source, 3u);

    // Load
    LIRInstr ld{};
    ld.op = LIROp::Load;
    ld.result = 6;
    ld.load.ptr = 5;
    EXPECT_EQ(ld.load.ptr, 5u);

    // Store
    LIRInstr st{};
    st.op = LIROp::Store;
    st.result = INVALID_VREG;
    st.store.ptr = 5;
    st.store.value = 7;
    EXPECT_EQ(st.store.value, 7u);

    // FieldPtr
    LIRInstr fp{};
    fp.op = LIROp::FieldPtr;
    fp.result = 8;
    fp.field_ptr.base = 5;
    fp.field_ptr.offset = 16;
    EXPECT_EQ(fp.field_ptr.offset, 16u);

    // StructAlloc
    LIRInstr sa{};
    sa.op = LIROp::StructAlloc;
    sa.result = 9;
    sa.struct_alloc.size = 24;
    sa.struct_alloc.align = 8;
    EXPECT_EQ(sa.struct_alloc.size, 24u);
}

TEST_F(LIRTest, BlockArgInstr) {
    LIRInstr i{};
    i.op = LIROp::BlockArg;
    i.result = 0;
    i.type = TypeTable::I64;
    i.block_arg.index = 0;
    EXPECT_EQ(i.block_arg.index, 0u);
}

// ============================================================================
// Block tests
// ============================================================================

TEST_F(LIRTest, BlockBasic) {
    LIRBlock block{};
    block.label = "entry";
    block.param_types = nullptr;
    block.param_count = 0;

    auto* instrs = ctx.arena.makeArray<LIRInstr>(2);
    instrs[0] = makeConstInt(0, 42, TypeTable::I64);
    instrs[1] = makeRet(0, TypeTable::I64);
    block.instrs = instrs;
    block.instr_count = 2;

    EXPECT_EQ(block.label, "entry");
    EXPECT_EQ(block.instr_count, 2u);
    EXPECT_EQ(block.instrs[0].op, LIROp::ConstInt);
    EXPECT_EQ(block.instrs[1].op, LIROp::Ret);
}

TEST_F(LIRTest, BlockWithParams) {
    LIRBlock block{};
    block.label = "merge";
    block.param_types = ctx.arena.makeArray<TypeId>(2);
    block.param_types[0] = TypeTable::I64;
    block.param_types[1] = TypeTable::Bool;
    block.param_count = 2;
    block.instrs = nullptr;
    block.instr_count = 0;

    EXPECT_EQ(block.param_count, 2u);
    EXPECT_EQ(block.param_types[0], TypeTable::I64);
    EXPECT_EQ(block.param_types[1], TypeTable::Bool);
}

// ============================================================================
// Function tests
// ============================================================================

TEST_F(LIRTest, FunctionBasic) {
    LIRFunction fn{};
    fn.name = "main";
    fn.param_types = nullptr;
    fn.param_count = 0;
    fn.return_type = TypeTable::I64;
    fn.next_vreg = 1;
    fn.purity = 0; // Pure
    fn.is_recursive = false;
    fn.is_tail_recursive = false;
    fn.is_intrinsic = false;

    auto* blocks = ctx.arena.makeArray<LIRBlock>(1);
    blocks[0].label = "entry";
    blocks[0].param_types = nullptr;
    blocks[0].param_count = 0;
    auto* instrs = ctx.arena.makeArray<LIRInstr>(2);
    instrs[0] = makeConstInt(0, 42, TypeTable::I64);
    instrs[1] = makeRet(0, TypeTable::I64);
    blocks[0].instrs = instrs;
    blocks[0].instr_count = 2;
    fn.blocks = blocks;
    fn.block_count = 1;

    EXPECT_EQ(fn.name, "main");
    EXPECT_EQ(fn.block_count, 1u);
    EXPECT_EQ(fn.blocks[0].instr_count, 2u);
}

// ============================================================================
// GlobalData tests
// ============================================================================

TEST_F(LIRTest, GlobalStringLit) {
    GlobalData g{};
    g.kind = GlobalData::StringLit;
    g.index = 0;
    g.label = "_str_0";
    g.string_lit.data = "hello";
    g.string_lit.length = 5;

    EXPECT_EQ(g.kind, GlobalData::StringLit);
    EXPECT_EQ(g.string_lit.length, 5u);
}

TEST_F(LIRTest, GlobalFloatConst) {
    GlobalData g{};
    g.kind = GlobalData::FloatConst;
    g.index = 1;
    g.label = "_float_1";
    g.float_const.value = 3.14;
    g.float_const.is_f32 = false;

    EXPECT_EQ(g.kind, GlobalData::FloatConst);
    EXPECT_EQ(g.float_const.value, 3.14);
    EXPECT_FALSE(g.float_const.is_f32);
}

// ============================================================================
// LIRModule tests
// ============================================================================

TEST_F(LIRTest, ModuleEmpty) {
    LIRModule mod{};
    mod.functions = nullptr;
    mod.fn_count = 0;
    mod.globals = nullptr;
    mod.global_count = 0;

    EXPECT_EQ(mod.fn_count, 0u);
    EXPECT_EQ(mod.global_count, 0u);
}

// ============================================================================
// Dump tests
// ============================================================================

TEST_F(LIRTest, DumpConstInt) {
    auto i = makeConstInt(0, 42, TypeTable::I64);
    std::ostringstream out;
    dumpLIRInstr(i, ctx.types, out);
    EXPECT_EQ(out.str(), "%v0 = const_int 42 : i64");
}

TEST_F(LIRTest, DumpBinOp) {
    auto i = makeBinOp(LIROp::Add, 2, 0, 1, TypeTable::I64);
    std::ostringstream out;
    dumpLIRInstr(i, ctx.types, out);
    EXPECT_EQ(out.str(), "%v2 = add %v0, %v1 : i64");
}

TEST_F(LIRTest, DumpRet) {
    auto i = makeRet(2, TypeTable::I64);
    std::ostringstream out;
    dumpLIRInstr(i, ctx.types, out);
    EXPECT_EQ(out.str(), "ret %v2 : i64");
}

TEST_F(LIRTest, DumpCall) {
    auto* args = ctx.arena.makeArray<VReg>(2);
    args[0] = 1; args[1] = 2;
    LIRInstr i{};
    i.op = LIROp::Call;
    i.result = 3;
    i.type = TypeTable::I64;
    i.call.callee = "add";
    i.call.args = args;
    i.call.arg_count = 2;
    i.call.is_tail = false;
    std::ostringstream out;
    dumpLIRInstr(i, ctx.types, out);
    EXPECT_EQ(out.str(), "%v3 = call @add(%v1, %v2) : i64");
}

TEST_F(LIRTest, DumpTailCall) {
    LIRInstr i{};
    i.op = LIROp::Call;
    i.result = 5;
    i.type = TypeTable::I64;
    i.call.callee = "foo";
    i.call.args = nullptr;
    i.call.arg_count = 0;
    i.call.is_tail = true;
    std::ostringstream out;
    dumpLIRInstr(i, ctx.types, out);
    EXPECT_EQ(out.str(), "%v5 = call [tail] @foo() : i64");
}

TEST_F(LIRTest, DumpCondBranch) {
    LIRInstr i{};
    i.op = LIROp::CondBranch;
    i.result = INVALID_VREG;
    i.type = TypeTable::Unit;
    i.cond_branch.cond = 3;
    i.cond_branch.true_target = 1;
    i.cond_branch.false_target = 2;
    std::ostringstream out;
    dumpLIRInstr(i, ctx.types, out);
    EXPECT_EQ(out.str(), "condbr %v3, bb1, bb2 : Unit");
}

TEST_F(LIRTest, DumpFunction) {
    LIRFunction fn{};
    fn.name = "main";
    fn.param_types = nullptr;
    fn.param_count = 0;
    fn.return_type = TypeTable::I64;
    fn.next_vreg = 1;
    fn.purity = 0;
    fn.is_recursive = false;
    fn.is_tail_recursive = false;
    fn.is_intrinsic = false;

    auto* blocks = ctx.arena.makeArray<LIRBlock>(1);
    blocks[0].label = "entry";
    blocks[0].param_types = nullptr;
    blocks[0].param_count = 0;
    auto* instrs = ctx.arena.makeArray<LIRInstr>(2);
    instrs[0] = makeConstInt(0, 42, TypeTable::I64);
    instrs[1] = makeRet(0, TypeTable::I64);
    blocks[0].instrs = instrs;
    blocks[0].instr_count = 2;
    fn.blocks = blocks;
    fn.block_count = 1;

    std::ostringstream out;
    dumpLIRFunction(&fn, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("fn @main") != std::string::npos);
    EXPECT_TRUE(s.find("entry:") != std::string::npos);
    EXPECT_TRUE(s.find("%v0 = const_int 42") != std::string::npos);
    EXPECT_TRUE(s.find("ret %v0") != std::string::npos);
}

TEST_F(LIRTest, DumpGlobals) {
    auto* globals = ctx.arena.makeArray<GlobalData>(1);
    globals[0].kind = GlobalData::StringLit;
    globals[0].index = 0;
    globals[0].label = "_str_0";
    globals[0].string_lit.data = "hello";
    globals[0].string_lit.length = 5;

    LIRModule mod{};
    mod.functions = nullptr;
    mod.fn_count = 0;
    mod.globals = globals;
    mod.global_count = 1;

    std::ostringstream out;
    dumpLIR(&mod, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("@g0 = string \"hello\"") != std::string::npos);
    EXPECT_TRUE(s.find("5 bytes") != std::string::npos);
}

// ============================================================================
// LIRPass infrastructure tests
// ============================================================================

class TestPass : public LIRPass {
public:
    int run_count = 0;
    std::string_view name() const override { return "TestPass"; }
    void run(LIRModule& /*module*/, CompilationContext& /*ctx*/) override {
        run_count++;
    }
};

TEST_F(LIRTest, PassManagerRuns) {
    LIRPassManager pm;
    auto* pass = new TestPass();
    // We can't use add<TestPass>() and then access run_count,
    // so we test via side effect on module
    LIRModule mod{};
    mod.functions = nullptr;
    mod.fn_count = 0;
    mod.globals = nullptr;
    mod.global_count = 0;

    // Just verify no crash
    pm.add<TestPass>();
    pm.run(mod, ctx);
    delete pass; // clean up unused instance
}

TEST_F(LIRTest, INVALID_VREG_Value) {
    EXPECT_EQ(INVALID_VREG, UINT32_MAX);
}

} // namespace kern
