#include "kern/backend/MachIR.h"
#include "kern/backend/MachIRDump.h"
#include "kern/support/CompilationContext.h"

#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

// ============================================================================
// PhysReg Tests
// ============================================================================

TEST(MachIR, PhysRegName64) {
    EXPECT_STREQ(physRegName(PhysReg::RAX), "rax");
    EXPECT_STREQ(physRegName(PhysReg::RBX), "rbx");
    EXPECT_STREQ(physRegName(PhysReg::R15), "r15");
    EXPECT_STREQ(physRegName(PhysReg::RSP), "rsp");
    EXPECT_STREQ(physRegName(PhysReg::RBP), "rbp");
    EXPECT_STREQ(physRegName(PhysReg::NONE), "none");
}

TEST(MachIR, PhysRegName32) {
    EXPECT_STREQ(physRegName(PhysReg::RAX, 32), "eax");
    EXPECT_STREQ(physRegName(PhysReg::RDX, 32), "edx");
    EXPECT_STREQ(physRegName(PhysReg::R8, 32), "r8d");
    EXPECT_STREQ(physRegName(PhysReg::R15, 32), "r15d");
}

TEST(MachIR, PhysRegName16) {
    EXPECT_STREQ(physRegName(PhysReg::RAX, 16), "ax");
    EXPECT_STREQ(physRegName(PhysReg::RSI, 16), "si");
    EXPECT_STREQ(physRegName(PhysReg::R8, 16), "r8w");
}

TEST(MachIR, PhysRegName8) {
    EXPECT_STREQ(physRegName(PhysReg::RAX, 8), "al");
    EXPECT_STREQ(physRegName(PhysReg::RDX, 8), "dl");
    EXPECT_STREQ(physRegName(PhysReg::RDI, 8), "dil");
    EXPECT_STREQ(physRegName(PhysReg::R8, 8), "r8b");
}

TEST(MachIR, PhysRegXMMName) {
    EXPECT_STREQ(physRegName(PhysReg::XMM0), "xmm0");
    EXPECT_STREQ(physRegName(PhysReg::XMM15), "xmm15");
    // XMM registers don't change with width
    EXPECT_STREQ(physRegName(PhysReg::XMM0, 32), "xmm0");
}

TEST(MachIR, IsGPR) {
    EXPECT_TRUE(isGPR(PhysReg::RAX));
    EXPECT_TRUE(isGPR(PhysReg::R15));
    EXPECT_FALSE(isGPR(PhysReg::XMM0));
    EXPECT_FALSE(isGPR(PhysReg::RSP));
    EXPECT_FALSE(isGPR(PhysReg::NONE));
}

TEST(MachIR, IsXMM) {
    EXPECT_TRUE(isXMM(PhysReg::XMM0));
    EXPECT_TRUE(isXMM(PhysReg::XMM15));
    EXPECT_FALSE(isXMM(PhysReg::RAX));
    EXPECT_FALSE(isXMM(PhysReg::RSP));
}

// ============================================================================
// MachOperand Tests
// ============================================================================

TEST(MachIR, OperandVirt) {
    auto op = MachOperand::virt(42);
    EXPECT_TRUE(op.isVirtual());
    EXPECT_TRUE(op.isReg());
    EXPECT_FALSE(op.isPhysical());
    EXPECT_EQ(op.vreg, 42u);
}

TEST(MachIR, OperandPrecolored) {
    auto op = MachOperand::precolored(PhysReg::RAX);
    EXPECT_TRUE(op.isPhysical());
    EXPECT_TRUE(op.isReg());
    EXPECT_FALSE(op.isVirtual());
    EXPECT_EQ(op.phys, PhysReg::RAX);
}

TEST(MachIR, OperandImm) {
    auto op = MachOperand::immediate(42);
    EXPECT_TRUE(op.isImm());
    EXPECT_FALSE(op.isReg());
    EXPECT_EQ(op.imm, 42);
}

TEST(MachIR, OperandStack) {
    auto op = MachOperand::stack(-16);
    EXPECT_TRUE(op.isStack());
    EXPECT_EQ(op.stack_offset, -16);
}

TEST(MachIR, OperandLabel) {
    auto op = MachOperand::lbl("_main");
    EXPECT_TRUE(op.isLabel());
    EXPECT_EQ(op.label, "_main");
}

TEST(MachIR, OperandNone) {
    auto op = MachOperand::none();
    EXPECT_TRUE(op.isNone());
    EXPECT_FALSE(op.isReg());
}

// ============================================================================
// X86Op Tests
// ============================================================================

TEST(MachIR, X86OpName) {
    EXPECT_STREQ(x86OpName(X86Op::Mov), "mov");
    EXPECT_STREQ(x86OpName(X86Op::Add), "add");
    EXPECT_STREQ(x86OpName(X86Op::IMul), "imul");
    EXPECT_STREQ(x86OpName(X86Op::Call), "call");
    EXPECT_STREQ(x86OpName(X86Op::Ret), "ret");
    EXPECT_STREQ(x86OpName(X86Op::Addsd), "addsd");
    EXPECT_STREQ(x86OpName(X86Op::Nop), "nop");
}

TEST(MachIR, CondCodeSuffix) {
    EXPECT_STREQ(condCodeSuffix(CondCode::E), "e");
    EXPECT_STREQ(condCodeSuffix(CondCode::NE), "ne");
    EXPECT_STREQ(condCodeSuffix(CondCode::L), "l");
    EXPECT_STREQ(condCodeSuffix(CondCode::GE), "ge");
    EXPECT_STREQ(condCodeSuffix(CondCode::B), "b");
    EXPECT_STREQ(condCodeSuffix(CondCode::AE), "ae");
}

// ============================================================================
// Instruction Builder Tests
// ============================================================================

TEST(MachIR, MakeMov) {
    auto mi = makeMov(MachOperand::precolored(PhysReg::RAX),
                      MachOperand::immediate(42));
    EXPECT_EQ(mi.op, X86Op::Mov);
    EXPECT_EQ(mi.width, 64);
    EXPECT_EQ(mi.operand_count, 2);
    EXPECT_TRUE(mi.dst().isPhysical());
    EXPECT_TRUE(mi.src1().isImm());
    EXPECT_EQ(mi.src1().imm, 42);
}

TEST(MachIR, MakeAlu) {
    auto mi = makeAlu(X86Op::Add, MachOperand::virt(0),
                      MachOperand::virt(1), 32);
    EXPECT_EQ(mi.op, X86Op::Add);
    EXPECT_EQ(mi.width, 32);
    EXPECT_EQ(mi.operand_count, 2);
    EXPECT_EQ(mi.dst().vreg, 0u);
    EXPECT_EQ(mi.src1().vreg, 1u);
}

TEST(MachIR, MakeCmp) {
    auto mi = makeCmp(MachOperand::virt(0), MachOperand::immediate(0));
    EXPECT_EQ(mi.op, X86Op::Cmp);
    EXPECT_EQ(mi.operand_count, 2);
}

TEST(MachIR, MakeSetcc) {
    auto mi = makeSetcc(CondCode::L, MachOperand::virt(2));
    EXPECT_EQ(mi.op, X86Op::Setcc);
    EXPECT_EQ(mi.cc, CondCode::L);
    EXPECT_EQ(mi.width, 8);
    EXPECT_EQ(mi.operand_count, 1);
}

TEST(MachIR, MakeJmpJcc) {
    auto jmp = makeJmp(MachOperand::lbl(".L1"));
    EXPECT_EQ(jmp.op, X86Op::Jmp);
    EXPECT_EQ(jmp.operand_count, 1);
    EXPECT_EQ(jmp.dst().label, ".L1");

    auto jcc = makeJcc(CondCode::NE, MachOperand::lbl(".L2"));
    EXPECT_EQ(jcc.op, X86Op::Jcc);
    EXPECT_EQ(jcc.cc, CondCode::NE);
}

TEST(MachIR, MakeCallRet) {
    auto call = makeCall(MachOperand::lbl("_foo"));
    EXPECT_EQ(call.op, X86Op::Call);
    EXPECT_EQ(call.operand_count, 1);

    auto ret = makeRet();
    EXPECT_EQ(ret.op, X86Op::Ret);
    EXPECT_EQ(ret.operand_count, 0);
}

TEST(MachIR, MakePushPop) {
    auto push = makePush(MachOperand::precolored(PhysReg::RBX));
    EXPECT_EQ(push.op, X86Op::Push);
    EXPECT_EQ(push.operand_count, 1);

    auto pop = makePop(MachOperand::precolored(PhysReg::RBX));
    EXPECT_EQ(pop.op, X86Op::Pop);
    EXPECT_EQ(pop.operand_count, 1);
}

TEST(MachIR, MakeLea) {
    auto lea = makeLea(MachOperand::virt(0), MachOperand::stack(-8));
    EXPECT_EQ(lea.op, X86Op::Lea);
    EXPECT_EQ(lea.operand_count, 2);
}

// ============================================================================
// MachBlock / MachFunction Tests
// ============================================================================

TEST(MachIR, BlockBasic) {
    MachBlock block;
    block.label = ".entry";
    block.instr_count = 0;
    block.instrs = nullptr;
    EXPECT_EQ(block.label, ".entry");
    EXPECT_EQ(block.instr_count, 0u);
}

TEST(MachIR, FunctionBasic) {
    MachFunction fn;
    fn.name = "main";
    fn.block_count = 0;
    fn.blocks = nullptr;
    fn.stack_size = 32;
    fn.is_intrinsic = false;
    EXPECT_EQ(fn.name, "main");
    EXPECT_EQ(fn.stack_size, 32u);
}

// ============================================================================
// Dump Tests
// ============================================================================

TEST(MachIRDump, DumpMovInstr) {
    auto mi = makeMov(MachOperand::precolored(PhysReg::RAX),
                      MachOperand::immediate(42));
    std::ostringstream oss;
    dumpMachInstr(mi, oss);
    EXPECT_EQ(oss.str(), "    mov rax, 42\n");
}

TEST(MachIRDump, DumpMovVreg) {
    auto mi = makeMov(MachOperand::virt(0), MachOperand::virt(1));
    std::ostringstream oss;
    dumpMachInstr(mi, oss);
    EXPECT_EQ(oss.str(), "    mov %v0, %v1\n");
}

TEST(MachIRDump, DumpSetcc) {
    auto mi = makeSetcc(CondCode::L, MachOperand::precolored(PhysReg::RAX));
    std::ostringstream oss;
    dumpMachInstr(mi, oss);
    EXPECT_EQ(oss.str(), "    setl al\n");
}

TEST(MachIRDump, DumpJcc) {
    auto mi = makeJcc(CondCode::NE, MachOperand::lbl(".L2"));
    std::ostringstream oss;
    dumpMachInstr(mi, oss);
    EXPECT_EQ(oss.str(), "    jne .L2\n");
}

TEST(MachIRDump, DumpStackOperand) {
    auto mi = makeMov(MachOperand::precolored(PhysReg::RAX),
                      MachOperand::stack(-16));
    std::ostringstream oss;
    dumpMachInstr(mi, oss);
    EXPECT_EQ(oss.str(), "    mov rax, [rbp-16]\n");
}

TEST(MachIRDump, DumpFramePseudo) {
    MachInstr setup(X86Op::Pseudo_FrameSetup);
    setup.operand_count = 0;
    std::ostringstream oss;
    dumpMachInstr(setup, oss);
    EXPECT_EQ(oss.str(), "    frame_setup\n");
}

TEST(MachIRDump, DumpParallelMove) {
    MachInstr pm(X86Op::Pseudo_ParallelMove);
    pm.operand_count = 4;
    pm.inline_ops[0] = MachOperand::precolored(PhysReg::RDI);
    pm.inline_ops[1] = MachOperand::virt(0);
    pm.inline_ops[2] = MachOperand::precolored(PhysReg::RSI);
    pm.inline_ops[3] = MachOperand::virt(1);
    std::ostringstream oss;
    dumpMachInstr(pm, oss);
    EXPECT_EQ(oss.str(), "    parallel_move rdi <- %v0, rsi <- %v1\n");
}

TEST(MachIRDump, DumpFunction) {
    CompilationContext ctx;
    MachInstr instrs[2];
    instrs[0] = makeMov(MachOperand::precolored(PhysReg::RAX),
                        MachOperand::immediate(42));
    instrs[1] = makeRet();

    MachBlock block;
    block.label = ".entry";
    block.instrs = instrs;
    block.instr_count = 2;

    MachFunction fn;
    fn.name = "main";
    fn.blocks = &block;
    fn.block_count = 1;
    fn.stack_size = 0;
    fn.is_intrinsic = false;

    std::ostringstream oss;
    dumpMachFunction(fn, ctx.types, oss);
    std::string out = oss.str();
    EXPECT_NE(out.find("fn @main"), std::string::npos);
    EXPECT_NE(out.find("mov rax, 42"), std::string::npos);
    EXPECT_NE(out.find("ret"), std::string::npos);
}

TEST(MachIRDump, DumpIntrinsic) {
    CompilationContext ctx;
    MachFunction fn;
    fn.name = "print";
    fn.blocks = nullptr;
    fn.block_count = 0;
    fn.is_intrinsic = true;

    std::ostringstream oss;
    dumpMachFunction(fn, ctx.types, oss);
    EXPECT_NE(oss.str().find("intrinsic"), std::string::npos);
}

// ============================================================================
// ABI Constants Tests
// ============================================================================

TEST(MachIR, ABIConstants) {
    EXPECT_EQ(MAX_GPR_ARGS, 6u);
    EXPECT_EQ(MAX_XMM_ARGS, 8u);
    EXPECT_EQ(NUM_CALLEE_SAVED, 5u);
    EXPECT_EQ(GPR_ARG_REGS[0], PhysReg::RDI);
    EXPECT_EQ(GPR_ARG_REGS[5], PhysReg::R9);
    EXPECT_EQ(XMM_ARG_REGS[0], PhysReg::XMM0);
    EXPECT_EQ(CALLEE_SAVED_GPRS[0], PhysReg::RBX);
}

TEST(MachIR, Width32Dump) {
    auto mi = makeAlu(X86Op::Add, MachOperand::precolored(PhysReg::RAX),
                      MachOperand::precolored(PhysReg::RDX), 32);
    std::ostringstream oss;
    dumpMachInstr(mi, oss);
    EXPECT_EQ(oss.str(), "    add eax, edx\n");
}
